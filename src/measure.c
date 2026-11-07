#include "intergrid/intergrid.h"
#include <math.h>

#define WINDOW_SIZE 100

static inline double rms_update(IgRmsSamples *rms, double sample) {
  if (rms->count == WINDOW_SIZE) {
    double old = rms->samples[rms->index];
    rms->sum_squared -= old * old;
  } else {
    rms->count++;
  }

  rms->samples[rms->index] = sample;
  rms->sum_squared += sample * sample;

  rms->index = (rms->index + 1) % WINDOW_SIZE;

  return sqrt(rms->sum_squared / (double)rms->count);
}


void ig_on_sample(Ig *ig, Pf *pf, double time, const float *buf_volts, const float *buf_amps, uint8_t len) {
  // TODO: analyze samples (more than just rms)
  for (int i = 0; i < len; i += 1) {
    ig->measurements.lines_volts_rms[i] = rms_update(&ig->measurements.lines_volts_rms_samples[i], buf_volts[i]);
    ig->measurements.lines_amps_rms[i] = rms_update(&ig->measurements.lines_amps_rms_samples[i], buf_amps[i]);
  }
  ig->measurements.lines_len = len;

  ig_on_protect(ig, pf);
}
