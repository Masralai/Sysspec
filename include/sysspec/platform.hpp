#pragma once
#include "sysspec/types.hpp"

namespace sysspec {

InfoPair getHostname();
InfoPair getOSInfo();
InfoPair getCPUInfo();
InfoPair getMemoryInfo();
InfoPair getGPUInfo();
InfoPair getDiskInfo();
InfoPair getResolutionInfo();
InfoPair getUptimeInfo();

} // namespace sysspec
