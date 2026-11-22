#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

extern "C" {
#include "intergrid/platform.h"
}

#define PF_RELAYS_LEN 4

struct Pf {
  uint8_t relay_pins[PF_RELAYS_LEN];
  PfRelayState relays[PF_RELAYS_LEN];
};

static Pf pf = {
    .relay_pins = {2, 3, 4, 5},
    .relays = {PF_RELAY_DISCONNECTED, PF_RELAY_DISCONNECTED,
               PF_RELAY_DISCONNECTED, PF_RELAY_DISCONNECTED},
};

// ---------------------------------------------------------------------------
// include/intergrid/platform.h
// ---------------------------------------------------------------------------

void pf_printf(Pf *pf, const char *fmt, ...) {
  (void)pf;

  char buf[128];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  Serial.print(buf);
}

void pf_set_relay(Pf *pf, uint8_t index, PfRelayState relay_state) {
  if (index >= PF_RELAYS_LEN)
    return;

  digitalWrite(pf->relay_pins[index],
               relay_state == PF_RELAY_CONNECTED ? HIGH : LOW);
  pf->relays[index] = relay_state;
}

// TODO: use the real radio. For now, dump the bytes as hex over serial.
void pf_radio_broadcast(Pf *pf, const uint8_t *data, uint8_t len) {
  pf_printf(pf, "[radio] %u bytes:", len);
  for (uint8_t i = 0; i < len; i++)
    pf_printf(pf, " %02X", data[i]);
  pf_printf(pf, "\n");
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_set_relay(void) {
  static PfRelayState state = PF_RELAY_DISCONNECTED;
  state = (state == PF_RELAY_CONNECTED) ? PF_RELAY_DISCONNECTED
                                        : PF_RELAY_CONNECTED;

  for (uint8_t i = 0; i < PF_RELAYS_LEN; i++) {
    pf_set_relay(&pf, i, state);

    int pin_level = digitalRead(pf.relay_pins[i]);
    int expected = (state == PF_RELAY_CONNECTED) ? HIGH : LOW;

    pf_printf(&pf, "[relay] %u -> %s (pin %u = %d) %s\n", i,
              state == PF_RELAY_CONNECTED ? "CONNECTED" : "DISCONNECTED",
              pf.relay_pins[i], pin_level,
              pin_level == expected ? "OK" : "FAIL");
  }

  // Out-of-range index must be a no-op.
  pf_set_relay(&pf, PF_RELAYS_LEN, PF_RELAY_CONNECTED);
}

static void test_radio_broadcast(void) {
  static uint8_t counter = 0;
  const uint8_t packet[] = {0x01, 0x02, 0xAB, counter++};
  pf_radio_broadcast(&pf, packet, sizeof(packet));
}

void setup() {
  Serial.begin(9600);

  for (uint8_t i = 0; i < PF_RELAYS_LEN; i++)
    pinMode(pf.relay_pins[i], OUTPUT);

  pf_printf(&pf, "Hello from pf_printf (%d, %s)\n", 42, "ok");
}

void loop() {
  pf_printf(&pf, "--- t=%lus ---\n", millis() / 1000);
  test_set_relay();
  test_radio_broadcast();

  delay(1000);
}
