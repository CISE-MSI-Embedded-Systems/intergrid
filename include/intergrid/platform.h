#pragma once

#include <stdint.h>

typedef enum {
  PF_RELAY_DISCONNECTED = 0,
  PF_RELAY_CONNECTED = 1,
} PfRelayState;

typedef struct Pf Pf;

void pf_radio_broadcast(Pf *pf, const uint8_t *data, uint8_t len);

void pf_set_relay(Pf *pf, uint8_t index, PfRelayState relay_state);

void pf_printf(Pf *pf, const char *fmt, ...);
