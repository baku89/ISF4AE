#include "GLSLCanvas.h"

#include "SystemUtil.h"
#include "AEUtil.h"

PF_Err
CreateDefaultArb(
    PF_InData            *in_data,
    PF_OutData            *out_data,
    PF_ArbitraryH        *dephault)
{
    AEGP_SuiteHandler    suites(in_data->pica_basicP);
    
    PF_Handle arbH = suites.HandleSuite1()->host_new_handle(sizeof(ParamArbGlsl));
    
    if (!arbH) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    ParamArbGlsl *arb = reinterpret_cast<ParamArbGlsl*>(PF_LOCK_HANDLE(arbH));
    
    if (!arb) {
        return PF_Err_OUT_OF_MEMORY;
    }
    
    AEFX_CLR_STRUCT(*arb);
    
    std::string resourcePath = "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/glslCanvas/GLSLCanvas.plugin/Contents/Resources/"; //AEUtil::getResourcesPath(in_data);
    std::string fragCode = SystemUtil::readTextFile(resourcePath + "shaders/uv-gradient.frag");
    
    PF_STRCPY(arb->fragCode, fragCode.c_str());
    
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
    
    ParamArbGlsl *srcArb = reinterpret_cast<ParamArbGlsl*>(suites.HandleSuite1()->host_lock_handle(srcH));
    ParamArbGlsl *dstArb = reinterpret_cast<ParamArbGlsl*>(suites.HandleSuite1()->host_lock_handle(dstH));
    
    if (srcArb && dstArb) {
        memcpy(dstArb, srcArb, sizeof(ParamArbGlsl));
        
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
    
    ParamArbGlsl *a = (ParamArbGlsl*)PF_LOCK_HANDLE(aH),
                 *b = (ParamArbGlsl*)PF_LOCK_HANDLE(bH);
    
    if (!a || !b) {
        return PF_Err_UNRECOGNIZED_PARAM_TYPE;
    }
    
    bool isEqual = strcmp(a->fragCode, b->fragCode) == 0;
    
    *resultP = isEqual ? PF_ArbCompare_EQUAL : PF_ArbCompare_NOT_EQUAL;
    
    PF_UNLOCK_HANDLE(aH);
    PF_UNLOCK_HANDLE(bH);
    
    return err;
}
