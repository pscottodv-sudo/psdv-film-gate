// psdv-film gate emulation - render parameters shared by CPU and Metal.
// Only 32-bit int/float members: identical memory layout in C++ and MSL.
#pragma once

struct PsdvParams
{
    // ---- frame / io
    int   width;          // output width in pixels (render scale applied)
    int   height;
    int   frameIndex;     // integer frame (time) used to seed per-frame randomness
    int   encoding;       // 0 DaVinci Intermediate, 1 Rec709 gamma 2.4, 2 linear
    float renderScale;    // OFX render scale (1 = full resolution)

    // ---- film gate & perforations
    int   gateOn;         // 0 = full-frame picture, no gate
    int   format;         // gauge index (see psdv_format)
    int   gateFraming;    // 0 manual frame size, 1 fit gate to screen, 2 fill screen with picture
    float aspect;         // 0 = native gate aspect, otherwise width / height
    float frameSize;      // gate height as a fraction of the output height
    float zoom;           // global zoom
    int   centreOn;       // 0 picture gate centre, 1 film centre line
    float posX, posY;     // global position, fraction of output size
    int   imageFit;       // 0 fill gate (crop), 1 fit whole image
    float imageZoom;      // picture zoom inside the gate
    float panX, panY;     // picture pan inside the gate
    int   neighbors;      // 0 off (black), 1 real previous/next frames, 2 repeat current
    int   havePrev, haveNext;
    float frameSpacing;   // extra distance between frames, fraction of gate height
    float edgeSoftness;   // gate edge penumbra multiplier
    float edgeDarkness;   // darkening of the picture along the gate edge (0..1)
    float edgeDarkWidth;  // width of that darkening, fraction of gate height
    float cornerRadius;   // multiplier of the gauge corner radius
    float opticalSoft;    // lens + emulsion softness multiplier
    int   perfOn;
    int   perfStyle;      // 0 dark hole, 1 backlit (scanner light), 2 outline
    int   perfSide;       // 0 gauge default, 1 left, 2 right, 3 both
    float perfScale;
    float perfOpacity;
    float dust, fibres, gateHair, scratches;
    int   dirtPolarity;   // 0 auto, 1 dark, 2 bright

    // ---- gate weave, jitter & flicker
    int   weaveOn;
    float weaveH;         // horizontal weave, micrometres rms on film
    float weaveV;         // vertical weave, micrometres rms on film
    float weaveRandom;    // 0 smooth drift .. 1 frame-to-frame jumps
    float weaveCycle;     // frames per drift cycle
    float rotation;       // rotational weave, degrees rms
    float scanJitter;     // whole frame jitter (scanner / projector), micrometres rms
    float flicker;        // exposure flicker, stops rms
    float flickerRandom;  // 0 slow pumping .. 1 frame-to-frame
    int   weaveSeed;

    // ---- film grain
    int   grainOn;
    int   emulsion;       // 0 colour negative, 1 colour reversal, 2 BW reversal, 3 BW negative
    float rms;            // diffuse RMS granularity x1000 at D = 1.0, 48 um aperture
    float grainIntensity; // multiplier of the physical amplitude
    float grainSize;      // dye cloud size multiplier
    float shadowResponse, midResponse, highlightResponse;
    float colorGrain;     // dye layer independence 0..1
    float negGamma;       // characteristic curve gamma used for negatives
    int   grainSeed;

    // ---- light leaks
    int   leakOn;
    float leakOpacity;
    int   leakBlend;      // 0 screen, 1 add, 2 lighten, 3 soft light
    float tintR, tintG, tintB;
    float leakFrequency;  // speed of change over time
    float leakSpread;     // size multiplier
    float perfGlow;       // glow around the perforations
    float edgeGlow;       // glow along the gate edges
    float flares;         // warm flares entering from the frame sides
    float burn;           // organic burn patches
    int   leakSeed;
};
