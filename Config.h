#pragma once

#ifndef IS_PIPL
// Include PF_Stage
#include "AE_Effect.h"

#else
// For PiPL.r, define manually
#define PF_Stage_DEVELOP 0
#define PF_Stage_ALPHA 1
#define PF_Stage_BETA 2
#define PF_Stage_RELEASE 3

#endif

#define CONFIG_NAME "ISF"
#define CONFIG_MATCH_NAME "BAKU89 ISF"
#define CONFIG_CATEGORY "Shader"
#define CONFIG_DESCRIPTION "(c) 2022 Baku Hashimoto"

/* Versioning information */

#define MAJOR_VERSION 0
#define MINOR_VERSION 4
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 0
