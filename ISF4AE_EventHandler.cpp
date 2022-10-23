#include "ISF4AE.h"

#include <adobesdk/DrawbotSuite.h>
#include "AEFX_SuiteHelper.h"

#include <codecvt>
#include <string>

#include "AEUtil.h"
#include "Debug.h"
#include "MiscUtil.h"

static DRAWBOT_MatrixF32 getLayer2FrameXform(PF_InData* in_data, PF_EventExtra* extra) {
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

static unique_ptr<DRAWBOT_UTF16Char[]> convertStringToUTF16Char(string str) {
  wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
  wstring wstr = converter.from_bytes(str);

  size_t length = wcslen(wstr.c_str());

  unique_ptr<DRAWBOT_UTF16Char[]> utf16char(new DRAWBOT_UTF16Char[length]);

  AEUtil::copyConvertStringLiteralIntoUTF16(wstr.c_str(), utf16char.get());

  return utf16char;
}

static PF_Err DrawEvent(PF_InData* in_data,
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

  auto* isf = reinterpret_cast<ParamArbIsf*>(*params[Param_ISF]->u.arb_d.value);
  auto sceneDesc = isf->desc;
  auto scene = sceneDesc->scene;

  // Determine if any Custom Comp UI should be rendered
  bool doRenderErrorLog = !sceneDesc->errorLog.empty();
  bool doRenderCustomUI = scene->inputNamed("i4a_CustomUI") != nullptr;

  bool doRenderAny = doRenderErrorLog || doRenderCustomUI;

  if (!err && doRenderAny) {
    // Retrieve the app's UI constants
    DRAWBOT_ColorRGBA foregroundColor, shadowColor, redColor;
    A_LPoint shadowOffset;
    float strokeWidth, vertexSize;

    if (in_data->appl_id != 'PrMr') {
      // Currently, EffectCustomUIOverlayThemeSuite is unsupported in Premiere Pro/Elements
      ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredForegroundColor(&foregroundColor));
      ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredShadowColor(&shadowColor));
      ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredShadowOffset(&shadowOffset));
      ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredStrokeWidth(&strokeWidth));
      ERR(suites.EffectCustomUIOverlayThemeSuite1()->PF_GetPreferredVertexSize(&vertexSize));

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

    float fontSize = 10.0;
    float largeFontScale = 1.5f;
    float padding = 3.0f;

    auto layer2FrameXform = getLayer2FrameXform(in_data, extra);

    // Setup Drawbot objects
    DRAWBOT_PointF32 origin = {padding, padding};
    DRAWBOT_SupplierRef supplierRef = NULL;
    DRAWBOT_BrushRef redBrushRef = NULL, shadowBrushRef = NULL;
    DRAWBOT_FontRef fontRef = NULL, largeFontRef = NULL;
    DRAWBOT_Rect32 clipRect;

    // Always set the scale to one regardless viewport zoom
    double zoom = suites.ANSICallbacksSuite1()->hypot(layer2FrameXform.mat[0][0], layer2FrameXform.mat[0][1]);
    layer2FrameXform.mat[0][0] /= zoom;
    layer2FrameXform.mat[0][1] /= zoom;
    layer2FrameXform.mat[1][0] /= zoom;
    layer2FrameXform.mat[1][1] /= zoom;

    clipRect.top = 0;
    clipRect.left = 0;
    clipRect.width = in_data->width * zoom;
    clipRect.height = in_data->height * zoom;

    ERR(drawbotSuites.drawbot_suiteP->GetSupplier(drawingRef, &supplierRef));
    ERR(drawbotSuites.supplier_suiteP->NewBrush(supplierRef, &redColor, &redBrushRef));
    ERR(drawbotSuites.supplier_suiteP->NewBrush(supplierRef, &shadowColor, &shadowBrushRef));
    ERR(drawbotSuites.supplier_suiteP->GetDefaultFontSize(supplierRef, &fontSize));
    ERR(drawbotSuites.supplier_suiteP->NewDefaultFont(supplierRef, fontSize, &fontRef));
    ERR(drawbotSuites.supplier_suiteP->NewDefaultFont(supplierRef, fontSize * largeFontScale, &largeFontRef));

    // Start Transform
    {
      auto* surface = suites.SurfaceSuiteCurrent();

      // Apply a transform
      ERR(drawbotSuites.surface_suiteP->PushStateStack(surfaceRef));
      ERR(drawbotSuites.surface_suiteP->Transform(surfaceRef, &layer2FrameXform));

      // Apply a clip
      ERR(drawbotSuites.surface_suiteP->PushStateStack(surfaceRef));
      surface->Clip(surfaceRef, supplierRef, &clipRect);

      if (doRenderErrorLog) {
        // Print the type of error with larger font
        origin.y += fontSize * largeFontScale;

        origin.x += shadowOffset.x;
        origin.y += shadowOffset.y;

        auto status = convertStringToUTF16Char(sceneDesc->status);
        ERR(surface->DrawString(surfaceRef, shadowBrushRef, largeFontRef, status.get(), &origin,
                                kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));

        origin.x -= shadowOffset.x;
        origin.y -= shadowOffset.y;

        ERR(surface->DrawString(surfaceRef, redBrushRef, largeFontRef, status.get(), &origin,
                                kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));

        origin.y += padding * 2;

        // Print error log for each line
        for (auto& line : splitWith(sceneDesc->errorLog, "\n")) {
          origin.y += fontSize + padding;

          auto lineUTF16Char = convertStringToUTF16Char(line);

          origin.x += shadowOffset.x;
          origin.y += shadowOffset.y;

          ERR(surface->DrawString(surfaceRef, shadowBrushRef, fontRef, lineUTF16Char.get(), &origin,
                                  kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));

          origin.x -= shadowOffset.x;
          origin.y -= shadowOffset.y;

          ERR(surface->DrawString(surfaceRef, redBrushRef, fontRef, lineUTF16Char.get(), &origin,
                                  kDRAWBOT_TextAlignment_Left, kDRAWBOT_TextTruncation_None, 0.0f));
        }
      } /* End doRenderErrorLog */

      if (doRenderCustomUI) {
        VVGL::Size outSize = {clipRect.width, clipRect.height};

        // Bind special uniforms reserved for ISF4AE
        scene->setValueForInputNamed(VVISF::ISFVal(VVISF::ISFValType_Point2D, zoom, zoom), "i4a_Downsample");
        scene->setValueForInputNamed(VVISF::ISFVal(VVISF::ISFValType_Bool, true), "i4a_CustomUI");
        scene->setValueForInputNamed(VVISF::ISFVal(VVISF::ISFValType_Color, foregroundColor.red, foregroundColor.green,
                                                   foregroundColor.blue, foregroundColor.alpha),
                                     "i4a_UIForegroundColor");
        scene->setValueForInputNamed(VVISF::ISFVal(VVISF::ISFValType_Color, shadowColor.red, shadowColor.green,
                                                   shadowColor.blue, shadowColor.alpha),
                                     "i4a_UIShadowColor");
        scene->setValueForInputNamed(VVISF::ISFVal(VVISF::ISFValType_Point2D, shadowOffset.x, shadowOffset.y),
                                     "i4a_UIShadowOffset");
        scene->setValueForInputNamed(VVISF::ISFVal(VVISF::ISFValType_Float, strokeWidth), "i4a_UIStrokeWidth");
        scene->setValueForInputNamed(VVISF::ISFVal(VVISF::ISFValType_Float, vertexSize), "i4a_UIVertexSize");

        // Prepare output buffer
        short bitdepth = 8;
        VVGL::GLBufferRef overlayImage = nullptr;
        VVGL::Size pointScale;
        pointScale.width = zoom * (double)in_data->downsample_x.den / in_data->downsample_x.num;
        pointScale.height = zoom * (double)in_data->downsample_y.den / in_data->downsample_y.num;

        ERR(renderISFToCPUBuffer(in_data, out_data, *scene, bitdepth, outSize, pointScale, &overlayImage));

        if (overlayImage) {
          DRAWBOT_ImageRef imageRef = nullptr;
          drawbotSuites.supplier_suiteP->NewImageFromBuffer(
              supplierRef, outSize.width, outSize.height, overlayImage->calculateBackingBytesPerRow(),
              kDRAWBOT_PixelLayout_32ARGB_Straight, overlayImage->cpuBackingPtr, &imageRef);

          // Render
          float opacity = 1.0f;
          origin.x = 0;
          origin.y = 0;
          ERR(surface->DrawImage(surfaceRef, imageRef, &origin, opacity));

          ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)imageRef));
        }
      } /* End doRenderCustomUI */

      // Pop clipping & transofrm stacks
      ERR(drawbotSuites.surface_suiteP->PopStateStack(surfaceRef));
      ERR(drawbotSuites.surface_suiteP->PopStateStack(surfaceRef));
    } /* End Transform */

    // Release drawbot objects
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)redBrushRef));
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)shadowBrushRef));
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)fontRef));
    ERR(drawbotSuites.supplier_suiteP->ReleaseObject((DRAWBOT_ObjectRef)largeFontRef));

    AEFX_ReleaseDrawbotSuites(in_data, out_data);
    extra->evt_out_flags = PF_EO_HANDLED_EVENT;

  } /* End doRenderSomething */

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
