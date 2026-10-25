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
    float edgeIrregularity; // bumps of the gate outline, micrometres
    float edgeIrrSize;      // size of the bumps (multiplier)
    float edgeIrrMotion;    // 0 = fixed outline, 1 = new outline every frame
    float gateDefocus;      // out-of-focus gate: soft edges and rounded, darker corners
    int   perfOn;
    int   perfStyle;      // 0 dark hole, 1 backlit (scanner light), 2 outline
    int   perfSide;       // 0 gauge default, 1 left, 2 right, 3 both
    float perfScale;
    float perfOpacity;
    float perfDefocus;      // soft, out-of-focus hole edge and light
    float perfIrregularity; // bumps of the hole outline, micrometres (uses Irregularity Motion)
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
    float grainSoftness;  // scanner / print softness of the grain (pixels x 0.6)
    float grainClumping;  // share of the grain in large clumps 0..1
    int   grainSeed;

    // ---- perforation light (halation of the scanner backlight around the holes)
    int   leakOn;         // always on in the plugin when perforations are shown
    float perfLightEV;    // exposure of the backlight, stops (-4..+4)
    float perfRim;        // width of the bright rim hugging the hole (multiplier)
    float perfHalo;       // size of the coloured glow and halo (multiplier)
    float coreR, coreG, coreB;      // colour at the hole edge
    float glowR, glowG, glowB;      // colour of the glow
    float outerR, outerG, outerB;   // colour of the wide cool halo
    float perfFringe;     // strength of the wide cool halo
    float perfVariation;  // how much the light changes between frames 0..1
    float perfSpeed;      // 0 slow drift .. 1 frame-to-frame changes
    float perfAsym;       // how far the glow leans to one side (offset of the halos)
    float perfAngle;      // direction the light leans to, degrees (0 = right, 90 = up)
    float perfLightOpacity; // overall mix 0..1
    float perfSaturation;   // saturation of the light (1 = colours as picked)
    float perfHotCore;      // how much the brightest part of the rim burns to white-yellow
    int   leakSeed;

    // ---- per-frame values, filled by psdv_prepare() on the host (not UI parameters)
    float regX[3], regY[3], regR[3];   // registration of frames T+1, T, T-1 (index slot + 1)
    float flick[3];                    // exposure flicker (stops) of the same frames
};
