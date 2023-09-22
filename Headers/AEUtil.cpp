#include "AEUtil.h"

#include <codecvt>

#include "AEConfig.h"
#include "AE_EffectCB.h"

namespace AEUtil {

string getResourcesPath(PF_InData* in_data) {
  // initialize and compile the shader objects
  A_UTF16Char pluginFolderPath[AEFX_MAX_PATH];
  PF_GET_PLATFORM_DATA(PF_PlatData_EXE_FILE_PATH_W, &pluginFolderPath);

#ifdef _WIN32
  // The resource path is only relevant for macOS
  return "";
#else
  NSUInteger length = 0;
  A_UTF16Char* tmp = pluginFolderPath;
  while (*tmp++ != 0) {
    ++length;
  }
  NSString* newStr = [[NSString alloc] initWithCharacters:pluginFolderPath length:length];
  string resourcePath([newStr UTF8String]);
  resourcePath += "/Contents/Resources/";
  return resourcePath;
#endif
}

PF_Err setParamVisibility(AEGP_PluginID aegpId,
                          PF_InData* in_data,
                          PF_ParamDef* params[],
                          PF_ParamIndex index,
                          A_Boolean visible) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  A_Boolean invisible = !visible;

  // For Premiere Pro
  PF_ParamDef newParam;
  AEFX_CLR_STRUCT(newParam);
  newParam = *params[index];

  setBitFlag(PF_PUI_INVISIBLE, invisible, &newParam.ui_flags);

  ERR(suites.ParamUtilsSuite3()->PF_UpdateParamUI(in_data->effect_ref, index, &newParam));

  // For After Effects
  AEGP_EffectRefH effectH = nullptr;
  AEGP_StreamRefH streamH = nullptr;

  ERR(suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(aegpId, in_data->effect_ref, &effectH));

  ERR(suites.StreamSuite5()->AEGP_GetNewEffectStreamByIndex(aegpId, effectH, index, &streamH));

  ERR(suites.DynamicStreamSuite4()->AEGP_SetDynamicStreamFlag(streamH, AEGP_DynStreamFlag_HIDDEN, FALSE, invisible));

  if (effectH)
    ERR2(suites.EffectSuite4()->AEGP_DisposeEffect(effectH));
  if (streamH)
    ERR2(suites.StreamSuite5()->AEGP_DisposeStream(streamH));

  return err;
}

PF_Err setParamName(AEGP_PluginID aegpId,
                    PF_InData* in_data,
                    PF_ParamDef* params[],
                    PF_ParamIndex index,
                    string& name) {
  PF_Err err = PF_Err_NONE;  //, err2 = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  // AEGP_EffectRefH effectH = nullptr;
  // AEGP_StreamRefH streamH = nullptr;

  // ERR(suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(aegpId, in_data->effect_ref, &effectH));
  // ERR(suites.StreamSuite5()->AEGP_GetNewEffectStreamByIndex(aegpId, effectH, index, &streamH));

  // A_UTF16Char* utf16Name;

  // wstring_convert<codecvt_utf8<char16_t>, char16_t> converter;
  // u16string wstr = converter.from_bytes(name.c_str());

  // utf16Name = (A_UTF16Char*)wstr.c_str();

  // TODO: Support unicode string. Using AEGP's SetStreamName pushes unnnecessary unco stack, so temporarily make it
  // support only ASCII characters.
  PF_ParamDef newParam;

  AEFX_CLR_STRUCT(newParam);
  newParam = *params[index];

  PF_STRCPY(newParam.name, name.c_str());

  ERR(suites.ParamUtilsSuite3()->PF_UpdateParamUI(in_data->effect_ref, index, &newParam));

  // ERR(suites.DynamicStreamSuite4()->AEGP_SetStreamName(streamH, utf16Name));

  // if (effectH)
  //   ERR2(suites.EffectSuite4()->AEGP_DisposeEffect(effectH));
  // if (streamH)
  //   ERR2(suites.StreamSuite5()->AEGP_DisposeStream(streamH));

  return err;
}

/**
 * Retrives a coordinate in pixel from point-type parameter with considering downsample
 */
PF_Err getPointParam(PF_InData* in_data, PF_OutData* out_data, int paramId, A_FloatPoint* value) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  PF_ParamDef param_def;
  AEFX_CLR_STRUCT(param_def);
  ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time, in_data->time_step, in_data->time_scale, &param_def));

  PF_PointParamSuite1* pointSuite;
  ERR(AEFX_AcquireSuite(in_data, out_data, kPFPointParamSuite, kPFPointParamSuiteVersion1, "Couldn't load suite.",
                        (void**)&pointSuite));

  // value takes downsampled coord
  ERR(pointSuite->PF_GetFloatingPointValueFromPointDef(in_data->effect_ref, &param_def, value));

  ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

  return err;
}

PF_Err getAngleParam(PF_InData* in_data, PF_OutData* out_data, int paramId, A_FpLong* value) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  PF_ParamDef param_def;
  AEFX_CLR_STRUCT(param_def);
  ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time, in_data->time_step, in_data->time_scale, &param_def));

  PF_AngleParamSuite1* angleSuite;
  ERR(AEFX_AcquireSuite(in_data, out_data, kPFAngleParamSuite, kPFAngleParamSuiteVersion1, "Couldn't load suite.",
                        (void**)&angleSuite));

  ERR(angleSuite->PF_GetFloatingPointValueFromAngleDef(in_data->effect_ref, &param_def, value));

  ERR2(PF_CHECKIN_PARAM(in_data, &param_def));
  return err;
}

PF_Err getPopupParam(PF_InData* in_data, PF_OutData* out_data, int paramId, A_long* value) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  PF_ParamDef param_def;
  AEFX_CLR_STRUCT(param_def);
  ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time, in_data->time_step, in_data->time_scale, &param_def));
  *value = param_def.u.pd.value;

  ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

  return err;
}

PF_Err getFloatSliderParam(PF_InData* in_data, PF_OutData* out_data, int paramId, PF_FpLong* value) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  PF_ParamDef param_def;
  AEFX_CLR_STRUCT(param_def);
  ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time, in_data->time_step, in_data->time_scale, &param_def));
  *value = param_def.u.fs_d.value;

  ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

  return err;
}

PF_Err getCheckboxParam(PF_InData* in_data, PF_OutData* out_data, int paramId, PF_Boolean* value) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  PF_ParamDef param_def;
  AEFX_CLR_STRUCT(param_def);
  ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time, in_data->time_step, in_data->time_scale, &param_def));
  *value = param_def.u.bd.value;

  ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

  return err;
}

/**
 Retrieves color parameter, in between 0.0-1.0 float value for each channel
 */
PF_Err getColorParam(PF_InData* in_data, PF_OutData* out_data, int paramIndex, PF_PixelFloat* value) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  PF_ParamDef paramDef;
  AEFX_CLR_STRUCT(paramDef);
  ERR(PF_CHECKOUT_PARAM(in_data, paramIndex, in_data->current_time, in_data->time_step, in_data->time_scale,
                        &paramDef));

  ERR(suites.ColorParamSuite1()->PF_GetFloatingPointColorFromColorDef(in_data->effect_ref, &paramDef, value));

  ERR2(PF_CHECKIN_PARAM(in_data, &paramDef));

  return err;
}

static PF_Err getAEGPEffectStream(AEGP_PluginID aegpId, PF_InData* in_data, AEGP_StreamRefH* effectStreamH) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  // https://ae-plugins.docsforadobe.dev/aegps/aegp-suites.html#streamrefs-and-effectrefs
  AEGP_EffectRefH effectH = nullptr;
  AEGP_StreamRefH firstParamH = nullptr;

  ERR(suites.PFInterfaceSuite1()->AEGP_GetNewEffectForEffect(aegpId, in_data->effect_ref, &effectH));

  // Assumes the effect has at least one parameter whose index is 1.
  ERR(suites.StreamSuite5()->AEGP_GetNewEffectStreamByIndex(aegpId, effectH, 1, &firstParamH));

  ERR(suites.DynamicStreamSuite4()->AEGP_GetNewParentStreamRef(aegpId, firstParamH, effectStreamH));

  if (effectH)
    ERR2(suites.EffectSuite4()->AEGP_DisposeEffect(effectH));
  if (firstParamH)
    ERR2(suites.StreamSuite5()->AEGP_DisposeStream(firstParamH));

  return err;
}

PF_Err getEffectName(AEGP_PluginID aegpId, PF_InData* in_data, string* name) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  AEGP_StreamRefH effectStreamH = nullptr;
  ERR(getAEGPEffectStream(aegpId, in_data, &effectStreamH));

  A_char effectName[AEGP_MAX_ITEM_NAME_SIZE];
  ERR(suites.StreamSuite2()->AEGP_GetStreamName(effectStreamH, FALSE, effectName));

  *name = string(effectName);

  if (effectStreamH)
    ERR2(suites.StreamSuite5()->AEGP_DisposeStream(effectStreamH));

  return err;
}

PF_Err setEffectName(AEGP_PluginID aegpId, PF_InData* in_data, const string& name) {
  PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
  AEGP_SuiteHandler suites(in_data->pica_basicP);

  AEGP_StreamRefH effectStreamH = nullptr;
  ERR(getAEGPEffectStream(aegpId, in_data, &effectStreamH));

  A_UTF16Char* utf16Name;

  wstring_convert<codecvt_utf8<char16_t>, char16_t> converter;
  u16string wstr = converter.from_bytes(name.c_str());

  utf16Name = (A_UTF16Char*)wstr.c_str();

  ERR(suites.DynamicStreamSuite4()->AEGP_SetStreamName(effectStreamH, utf16Name));

  if (effectStreamH)
    ERR2(suites.StreamSuite5()->AEGP_DisposeStream(effectStreamH));

  return err;
}

/**
 * Function to convert and copy string literals to A_UTF16Char.
 * On Win: Pass the input directly to the output
 * On Mac: All conversion happens through the CFString format
 */
void copyConvertStringLiteralIntoUTF16(const wchar_t* inputString, A_UTF16Char* destination) {
#ifdef AE_OS_MAC
  int length = wcslen(inputString);
  CFRange range = {0, AEGP_MAX_PATH_SIZE};
  range.length = length;
  CFStringRef inputStringCFSR =
      CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(inputString),
                              length * sizeof(wchar_t), kCFStringEncodingUTF32LE, FALSE);
  CFStringGetBytes(inputStringCFSR, range, kCFStringEncodingUTF16, 0, FALSE, reinterpret_cast<UInt8*>(destination),
                   length * (sizeof(A_UTF16Char)), NULL);
  destination[length] = 0;  // Set NULL-terminator, since CFString calls don't set it
  CFRelease(inputStringCFSR);
#elif defined _WIN32
  size_t length = wcslen(inputString);
  wcscpy_s(reinterpret_cast<wchar_t*>(destination), length + 1, inputString);
#endif
}

PF_Err getStringPersistentData(PF_InData* in_data,
                               const A_char* sectionKey,
                               const A_char* valueKey,
                               const string& defaultValue,
                               string* value) {
  PF_Err err = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  AEGP_PersistentBlobH blobH;
  ERR(suites.PersistentDataSuite4()->AEGP_GetApplicationBlob(AEGP_PersistentType_MACHINE_SPECIFIC, &blobH));

  char charStr[1024];
  A_u_long charSize = 0;

  ERR(suites.PersistentDataSuite4()->AEGP_GetString(blobH, sectionKey, valueKey, defaultValue.c_str(), 1024, charStr,
                                                    &charSize));

  (*value) = string(charStr, charSize);

  return err;
}

PF_Err setStringPersistentData(PF_InData* in_data,
                               const A_char* sectionKey,
                               const A_char* valueKey,
                               const string& value) {
  PF_Err err = PF_Err_NONE;

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  AEGP_PersistentBlobH blobH;
  ERR(suites.PersistentDataSuite4()->AEGP_GetApplicationBlob(AEGP_PersistentType_MACHINE_SPECIFIC, &blobH));

  ERR(suites.PersistentDataSuite3()->AEGP_SetString(blobH, sectionKey, valueKey, value.c_str()));

  return err;
}

}  // namespace AEUtil
