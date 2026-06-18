#pragma once

#include <stdint.h>

namespace csm::board::uplink {

enum class UplinkPriority : uint8_t {
  Diagnostic = 0,
  Normal = 1,
  Critical = 2,
};

}  // namespace csm::board::uplink
