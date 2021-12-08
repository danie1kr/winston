#pragma once

#ifdef WINSTON_PLATFORM_WIN_x64
#include <string>
void setStoragePath(std::string appendix);
#endif

#ifdef __cplusplus
extern "C" {
#endif
void winston_setup();
void winston_loop();

#ifdef __cplusplus
}
#endif
