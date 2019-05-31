#ifndef RX_MODEL_MESH_H
#define RX_MODEL_MESH_H

#include "rx/core/string.h"

namespace rx::model {

struct mesh {
  rx_size offset;
  rx_size count;
  string material;
};

}

#endif