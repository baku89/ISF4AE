#include "ISF4AE.h"

PF_Err CreateDefaultArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbitraryH* dephault) {
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  PF_Handle arbH = suites.HandleSuite1()->host_new_handle(sizeof(ParamArbIsf));

  if (!arbH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));
  auto* isf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(arbH));

  if (!isf) {
    return PF_Err_OUT_OF_MEMORY;
  }

  AEFX_CLR_STRUCT(*isf);
  isf->name = "Default ISF4AE Shader";
  isf->desc = globalData->notLoadedSceneDesc;

  *dephault = arbH;

  suites.HandleSuite1()->host_unlock_handle(arbH);
  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);

  return PF_Err_NONE;
}

static PF_Err NewArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.new_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  ERR(CreateDefaultArb(in_data, out_data, extra->u.new_func_params.arbPH));

  return err;
}

static PF_Err DisposeArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.dispose_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);
  auto* isf =
      reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(extra->u.dispose_func_params.arbH));

  isf->name.clear();
  isf->desc = nullptr;

  suites.HandleSuite1()->host_unlock_handle(extra->u.dispose_func_params.arbH);
  suites.HandleSuite1()->host_dispose_handle(extra->u.dispose_func_params.arbH);

  return err;
}

static PF_Err CopyArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.copy_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  ERR(CreateDefaultArb(in_data, out_data, extra->u.copy_func_params.dst_arbPH));

  PF_Handle srcH = extra->u.copy_func_params.src_arbH;
  PF_Handle dstH = *extra->u.copy_func_params.dst_arbPH;

  if (!srcH || !dstH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  AEGP_SuiteHandler suites(in_data->pica_basicP);
  auto* srcIsf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(srcH));
  auto* dstIsf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(dstH));

  if (!srcIsf || !dstIsf) {
    return PF_Err_OUT_OF_MEMORY;
  }

  dstIsf->name = srcIsf->name;
  dstIsf->desc = srcIsf->desc;

  suites.HandleSuite1()->host_unlock_handle(srcH);
  suites.HandleSuite1()->host_unlock_handle(dstH);

  return err;
}

static PF_Err CompareArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.compare_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  PF_Handle aH = extra->u.compare_func_params.a_arbH;
  PF_Handle bH = extra->u.compare_func_params.b_arbH;

  if (!aH || !bH) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  AEGP_SuiteHandler suites(in_data->pica_basicP);
  auto* a = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(aH));
  auto* b = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(bH));

  if (!a || !b) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  bool isEqual = a->name == b->name && a->desc.get() == b->desc.get();

  PF_ArbCompareResult result = isEqual ? PF_ArbCompare_EQUAL : PF_ArbCompare_NOT_EQUAL;

  (*extra->u.compare_func_params.compareP) = result;

  suites.HandleSuite1()->host_unlock_handle(aH);
  suites.HandleSuite1()->host_unlock_handle(bH);

  return err;
}

static PF_Err FlatSizeArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.flat_size_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);
  auto* isf =
      reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(extra->u.flat_size_func_params.arbH));

  string fsCode = isf->desc->scene->getFragCode();
  string& vsCode = *isf->desc->scene->doc()->vertShaderSource();

  if (vsCode == VVISF::ISFVertPassthru_GL2) {
    vsCode = "";
  }

  A_u_long size =
      (A_u_long)(sizeof(ParamArbIsfFlatV1) + (isf->name.size() + fsCode.size() + vsCode.size()) * sizeof(char));

  *(extra->u.flat_size_func_params.flat_data_sizePLu) = size;

  suites.HandleSuite1()->host_unlock_handle(extra->u.flat_size_func_params.arbH);

  return err;
}

static PF_Err FlattenArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.flatten_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);
  auto* isf =
      reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(extra->u.flatten_func_params.arbH));
  ParamArbIsfFlatV1* dstHeader = (ParamArbIsfFlatV1*)extra->u.flatten_func_params.flat_dataPV;
  char* dstPV = (char*)extra->u.flatten_func_params.flat_dataPV;

  string fsCode = isf->desc->scene->getFragCode();
  string& vsCode = *isf->desc->scene->doc()->vertShaderSource();

  if (vsCode == VVISF::ISFVertPassthru_GL2) {
    vsCode = "";
  }

  A_u_long nameSize = (A_u_long)isf->name.size();
  A_u_long fsCodeSize = (A_u_long)fsCode.size();
  A_u_long vsCodeSize = (A_u_long)vsCode.size();

  dstHeader->magicNumber = ARB_ISF_MAGIC_NUMBER;
  dstHeader->version = 1;
  dstHeader->offsetName = sizeof(ParamArbIsfFlatV1);
  dstHeader->offsetFragCode = dstHeader->offsetName + nameSize;
  dstHeader->offsetVertCode = dstHeader->offsetFragCode + fsCodeSize;

  memcpy(dstPV + dstHeader->offsetName, isf->name.c_str(), nameSize * sizeof(char));
  memcpy(dstPV + dstHeader->offsetFragCode, fsCode.c_str(), fsCodeSize * sizeof(char));
  memcpy(dstPV + dstHeader->offsetVertCode, vsCode.c_str(), vsCodeSize * sizeof(char));

  suites.HandleSuite1()->host_unlock_handle(extra->u.flatten_func_params.arbH);

  return err;
}

static void UnflattenArbV0(GlobalData* globalData, char* flatData, A_u_long bufSize, ParamArbIsf* isf) {
  // Find an offset of '\0' to split name and code
  uint32_t nameSize = 0;
  for (nameSize = 0;; nameSize++) {
    if (flatData[nameSize] == '\0')
      break;
  }

  auto fsCode = string(flatData + nameSize + 1, bufSize - (nameSize + 1) * sizeof(char));

  isf->name = string(flatData, nameSize * sizeof(char));
  isf->desc = getCompiledSceneDesc(globalData, fsCode, "");
}

static void UnflattenArbV1(GlobalData* globalData, char* flatData, A_u_long bufSize, ParamArbIsf* isf) {
  ParamArbIsfFlatV1* flatHeader = reinterpret_cast<ParamArbIsfFlatV1*>(flatData);

  isf->name = string(flatData + flatHeader->offsetName, flatHeader->offsetFragCode - flatHeader->offsetName);

  auto fsCode = string(flatData + flatHeader->offsetFragCode, flatHeader->offsetVertCode - flatHeader->offsetFragCode);

  auto vsCode = string(flatData + flatHeader->offsetVertCode, bufSize - flatHeader->offsetVertCode);

  isf->desc = getCompiledSceneDesc(globalData, fsCode, vsCode);
}

static PF_Err UnflattenArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.unflatten_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  ERR(CreateDefaultArb(in_data, out_data, extra->u.unflatten_func_params.arbPH));

  PF_Handle arbH = *extra->u.unflatten_func_params.arbPH;

  AEGP_SuiteHandler suites(in_data->pica_basicP);
  auto* globalData = reinterpret_cast<GlobalData*>(suites.HandleSuite1()->host_lock_handle(in_data->global_data));
  auto* isf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(arbH));

  char* flatData = (char*)extra->u.unflatten_func_params.flat_dataPV;

  if (flatData[0] != ARB_ISF_MAGIC_NUMBER) {
    // Means version 0
    UnflattenArbV0(globalData, flatData, extra->u.unflatten_func_params.buf_sizeLu, isf);
  } else {
    UnflattenArbV1(globalData, flatData, extra->u.unflatten_func_params.buf_sizeLu, isf);
  }

  suites.HandleSuite1()->host_unlock_handle(arbH);
  suites.HandleSuite1()->host_unlock_handle(in_data->global_data);

  return err;
}

PF_Err HandleArbitrary(PF_InData* in_data,
                       PF_OutData* out_data,
                       PF_ParamDef* params[],
                       PF_LayerDef* output,
                       PF_ArbParamsExtra* extra) {
  PF_Err err = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  switch (extra->which_function) {
    case PF_Arbitrary_NEW_FUNC:
      ERR(NewArb(in_data, out_data, extra));
      break;

    case PF_Arbitrary_DISPOSE_FUNC:
      ERR(DisposeArb(in_data, out_data, extra));
      break;

    case PF_Arbitrary_COMPARE_FUNC:
      ERR(CompareArb(in_data, out_data, extra));
      break;

    case PF_Arbitrary_COPY_FUNC:
      ERR(CopyArb(in_data, out_data, extra));
      break;

    case PF_Arbitrary_FLAT_SIZE_FUNC:
      ERR(FlatSizeArb(in_data, out_data, extra));
      break;

    case PF_Arbitrary_FLATTEN_FUNC:
      ERR(FlattenArb(in_data, out_data, extra));
      break;

    case PF_Arbitrary_UNFLATTEN_FUNC:
      ERR(UnflattenArb(in_data, out_data, extra));
      break;
  }

  return err;
}
