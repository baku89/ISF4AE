#include "ISF4AE.h"

#include <adobesdk/DrawbotSuite.h>
#include "AEFX_SuiteHelper.h"

#include <cmath>
#include <string>

#include "Debug.h"
#include "MiscUtil.h"

DRAWBOT_MatrixF32 getLayer2FrameXform(PF_InData* in_data, PF_EventExtra* extra) {
  PF_FixedPoint pts[3];

  pts[0].x = FLOAT2FIX(0.0);
  pts[0].y = FLOAT2FIX(0.0);

  pts[1].x = FLOAT2FIX(1.0);
  pts[1].y = FLOAT2FIX(0.0);

  pts[2].x = FLOAT2FIX(0.0);
  pts[2].y = FLOAT2FIX(1.0);

  if ((*extra->contextH)->w_type == PF_Window_COMP) {
    for (A_short i = 0; i < 3; i++) {
      extra->cbs.layer_to_comp(extra->cbs.refcon, extra->contextH, in_data->current_time, in_data->time_scale, &pts[i]);
    }
  }

  for (A_short i = 0; i < 3; i++) {
    extra->cbs.source_to_frame(extra->cbs.refcon, extra->contextH, &pts[i]);
  }

  DRAWBOT_MatrixF32 xform;

  AEFX_CLR_STRUCT(xform);

  xform.mat[0][0] = FIX_2_FLOAT(pts[1].x) - FIX_2_FLOAT(pts[0].x);
  xform.mat[0][1] = FIX_2_FLOAT(pts[1].y) - FIX_2_FLOAT(pts[0].y);

  xform.mat[1][0] = FIX_2_FLOAT(pts[2].x) - FIX_2_FLOAT(pts[0].x);
  xform.mat[1][1] = FIX_2_FLOAT(pts[2].y) - FIX_2_FLOAT(pts[0].y);

  xform.mat[2][0] = FIX_2_FLOAT(pts[0].x);
  xform.mat[2][1] = FIX_2_FLOAT(pts[0].y);

  return xform;
}

DRAWBOT_UTF16Char* convertStringToUTF16Char(std::string line) {
  std::wstring lineWstr = std::wstring(line.begin(), line.end());
  DRAWBOT_UTF16Char* lineUTF16Char = new DRAWBOT_UTF16Char[lineWstr.length()];

  copyConvertStringLiteralIntoUTF16(lineWstr.c_str(), lineUTF16Char);

  return lineUTF16Char;
}

PF_Err DrawEvent(PF_InData* in_data,
                 PF_OutData* out_data,
                 PF_ParamDef* params[],
                 PF_LayerDef* output,
                 PF_EventExtra* extra) {
  PF_Err err = PF_Err_NONE;

  if ((*extra->contextH)->w_type != PF_Window_LAYER && (*extra->contextH)->w_type != PF_Window_COMP) {
    return err;
  }

  AEGP_SuiteHandler suites(in_data->pica_basicP);

  DRAWBOT_Suites drawbotSuites;
  DRAWBOT_DrawRef drawingRef = NULL;
  DRAWBOT_SurfaceRef surfaceRef = NULL;

  AEFX_AcquireDrawbotSuites(in_data, out_data, &drawbotSuites);
  ERR(suites.EffectCustomUISuite1()->PF_GetDrawingReference(extra->contextH, &drawingRef));

  if (!drawingRef) {
    return err;
  }

  ERR(suites.DrawbotSuiteCurrent()->GetSurface(drawingRef, &surfaceRef));

  auto* globalData = reinterpret_cast<GlobalData*>(DH(out_data->global_data));
  auto* isf = reinterpret_cast<ParamArbIsf*>(*params[Param_ISF]->u.arb_d.value);
  auto* sceneDesc = getCompiledSceneDesc(globalData, isf->code);

  // Retrieve the app's UI constants
  DRAWBOT_ColorRGBA foregroundColor, shadowColor, redColor;
  A_LPoint shadowOffset;

  if (in_data->appl_id != 'PrMr') {
    // Currently, EffectCustomUIOverlayThemeSuite is unsupported in Premiere Pro/Elements
    ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredForegroundColor(&foregroundColor));
    ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredShadowColor(&shadowColor));
    ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredShadowOffset(&shadowOffset));

  } else {
    foregroundColor = {0.9f, 0.9f, 0.9f, 1.0f};
    shadowColor = {0.0, 0.0, 0.0, 0.2};
    redColor = {0.9f, 0.0, 0.0, 1.0};
    shadowOffset.x = -1;
    shadowOffset.y = +1;
  }

  redColor = foregroundColor;
  redColor.green = 0;
  redColor.blue = 0;

  // Show error log
  if (!err && !sceneDesc->errorLog.empty()) {
    DRAWBOT_SupplierRef supplierRef = NULL;

    DRAWBOT_BrushRef redBrushRef = NULL, shadowBrushRef = NULL;
    DRAWBOT_FontRef fontRef = NULL, largeFontRef = NULL;
    float fontSize = 10.0;
    float largeFontScale = 1.5f;
    float padding = 3.0f;

    DRAWBOT_PointF32 origin = {padding, padding};

    auto layer2FrameXform = getLayer2FrameXform(in_data, extra);

    // Always set the scale to one regardless viewport zoom
    float zoom = hypot(layer2FrameXform.mat[0][0], layer2FrameXform.mat[0][1]);
    layer2FrameXform.mat[0][0] /= zoom;
    layer2FrameXform.mat[0][1] /= zoom;
    layer2FrameXform.mat[1][0] /= zoom;
    layer2FrameXform.mat[1][1] /= zoom;

    ERR(drawbotSuites.drawbot_suiteP->GetSupplier(drawingRef, &supplierRef));
    ERR(drawbotSuites.supplier_suiteP->NewBrush(supplierRef, &redColor, &redBrushRef));
    ERR(drawbotSuites.supplier_suiteP->NewBrush(supplierRef, &shadowColor, &shadowBrushRef));
    ERR(drawbotSuites.supplier_suiteP->GetDefaultFontSize(supplierRef, &fontSize));
    ERR(drawbotSuites.supplier_suiteP->NewDefaultFont(supplierRef, fontSize, &fontRef));
    ERR(drawbotSuites.supplier_suiteP->NewDefaultFont(supplierRef, fontSize * largeFontScale, &largeFontRef));

    // Start Transform
    ERR(drawbotSuites.surface_suiteP->PushStateStack(surfaceRef));
    ERR(drawbotSuites.surface_suiteP->Transform(surfaceRef, &layer2FrameXform));
    {
      // Print the type of error with larger font
      origin.y += fontSize * largeFontScale;

      origin.x += shadowOffset.x;
      origin.y += shadowOffset.y;

      auto* status = convertStringToUTF16Char(sceneDesc->status);
      ERR(suites.SurfaceSuiteCurrent()->DrawString(surfaceRef, shadowBrushRef, largeFontRef, &status[0], &origin,
                                                   kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));

      origin.x -= shadowOffset.x;
      origin.y -= shadowOffset.y;

      ERR(suites.SurfaceSuiteCurrent()->DrawString(surfaceRef, redBrushRef, largeFontRef, &status[0], &origin,
                                                   kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));

      delete[] status;

      origin.y += padding * 2;

      // Print error log for each line
      for (auto& line : splitWith(sceneDesc->errorLog, "\n")) {
        origin.y += fontSize + padding;

        auto* lineUTF16Char = convertStringToUTF16Char(line);

        origin.x += shadowOffset.x;
        origin.y += shadowOffset.y;

        ERR(suites.SurfaceSuiteCurrent()->DrawString(surfaceRef, shadowBrushRef, fontRef, &lineUTF16Char[0], &origin,
                                                     kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));

        origin.x -= shadowOffset.x;
        origin.y -= shadowOffset.y;

        ERR(suites.SurfaceSuiteCurrent()->DrawString(surfaceRef, redBrushRef, fontRef, &lineUTF16Char[0], &origin,
                                                     kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));

        delete[] lineUTF16Char;
      }
    }
    // End Transform
    ERR(drawbotSuites.surface_suiteP->PopStateStack(surfaceRef));

    // Release drawbot objects
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)redBrushRef));
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)shadowBrushRef));
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)fontRef));
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)largeFontRef));
  }

  AEFX_ReleaseDrawbotSuites(in_data, out_data);

  extra->evt_out_flags = PF_EO_HANDLED_EVENT;

  return err;
}

PF_Err HandleEvent(PF_InData* in_data,
                   PF_OutData* out_data,
                   PF_ParamDef* params[],
                   PF_LayerDef* output,
                   PF_EventExtra* event_extraP) {
  PF_Err err = PF_Err_NONE;

  switch (event_extraP->e_type) {
    case PF_Event_DRAW:
      err = DrawEvent(in_data, out_data, params, output, event_extraP);
      break;
  }

  return err;
}
