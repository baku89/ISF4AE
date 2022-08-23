#include "ISF4AE.h"

#include "SystemUtil.h"
#include "AEUtil.h"

PF_Err
CreateDefaultArb(
    PF_InData            *in_data,
    PF_OutData            *out_data,
    PF_ArbitraryH        *dephault)
{
    AEGP_SuiteHandler    suites(in_data->pica_basicP);
    
    PF_Handle arbH = suites.HandleSuite1()->host_new_handle(sizeof(ParamArbIsf));
    
    if (!arbH) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    auto *isf = reinterpret_cast<ParamArbIsf*>(PF_LOCK_HANDLE(arbH));
    
    if (!isf) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    AEFX_CLR_STRUCT(*isf);
    PF_STRCPY(isf->code, "");
    
    *dephault = arbH;
    
    suites.HandleSuite1()->host_unlock_handle(arbH);
    
    return PF_Err_NONE;
}

PF_Err
ArbCopy(
    PF_InData                *in_data,
    PF_OutData                *out_data,
    const PF_ArbitraryH        *srcP,
    PF_ArbitraryH            *dstP)
{
    PF_Err err = PF_Err_NONE;
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    
    PF_Handle    srcH = *srcP;
    PF_Handle    dstH = *dstP;
        
    if (!srcH || !dstH) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    auto *srcIsf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(srcH));
    auto *dstIsf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(dstH));
    
    if (srcIsf && dstIsf) {
        memcpy(dstIsf, srcIsf, sizeof(ParamArbIsf));
        
    } else {
        err = PF_Err_OUT_OF_MEMORY;
    }
    
    suites.HandleSuite1()->host_unlock_handle(srcH);
    suites.HandleSuite1()->host_unlock_handle(dstH);
    
    return err;
}

PF_Err
ArbCompare(
    PF_InData                *in_data,
    PF_OutData               *out_data,
    const PF_ArbitraryH      *a_arbP,
    const PF_ArbitraryH      *b_arbP,
    PF_ArbCompareResult      *resultP)
{
    PF_Err err = PF_Err_NONE;
    
    PF_Handle    aH = *a_arbP,
                 bH = *b_arbP;

    
    
    if (!aH || !bH) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    
    auto *a = (ParamArbIsf*)PF_LOCK_HANDLE(aH),
         *b = (ParamArbIsf*)PF_LOCK_HANDLE(bH);
    
    if (!a || !b) {
        return PF_Err_UNRECOGNIZED_PARAM_TYPE;
    }
    
    bool isEqual = strcmp(a->code, b->code) == 0;
    
    *resultP = isEqual ? PF_ArbCompare_EQUAL : PF_ArbCompare_NOT_EQUAL;
    
    PF_UNLOCK_HANDLE(aH);
    PF_UNLOCK_HANDLE(bH);
    
    return err;
}

PF_Err
HandleArbitrary(
	PF_InData            *in_data,
	PF_OutData            *out_data,
	PF_ParamDef            *params[],
	PF_LayerDef            *output,
	PF_ArbParamsExtra    *extra)
{
	PF_Err     err     = PF_Err_NONE;
	
	switch (extra->which_function) {
		case PF_Arbitrary_NEW_FUNC:
			if (extra->u.new_func_params.refconPV != ARB_REFCON) {
				err = PF_Err_INTERNAL_STRUCT_DAMAGED;
			} else {
				err = CreateDefaultArb(in_data, out_data, extra->u.new_func_params.arbPH);
			}
			break;
			
		case PF_Arbitrary_DISPOSE_FUNC:
			if (extra->u.dispose_func_params.refconPV != ARB_REFCON) {
				err = PF_Err_INTERNAL_STRUCT_DAMAGED;
			} else {
				PF_DISPOSE_HANDLE(extra->u.dispose_func_params.arbH);
			}
			break;
		
		case PF_Arbitrary_COPY_FUNC:
			if(extra->u.copy_func_params.refconPV == ARB_REFCON) {
				ERR(CreateDefaultArb(in_data,
									 out_data,
									 extra->u.copy_func_params.dst_arbPH));

				ERR(ArbCopy(in_data,
							out_data,
							&extra->u.copy_func_params.src_arbH,
							extra->u.copy_func_params.dst_arbPH));
			}
			break;
		
		case PF_Arbitrary_FLAT_SIZE_FUNC:
			*(extra->u.flat_size_func_params.flat_data_sizePLu) = sizeof(ParamArbIsf);
			break;

		case PF_Arbitrary_FLATTEN_FUNC:
			if(extra->u.flatten_func_params.buf_sizeLu == sizeof(ParamArbIsf)){
				void *src = (ParamArbIsf*)PF_LOCK_HANDLE(extra->u.flatten_func_params.arbH);
				void *dst = extra->u.flatten_func_params.flat_dataPV;
				if (src) {
					memcpy(dst, src, sizeof(ParamArbIsf));
				}
				PF_UNLOCK_HANDLE(extra->u.flatten_func_params.arbH);
			}
			break;

		case PF_Arbitrary_UNFLATTEN_FUNC:
			if(extra->u.unflatten_func_params.buf_sizeLu == sizeof(ParamArbIsf)){
				PF_Handle    handle = PF_NEW_HANDLE(sizeof(ParamArbIsf));
				void *dst = (ParamArbIsf*)PF_LOCK_HANDLE(handle);
				void *src = (void*)extra->u.unflatten_func_params.flat_dataPV;
				if (src) {
					memcpy(dst, src, sizeof(ParamArbIsf));
				}
				*(extra->u.unflatten_func_params.arbPH) = handle;
				PF_UNLOCK_HANDLE(handle);
			}
			break;
		
		case PF_Arbitrary_COMPARE_FUNC:
			ERR(ArbCompare(in_data,
						   out_data,
						   &extra->u.compare_func_params.a_arbH,
						   &extra->u.compare_func_params.b_arbH,
						   extra->u.compare_func_params.compareP));
			break;
	}
	
	return err;
}
