#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void qmi8658_gforce_start(void);
void qmi8658_gforce_set_enabled(bool enabled);
bool qmi8658_gforce_is_enabled(void);

#ifdef __cplusplus
}
#endif
