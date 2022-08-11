#pragma once

#include "OGL.h"

// make sure we get 16bpc pixels;
// AE_Effect.h checks for this.
#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_GeneralPlug.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "String_Utils.h"

#include <memory>

#define PI 3.14159265358979323864f

namespace AEOGLInterop {

float getMultiplier16bit(GLenum pixelType) {
    return pixelType == GL_UNSIGNED_SHORT ? (65535.0f / 32768.0f) : 1.0f;
}

size_t getPixelBytes(GLenum pixelType) {
    switch (pixelType) {
        case GL_UNSIGNED_BYTE:
            return sizeof(PF_Pixel8);
        case GL_UNSIGNED_SHORT:
            return sizeof(PF_Pixel16);
        default:  //case GL_FLOAT:
            return sizeof(PF_PixelFloat);
    }
}

void uploadTexture(OGL::Texture *tex,
                   PF_LayerDef *layerDef,
                   void *pixelsBufferP,
                   GLenum pixelType) {
    GLsizei width = layerDef->width;
    GLsizei height = layerDef->height;

    size_t pixelBytes = getPixelBytes(pixelType);

    // Copy to buffer per row
    char *glP = nullptr;  // OpenGL
    char *aeP = nullptr;  // AE

    for (size_t y = 0; y < height; y++) {
        glP = (char *)pixelsBufferP + (height - y - 1) * width * pixelBytes;
        aeP = (char *)layerDef->data + y * layerDef->rowbytes;
        std::memcpy(glP, aeP, width * pixelBytes);
    }

    // Uplaod to texture
    tex->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, pixelType,
                    pixelsBufferP);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    tex->unbind();
}

PF_Err downloadTexture(const void *pixelsBufferP,
                       PF_LayerDef *layerDef,
                       GLenum pixelType) {
    PF_Err err = PF_Err_NONE;

    size_t pixelBytes = getPixelBytes(pixelType);

    size_t width = layerDef->width;
    size_t height = layerDef->height;

    char *glP = nullptr;  // OpenGL
    char *aeP = nullptr;  // AE

    // Copy per row
    for (size_t y = 0; y < height; y++) {
        glP = (char *)pixelsBufferP + (height - y - 1) * width * pixelBytes;
        aeP = (char *)layerDef->data + y * layerDef->rowbytes;
        std::memcpy(aeP, glP, width * pixelBytes);
    }
    return err;
}

enum { GL_SPACE = 1,
       AE_SPACE };

PF_Err getPointParam(PF_InData *in_data, PF_OutData *out_data, int paramId,
                     int space, A_FloatPoint *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));

    PF_PointParamSuite1 *pointSuite;
    ERR(AEFX_AcquireSuite(in_data, out_data, kPFPointParamSuite,
                          kPFPointParamSuiteVersion1, "Couldn't load suite.",
                          (void **)&pointSuite));

    // value takes downsampled coord
    ERR(pointSuite->PF_GetFloatingPointValueFromPointDef(in_data->effect_ref,
                                                         &param_def, value));

    float downsampleX = (float)in_data->downsample_x.num / in_data->downsample_x.den;
    float downsampleY = (float)in_data->downsample_y.num / in_data->downsample_y.den;

    if (space == GL_SPACE) {
        // Scale size by downsample ratio
        float width = (float)in_data->width * downsampleX;
        float height = (float)in_data->height * downsampleY;

        value->x /= width;
        value->y = 1.0f - value->y / height;
    } else {  // AE_SPACE
        // Convert to actual size
        value->x /= downsampleX;
        value->y /= downsampleY;
    }

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

PF_Err getAngleParam(PF_InData *in_data, PF_OutData *out_data, int paramId,
                     int space, A_FpLong *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));

    PF_AngleParamSuite1 *angleSuite;
    ERR(AEFX_AcquireSuite(in_data, out_data, kPFAngleParamSuite,
                          kPFAngleParamSuiteVersion1, "Couldn't load suite.",
                          (void **)&angleSuite));

    ERR(angleSuite->PF_GetFloatingPointValueFromAngleDef(in_data->effect_ref,
                                                         &param_def, value));

    if (space == GL_SPACE) {
        *value = -*value * PI / 180.0f;
    }

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));
    return err;
}

PF_Err getPopupParam(PF_InData *in_data, PF_OutData *out_data, int paramId, A_long *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));
    *value = param_def.u.pd.value;

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

PF_Err getFloatSliderParam(PF_InData *in_data, PF_OutData *out_data, int paramId, PF_FpLong *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));
    *value = param_def.u.fs_d.value;

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

PF_Err getCheckboxParam(PF_InData *in_data, PF_OutData *out_data, int paramId, PF_Boolean *value) {
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;

    PF_ParamDef param_def;
    AEFX_CLR_STRUCT(param_def);
    ERR(PF_CHECKOUT_PARAM(in_data, paramId, in_data->current_time,
                          in_data->time_step, in_data->time_scale, &param_def));
    *value = param_def.u.bd.value;

    ERR2(PF_CHECKIN_PARAM(in_data, &param_def));

    return err;
}

}  // namespace AEOGLInterop
