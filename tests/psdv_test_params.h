// Shared by the CPU reference renderer and the Metal test: plugin defaults and
// name=value parameter parsing (names = PsdvParams members).
#pragma once
#include <cstring>
#include <string>
#include "core/psdv_params.h"

inline void psdvDefaults(PsdvParams& p)
{
    std::memset(&p, 0, sizeof(p));
    p.frameIndex = 1; p.encoding = 0; p.renderScale = 1;
    p.gateOn = 1; p.format = 1; p.gateFraming = 0; p.aspect = 0; p.frameSize = 0.76f; p.zoom = 1;
    p.centreOn = 0; p.imageFit = 0; p.imageZoom = 1; p.neighbors = 2; p.havePrev = 0; p.haveNext = 0;
    p.frameSpacing = 0.12f; p.edgeSoftness = 1.6f; p.edgeDarkness = 0.35f; p.edgeDarkWidth = 0.05f;
    p.cornerRadius = 1.6f; p.opticalSoft = 1.0f; p.edgeIrregularity = 14.0f; p.edgeIrrSize = 1.0f; p.edgeIrrMotion = 0.6f; p.gateDefocus = 0.15f; p.perfOn = 1; p.perfStyle = 0; p.perfSide = 0;
    p.perfScale = 1; p.perfOpacity = 1; p.perfIrregularity = 6.0f; p.dust = 0.15f; p.fibres = 0.04f; p.gateHair = 0; p.scratches = 0.06f;
    p.weaveOn = 1; p.weaveH = 8; p.weaveV = 18; p.weaveRandom = 0.65f; p.weaveCycle = 6; p.rotation = 0.05f;
    p.scanJitter = 2; p.flicker = 0.10f; p.flickerRandom = 0.6f;
    p.grainOn = 1; p.emulsion = 1; p.rms = 8; p.grainIntensity = 1; p.grainSize = 1;
    p.shadowResponse = 1; p.midResponse = 1; p.highlightResponse = 1; p.colorGrain = 0.85f; p.negGamma = 0.6f; p.grainSoftness = 1.0f; p.grainClumping = 0.4f;
    p.leakOn = 1; p.perfLightEV = 0; p.perfRim = 1.3f; p.perfHalo = 1.3f;
    p.coreR = 1.0f; p.coreG = 0.86f; p.coreB = 0.50f; p.glowR = 1.0f; p.glowG = 0.34f; p.glowB = 0.03f;
    p.outerR = 0.10f; p.outerG = 0.40f; p.outerB = 1.0f;
    p.perfFringe = 1.0f; p.perfVariation = 0.55f; p.perfSpeed = 0.6f; p.perfAsym = 1.0f; p.perfAngle = 190.0f; p.perfLightOpacity = 1.0f;
}

#define FIELD_I(n) if (k == #n) { p.n = (int)v; return true; }
#define FIELD_F(n) if (k == #n) { p.n = (float)v; return true; }
inline bool psdvSetParam(PsdvParams& p, const std::string& k, double v)
{
    FIELD_I(frameIndex) FIELD_I(encoding) FIELD_I(gateOn) FIELD_I(format) FIELD_I(gateFraming) FIELD_F(aspect)
    FIELD_F(frameSize) FIELD_F(zoom) FIELD_I(centreOn) FIELD_F(posX) FIELD_F(posY) FIELD_I(imageFit) FIELD_F(imageZoom)
    FIELD_F(panX) FIELD_F(panY) FIELD_I(neighbors) FIELD_F(frameSpacing) FIELD_F(edgeSoftness) FIELD_F(edgeDarkness)
    FIELD_F(edgeDarkWidth) FIELD_F(cornerRadius) FIELD_F(edgeIrregularity) FIELD_F(edgeIrrSize) FIELD_F(edgeIrrMotion) FIELD_F(gateDefocus) FIELD_F(opticalSoft) FIELD_I(perfOn) FIELD_I(perfStyle) FIELD_I(perfSide)
    FIELD_F(perfScale) FIELD_F(perfOpacity) FIELD_F(perfIrregularity) FIELD_F(dust) FIELD_F(fibres) FIELD_F(gateHair) FIELD_F(scratches)
    FIELD_I(dirtPolarity) FIELD_I(weaveOn) FIELD_F(weaveH) FIELD_F(weaveV) FIELD_F(weaveRandom) FIELD_F(weaveCycle)
    FIELD_F(rotation) FIELD_F(scanJitter) FIELD_F(flicker) FIELD_F(flickerRandom) FIELD_I(weaveSeed) FIELD_I(grainOn)
    FIELD_I(emulsion) FIELD_F(rms) FIELD_F(grainIntensity) FIELD_F(grainSize) FIELD_F(shadowResponse) FIELD_F(midResponse)
    FIELD_F(highlightResponse) FIELD_F(colorGrain) FIELD_F(negGamma) FIELD_F(grainSoftness) FIELD_F(grainClumping) FIELD_I(grainSeed) FIELD_I(leakOn) FIELD_F(perfLightEV) FIELD_F(perfRim) FIELD_F(perfHalo)
    FIELD_F(coreR) FIELD_F(coreG) FIELD_F(coreB) FIELD_F(glowR) FIELD_F(glowG) FIELD_F(glowB) FIELD_F(outerR) FIELD_F(outerG) FIELD_F(outerB)
    FIELD_F(perfFringe) FIELD_F(perfVariation) FIELD_F(perfSpeed) FIELD_F(perfAsym) FIELD_F(perfAngle) FIELD_F(perfLightOpacity) FIELD_I(leakSeed)
    return false;
}

