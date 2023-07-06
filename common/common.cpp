#include <xlog.h>

#include "common.h"

namespace flvpusher {

/////////////////////////////////////////////////////////////

static volatile bool is_interrupted = false;

bool interrupt_cb()
{
  return is_interrupted;
}

void set_interrupt(bool b)
{
  is_interrupted = b;
}

volatile bool *interrupt_variable()
{
  return &is_interrupted;
}

}
