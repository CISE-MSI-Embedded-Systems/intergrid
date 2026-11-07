#include "intergrid/intergrid.h"

void ig_init(Ig *ig, const IgConfig *config) {
  *ig = (Ig){
    .config = *config,
    .measurements = (IgMeasurements){
      .lines_len = 0,
    },
  };
}
