#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "intergrid/intergrid.h"
#include "intergrid/platform.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ngspice/sharedspice.h>

#define RADIO_PORT 5000
#define RADIO_MCAST_ADDR "239.192.0.1"

#define GRID_TIMESTEP "1m"
#define GRID_DURATION "3600"

#define GRID_SOURCE_VOLTAGE 16.97056275
#define GRID_LOAD_RESISTANCE 24.0

struct Pf {
  int radio_fd;

  Ig ig;

  PfRelayState relays[3];

  struct timespec wall_start;

  double time;
  double source_voltage;
  double bus_voltage;
  double load1_voltage;
  double load2_voltage;
  double source_current;
  double load1_current;
  double load2_current;

  double next_status_time;
};

static int create_udp_rx(uint16_t port, const char *mcast_addr) {
  int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  if (fd < 0)
    return -1;

  int reuse = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    goto error;

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr.s_addr = htonl(INADDR_ANY),
  };

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    goto error;

  if (mcast_addr) {
    struct ip_mreq membership = {
        .imr_multiaddr.s_addr = inet_addr(mcast_addr),
        .imr_interface.s_addr = htonl(INADDR_ANY),
    };

    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership,
                   sizeof(membership)) < 0)
      goto error;
  }

  return fd;

error:
  close(fd);
  return -1;
}

void pf_radio_broadcast(Pf *pf, const uint8_t *data, uint8_t len) {
  assert(pf->radio_fd >= 0);

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(RADIO_PORT),
      .sin_addr.s_addr = inet_addr(RADIO_MCAST_ADDR),
  };

  ssize_t sent = sendto(pf->radio_fd, data, len, 0, (struct sockaddr *)&addr,
                        sizeof(addr));

  assert(sent == (ssize_t)len);
}

void pf_set_relay(Pf *pf, uint8_t index, PfRelayState relay_state) {
  assert(index < 3);

  pf->relays[index] = relay_state;
}

void pf_printf(Pf *pf, const char *fmt, ...) {
  (void)pf;

  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

static int send_char(char *message, int ident, void *user) {
  (void)ident;
  (void)user;

  printf("[ngspice] %s\n", message);
  return 0;
}

static void print_grid_status(const Pf *pf) {
  printf("[grid] t=%7.2fs "
         "source=%7.2fV bus=%7.2fV "
         "load1=%10.6fA load2=%10.6fA "
         "relays=[%d %d %d]\n",
         pf->time, pf->ig.measurements.lines_volts_rms[0], pf->bus_voltage,
         pf->ig.measurements.lines_amps_rms[1],
         pf->ig.measurements.lines_amps_rms[2], pf->relays[0], pf->relays[1],
         pf->relays[2]);
}

static int send_data(pvecvaluesall data, int count, int ident, void *user) {
  (void)count;
  (void)ident;

  Pf *pf = user;
  double previous_time = pf->time;

  for (int i = 0; i < data->veccount; i++) {
    const char *name = data->vecsa[i]->name;
    double value = data->vecsa[i]->creal;

    // printf("vector: %s\n", data->vecsa[i]->name);

    if (strcmp(name, "time") == 0)
      pf->time = value;
    else if (strcmp(name, "source") == 0)
      pf->source_voltage = value;
    else if (strcmp(name, "bus") == 0)
      pf->bus_voltage = value;
    else if (strcmp(name, "load1") == 0)
      pf->load1_voltage = value;
    else if (strcmp(name, "load2") == 0)
      pf->load2_voltage = value;
    else if (strcmp(name, "v1#branch") == 0)
      pf->source_current = -value;
    else if (strcmp(name, "vsense1#branch") == 0)
      pf->load1_current = value;
    else if (strcmp(name, "vsense2#branch") == 0)
      pf->load2_current = value;
  }

  float voltages[3] = {pf->source_voltage, pf->load1_voltage,
                       pf->load2_voltage};
  float currents[3] = {pf->source_current, pf->load1_current,
                       pf->load2_current};

  ig_on_sample(&pf->ig, pf, pf->time, voltages, currents, 3);

  if (pf->time >= pf->next_status_time) {
    print_grid_status(pf);
    // pf->next_status_time = (double)((unsigned int)pf->time + 1);
    pf->next_status_time += 0.0005;
  }

  return 0;
}

static int send_init_data(pvecinfoall data, int ident, void *userdata) {
  (void)data;
  (void)ident;
  (void)userdata;

  return 0;
}

static int get_vsrc_data(double *voltage, double time, char *name, int ident,
                         void *user) {
  (void)time;
  (void)ident;

  Pf *pf = user;

  if (strcmp(name, "vctrl1") == 0) {
    *voltage = pf->relays[0] == PF_RELAY_CONNECTED ? 5.0 : 0.0;
  } else if (strcmp(name, "vctrl2") == 0) {
    *voltage = pf->relays[1] == PF_RELAY_CONNECTED ? 5.0 : 0.0;
  } else if (strcmp(name, "vctrl3") == 0) {
    *voltage = pf->relays[2] == PF_RELAY_CONNECTED ? 5.0 : 0.0;
  } else if (strcmp(name, "vres1") == 0 || strcmp(name, "vres2") == 0) {
    *voltage = GRID_LOAD_RESISTANCE;
  }

  return 0;
}

static int get_sync_data(double time, double *delta, double old_delta,
                         int redo_step, int ident, int location, void *user) {
  (void)delta;
  (void)old_delta;
  (void)redo_step;
  (void)ident;
  (void)location;

  Pf *pf = user;

  time_t seconds = (time_t)time;
  long nanoseconds = (long)((time - (double)seconds) * 1e9);

  struct timespec target = pf->wall_start;
  target.tv_sec += seconds;
  target.tv_nsec += nanoseconds;

  if (target.tv_nsec >= 1000000000L) {
    target.tv_sec++;
    target.tv_nsec -= 1000000000L;
  }

  while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, NULL) ==
         EINTR) {
  }

  return 0;
}

static char *circuit[] = {
    "* Intergrid power grid",

    "V1 source 0 SIN(0 16.97056275 60)",

    "VCTRL1 ctrl1 0 DC 0 EXTERNAL",
    "VCTRL2 ctrl2 0 DC 0 EXTERNAL",
    "VCTRL3 ctrl3 0 DC 0 EXTERNAL",

    "VRES1 ctrl_r1 0 DC 0 EXTERNAL",
    "VRES2 ctrl_r2 0 DC 0 EXTERNAL",

    "S1 source bus ctrl1 0 RELAY",
    "S2 bus load1 ctrl2 0 RELAY",
    "S3 bus load2 ctrl3 0 RELAY",

    "B_R1 load1 sense1 I = V(load1)/V(ctrl_r1)",
    "VSENSE1 sense1 0 DC 0",

    "B_R2 load2 sense2 I = V(load2)/V(ctrl_r2)",
    "VSENSE2 sense2 0 DC 0",

    ".model RELAY SW(Ron=0.01 Roff=1Meg Vt=2.5 Vh=0.1)",

    (".save time v(source) v(bus) v(load1) v(load2) "
     "i(V1) i(VSENSE1) i(VSENSE2)"),

    (".tran " GRID_TIMESTEP " " GRID_DURATION),
    ".end",
    NULL,
};

int main(void) {
  Pf pf = {
      .radio_fd = -1,
      .next_status_time = 1.0,
      .relays =
          {
              PF_RELAY_DISCONNECTED,
              PF_RELAY_DISCONNECTED,
              PF_RELAY_DISCONNECTED,
          },
  };

  pf.radio_fd = create_udp_rx(RADIO_PORT, RADIO_MCAST_ADDR);
  if (pf.radio_fd < 0) {
    perror("radio");
    return 1;
  }

  IgConfig config = {
      .lines =
          {
              (IgLineConfig){
                  .type = IG_LINE_TYPE_SOURCE,
                  .source =
                      {
                          .min_volts = 11.5f,
                          .max_volts = 13.5f,
                          .max_amps = 1.5f,
                      },
              },
              (IgLineConfig){
                  .type = IG_LINE_TYPE_LOAD,
                  .load =
                      {
                          .max_amps = 0.5f,
                      },
              },
              (IgLineConfig){
                  .type = IG_LINE_TYPE_LOAD,
                  .load =
                      {
                          .max_amps = 0.5f,
                      },
              },
          },
      .lines_len = 3,
  };

  ig_init(&pf.ig, &config);

  if (ngSpice_Init(send_char, NULL, NULL, send_data, send_init_data, NULL,
                   &pf) != 0) {
    fprintf(stderr, "Failed to initialize ngspice\n");
    goto error;
  }

  if (ngSpice_Init_Sync(get_vsrc_data, NULL, get_sync_data, NULL, &pf) != 0) {
    fprintf(stderr, "Failed to initialize ngspice synchronization\n");
    goto error;
  }

  if (ngSpice_Circ(circuit) != 0) {
    fprintf(stderr, "Failed to load ngspice circuit\n");
    goto error;
  }

  if (clock_gettime(CLOCK_MONOTONIC, &pf.wall_start) != 0) {
    perror("clock_gettime");
    goto error;
  }

  if (ngSpice_Command("bg_run") != 0) {
    fprintf(stderr, "Failed to start ngspice simulation\n");
    goto error;
  }

  const struct timespec delay = {
      .tv_sec = 0,
      .tv_nsec = 1000000,
  };

  while (!ngSpice_running())
    nanosleep(&delay, NULL);

  while (ngSpice_running())
    nanosleep(&delay, NULL);

  close(pf.radio_fd);
  return 0;

error:
  close(pf.radio_fd);
  return 1;
}

void pf_wait(void) {
  const struct timespec delay = {
      .tv_sec = 0,
      .tv_nsec = 1000000,
  };

  while (!ngSpice_running())
    nanosleep(&delay, NULL);

  while (ngSpice_running())
    nanosleep(&delay, NULL);
}

void pf_cleanup(Pf *pf) {
  if (pf->radio_fd >= 0) {
    close(pf->radio_fd);
    pf->radio_fd = -1;
  }
}
