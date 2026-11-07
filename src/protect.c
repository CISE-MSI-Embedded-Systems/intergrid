#include "intergrid/intergrid.h"
#include "intergrid/platform.h"

void ig_on_protect(Ig *ig, Pf *pf) {
  float available_amps = 0;
  for (int i = 0; i < ig->config.lines_len; i += 1) {
    switch (ig->config.lines[i].type) {
    case IG_LINE_TYPE_SOURCE:
      if (ig->measurements.lines_volts_rms[i] >= ig->config.lines[i].source.min_volts &&
          ig->measurements.lines_volts_rms[i] <= ig->config.lines[i].source.max_volts) {
        pf_printf(pf, "source mA = %f\n", ig->measurements.lines_amps_rms[i]);
        pf_printf(pf, "source open %d\n", i);
        pf_set_relay(pf, i, PF_RELAY_CONNECTED);
        available_amps += ig->config.lines[i].source.max_amps;
      } else {
        pf_printf(pf, "source close %d\n", i);
        // pf_set_relay(pf, i, PF_RELAY_DISCONNECTED);
      }
      break;
    case IG_LINE_TYPE_LOAD:
      break;
    case IG_LINE_TYPE_TIE:
      // TODO: take into consideration what was been promissed (if function as source)
      pf_set_relay(pf, i, PF_RELAY_DISCONNECTED);
      break;
    }
  }

  float remaining_amps = available_amps;
  for (int i = 0; i < ig->config.lines_len; i += 1) {
    switch (ig->config.lines[i].type) {
    case IG_LINE_TYPE_SOURCE:
      break;
    case IG_LINE_TYPE_LOAD:
      // TODO: currently flikering when measured current is greater than configured
      if (ig->config.lines[i].load.max_amps <= remaining_amps &&
          ig->measurements.lines_amps_rms[i] <= ig->config.lines[i].load.max_amps) {
        pf_set_relay(pf, i, PF_RELAY_CONNECTED);
        remaining_amps -= ig->config.lines[i].load.max_amps;
        pf_printf(pf, "load open %d\n", i);
        pf_printf(pf, "remaining_amps = %f\n", remaining_amps);
      } else {
        pf_printf(pf, "load close %d\n", i);
        pf_printf(pf, "remaining_amps = %f\n", remaining_amps);
        pf_set_relay(pf, i, PF_RELAY_DISCONNECTED);
      }
      break;
    case IG_LINE_TYPE_TIE:
      // TODO: take into consideration what was been promissed (if function as load)
      pf_set_relay(pf, i, PF_RELAY_DISCONNECTED);
      break;
    }
  }
}
