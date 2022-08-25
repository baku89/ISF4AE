#include "ISF4AE.h"

PF_Err CreateDefaultArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbitraryH* dephault) {
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  PF_Handle arbH = suites.HandleSuite1()->host_new_handle(sizeof(ParamArbIsf));

  if (!arbH) {
    return PF_Err_OUT_OF_MEMORY;
  }

  auto* isf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(arbH));

  if (!isf) {
    return PF_Err_OUT_OF_MEMORY;
  }

  AEFX_CLR_STRUCT(*isf);
  isf->name = "Default ISF4AE Shader";
  isf->code = "";

  *dephault = arbH;

  suites.HandleSuite1()->host_unlock_handle(arbH);

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
  isf->code.clear();

  suites.HandleSuite1()->host_unlock_handle(extra->u.dispose_func_params.arbH);
  suites.HandleSuite1()->host_dispose_handle(extra->u.dispose_func_params.arbH);

  return err;
}

static PF_Err CopyArb(PF_InData* in_data,
                      PF_OutData* out_data,
                      PF_ArbParamsExtra* extra) {
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
  dstIsf->code = srcIsf->code;

  suites.HandleSuite1()->host_unlock_handle(srcH);
  suites.HandleSuite1()->host_unlock_handle(dstH);

  return err;
}

static PF_Err CompareArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.copy_func_params.refconPV != ARB_REFCON) {
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

  bool isEqual = a->name == b->name && a->code == b->code;

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

  A_u_long size = (A_u_long)(isf->name.size() + 1 + isf->code.size()) * sizeof(char);

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
  char* dst = (char*)extra->u.flatten_func_params.flat_dataPV;

  auto nameSize = isf->name.size();
  auto codeSize = isf->code.size();

  memcpy(dst, isf->name.c_str(), nameSize * sizeof(char));

  // Insert '\0' as a delimiter to split name and code
  dst[nameSize] = '\0';

  memcpy(dst + nameSize + 1, isf->code.c_str(), codeSize * sizeof(char));

  suites.HandleSuite1()->host_unlock_handle(extra->u.flatten_func_params.arbH);

  return err;
}

static PF_Err UnflattenArb(PF_InData* in_data, PF_OutData* out_data, PF_ArbParamsExtra* extra) {
  if (extra->u.unflatten_func_params.refconPV != ARB_REFCON) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
  }

  PF_Err err = PF_Err_NONE;

  ERR(CreateDefaultArb(in_data, out_data, extra->u.unflatten_func_params.arbPH));

  PF_Handle arbH = *extra->u.unflatten_func_params.arbPH;

  AEGP_SuiteHandler suites(in_data->pica_basicP);
  auto* isf = reinterpret_cast<ParamArbIsf*>(suites.HandleSuite1()->host_lock_handle(arbH));

  char* src = (char*)extra->u.unflatten_func_params.flat_dataPV;

  // Find an offset of '\0' to split name and code
  uint32_t nameSize = 0;
  for (nameSize = 0;; nameSize++) {
    if (src[nameSize] == '\0')
      break;
  }

  isf->name = std::string(src, nameSize * sizeof(char));
  isf->code =
      std::string(src + nameSize + 1, extra->u.unflatten_func_params.buf_sizeLu - (nameSize + 1) * sizeof(char));

  suites.HandleSuite1()->host_unlock_handle(arbH);

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
