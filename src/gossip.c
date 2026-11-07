#include "intergrid/intergrid.h"
#include "intergrid/platform.h"

void ig_on_radio_rx(Ig *ig, Pf *pf,const uint8_t *buf, uint8_t len) {
  // TODO: gossip stuff
  // TODO: add decay to echoing
  pf_radio_broadcast(pf, buf, len);
  // TODO: node stuff
  ig_on_protect(ig, pf);
}
