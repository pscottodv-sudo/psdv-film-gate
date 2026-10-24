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
    p.cornerRadius = 1.6f; p.opticalSoft = 1.0f; p.perfOn = 1; p.perfStyle = 0; p.perfSide = 0;
    p.perfScale = 1; p.perfOpacity = 1; p.dust = 0.15f; p.fibres = 0.04f; p.gateHair = 0; p.scratches = 0.06f;
    p.weaveOn = 1; p.weaveH = 8; p.weaveV = 18; p.weaveRandom = 0.65f; p.weaveCycle = 6; p.rotation = 0.05f;
    p.scanJitter = 2; p.flicker = 0.10f; p.flickerRandom = 0.6f;
    p.grainOn = 1; p.emulsion = 1; p.rms = 8; p.grainIntensity = 1; p.grainSize = 1;
    p.shadowResponse = 1; p.midResponse = 1; p.highlightResponse = 1; p.colorGrain = 0.85f; p.negGamma = 0.6f; p.grainSoftness = 1.0f; p.grainClumping = 0.4f;
    p.leakOn = 1; p.leakOpacity = 1; p.leakBlend = 0; p.tintR = 1; p.tintG = 0.28f; p.tintB = 0.04f;
    p.leakFrequency = 0.3f; p.leakSpread = 1; p.perfGlow = 0.9f; p.edgeGlow = 0.7f;
}

#define FIELD_I(n) if (k == #n) { p.n = (int)v; return true; }
#define FIELD_F(n) if (k == #n) { p.n = (float)v; return true; }
inline bool psdvSetParam(PsdvParams& p, const std::string& k, double v)
{
    FIELD_I(frameIndex) FIELD_I(encoding) FIELD_I(gateOn) FIELD_I(format) FIELD_I(gateFraming) FIELD_F(aspect)
    FIELD_F(frameSize) FIELD_F(zoom) FIELD_I(centreOn) FIELD_F(posX) FIELD_F(posY) FIELD_I(imageFit) FIELD_F(imageZoom)
    FIELD_F(panX) FIELD_F(panY) FIELD_I(neighbors) FIELD_F(frameSpacing) FIELD_F(edgeSoftness) FIELD_F(edgeDarkness)
    FIELD_F(edgeDarkWidth) FIELD_F(cornerRadius) FIELD_F(opticalSoft) FIELD_I(perfOn) FIELD_I(perfStyle) FIELD_I(perfSide)
    FIELD_F(perfScale) FIELD_F(perfOpacity) FIELD_F(dust) FIELD_F(fibres) FIELD_F(gateHair) FIELD_F(scratches)
    FIELD_I(dirtPolarity) FIELD_I(weaveOn) FIELD_F(weaveH) FIELD_F(weaveV) FIELD_F(weaveRandom) FIELD_F(weaveCycle)
    FIELD_F(rotation) FIELD_F(scanJitter) FIELD_F(flicker) FIELD_F(flickerRandom) FIELD_I(weaveSeed) FIELD_I(grainOn)
    FIELD_I(emulsion) FIELD_F(rms) FIELD_F(grainIntensity) FIELD_F(grainSize) FIELD_F(shadowResponse) FIELD_F(midResponse)
    FIELD_F(highlightResponse) FIELD_F(colorGrain) FIELD_F(negGamma) FIELD_F(grainSoftness) FIELD_F(grainClumping) FIELD_I(grainSeed) FIELD_I(leakOn) FIELD_F(leakOpacity)
    FIELD_I(leakBlend) FIELD_F(tintR) FIELD_F(tintG) FIELD_F(tintB) FIELD_F(leakFrequency) FIELD_F(leakSpread)
    FIELD_F(perfGlow) FIELD_F(edgeGlow) FIELD_F(flares) FIELD_F(burn) FIELD_I(leakSeed)
    return false;
}

