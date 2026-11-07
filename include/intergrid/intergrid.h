#pragma once

#include "intergrid/platform.h"

#include <stdint.h>

#define IG_CONFIG_LINES_MAX 16
#define IG_RMS_WINDOW_SAMPLES 100

typedef enum {
  IG_LINE_TYPE_SOURCE,
  IG_LINE_TYPE_LOAD,
  IG_LINE_TYPE_TIE,
} IgLineConfigType;

typedef struct {
  float min_volts;
  float max_volts;
  float max_amps;
} IgLineConfigSource;

typedef struct {
  float max_amps;
} IgLineConfigLoad;

typedef struct {
  float min_volts;
  float max_volts;
  float max_amps;
  uint32_t node_id;
} IgLineConfigTie;

typedef struct {
  IgLineConfigType type;
  union {
    IgLineConfigSource source;
    IgLineConfigLoad load;
    IgLineConfigTie tie;
  };
} IgLineConfig;

typedef struct {
  IgLineConfig lines[IG_CONFIG_LINES_MAX];
  uint8_t lines_len;
} IgConfig;

typedef struct {
  double samples[IG_RMS_WINDOW_SAMPLES];
  double sum_squared;
  int index;
  int count;
} IgRmsSamples;

typedef struct {
  IgRmsSamples lines_volts_rms_samples[IG_CONFIG_LINES_MAX];
  IgRmsSamples lines_amps_rms_samples[IG_CONFIG_LINES_MAX];
  float lines_amps_rms[IG_CONFIG_LINES_MAX];
  float lines_volts_rms[IG_CONFIG_LINES_MAX];
  uint8_t lines_len;
} IgMeasurements;

typedef struct {
  IgConfig config;
  IgMeasurements measurements;
} Ig;

void ig_init(Ig *ig, const IgConfig *config);

void ig_on_config(Ig *ig, Pf *pf, const uint8_t *buf, uint8_t len);
void ig_on_radio_rx(Ig *ig, Pf *pf, const uint8_t *buf, uint8_t len);

void ig_on_sample(Ig *ig, Pf *pf, double time, const float *buf_volts,
                  const float *buf_amps, uint8_t len);

void ig_on_protect(Ig *ig, Pf *pf);
