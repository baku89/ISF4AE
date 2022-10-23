#include "AEConfig.h"
#include "AE_EffectVers.h"

#ifndef AE_OS_WIN
	#include <AE_General.r>
#endif

#define IS_PIPL

#include "Config.h"
	
resource 'PiPL' (16000) {
	{	/* array properties: 12 elements */
		/* [1] */
		Kind {
			AEEffect
		},
		/* [2] */
		Name {
			CONFIG_NAME
		},
		/* [3] */
		Category {
			CONFIG_CATEGORY
		},
#ifdef AE_OS_WIN
	#ifdef AE_PROC_INTELx64
		CodeWin64X86 {"EffectMain"},
	#endif
#else
	#ifdef AE_OS_MAC
		CodeMacIntel64 {"EffectMain"},
		CodeMacARM64 {"EffectMain"},
	#endif
#endif
		/* [6] */
		AE_PiPL_Version {
			2,
			0
		},
		/* [7] */
		AE_Effect_Spec_Version {
			PF_PLUG_IN_VERSION,
			PF_PLUG_IN_SUBVERS
		},
		/* [8] */
		AE_Effect_Version {
			// https://community.adobe.com/t5/after-effects-discussions/pipl-and-code-version-mismatch-warning/td-p/5531272
			MAJOR_VERSION * 0x80000 +
			MINOR_VERSION * 0x8000 +
			BUG_VERSION   * 0x800 +
			STAGE_VERSION * 0x200 +
			BUILD_VERSION
		},
		/* [9] */
		AE_Effect_Info_Flags {
			0
		},
		/* [10] */
		AE_Effect_Global_OutFlags {
            0x06008024

		},
		AE_Effect_Global_OutFlags_2 {
            0x08001401
		},
		/* [11] */
		AE_Effect_Match_Name {
			CONFIG_MATCH_NAME
		},
		/* [12] */
		AE_Reserved_Info {
			0
		}
	}
};

