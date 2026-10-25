// psdv-film gate emulation - presets (UI side, C++ only).
// Choosing a preset copies its values into the manual sliders; editing any
// slider afterwards switches the menu back to "Custom".
// Provenance tags: [M] published value, [E] estimate. Sources: docs/PRESETS.md
#pragma once

struct PsdvWeavePreset
{
    const char* label;
    float weaveH, weaveV, randomness, cycle, rotation, scanJitter, flicker, flickerRandom;
};
// Weave in micrometres rms on the film. No numeric steadiness figures are
// published by the manufacturers; values are [E] estimates ordered by mechanism:
// dual registration pins < single pin < claw only < Super 8 cartridge (plastic
// pressure plate) - the ordering itself is documented in the literature.
static const PsdvWeavePreset kWeavePresets[] = {
    {"Custom",                                        0, 0, 0, 0, 0, 0, 0, 0},
    {"35mm Pin-Registered Camera (Arricam / Arriflex 435 class)", 1.0f, 1.5f, 0.45f, 12.0f, 0.00f, 0.6f, 0.015f, 0.4f},
    {"16mm Pin-Registered Camera (Arriflex SR class)",          1.5f, 2.5f, 0.50f, 10.0f, 0.00f, 1.0f, 0.025f, 0.5f},
    {"16mm Claw-Only Camera (Bolex H16 class)",                 4.0f, 8.0f, 0.60f,  8.0f, 0.02f, 1.5f, 0.060f, 0.6f},
    {"Regular 8 Spring Camera (Bolex H8 class)",                6.0f, 12.0f, 0.60f, 7.0f, 0.03f, 2.0f, 0.120f, 0.6f},
    {"Super 8 Cartridge Camera",                                8.0f, 18.0f, 0.65f, 6.0f, 0.05f, 2.0f, 0.100f, 0.6f},
    {"Worn Super 8 Camera (heavy)",                             16.0f, 35.0f, 0.70f, 5.0f, 0.10f, 3.0f, 0.180f, 0.7f},
    {"Projector Telecine (projected print)",                    10.0f, 14.0f, 0.30f, 24.0f, 0.02f, 4.0f, 0.080f, 0.3f},
    {"Locked (no movement)",                                    0, 0, 0.5f, 8.0f, 0, 0, 0, 0.5f},
};
static const int kWeavePresetCount = sizeof(kWeavePresets) / sizeof(kWeavePresets[0]);

struct PsdvGrainPreset
{
    const char* label;
    int emulsion;      // 0 colour negative, 1 colour reversal, 2 BW reversal, 3 BW negative
    float rms;         // diffuse RMS granularity x1000, D = 1.0, 48 um aperture
    float size, colorGrain, negGamma, shadow, mid, highlight;
    float softness, clumping;   // look of the structure [E]
};
static const PsdvGrainPreset kGrainPresets[] = {
    {"Custom",                                          0, 0, 0, 0, 0, 0, 0, 0, 0.00f, 0.00f},
    {"Kodak Vision3 50D 5203 style (estimate)",         0, 2.5f, 0.85f, 0.85f, 0.60f, 1.0f, 1.0f, 1.0f, 1.00f, 0.25f},
    {"Kodak Vision3 250D 5207 style (estimate)",        0, 4.0f, 1.15f, 0.85f, 0.60f, 1.0f, 1.0f, 1.0f, 1.00f, 0.40f},
    {"Kodak Vision3 200T 5213 style (estimate)",        0, 4.2f, 1.20f, 0.85f, 0.60f, 1.0f, 1.0f, 1.0f, 1.00f, 0.45f},
    {"Kodak Vision3 500T 5219 style (estimate)",        0, 6.0f, 1.45f, 0.85f, 0.60f, 1.0f, 1.0f, 1.0f, 1.00f, 0.60f},
    {"Eastman Double-X 5222 (RMS 14 published)",        3, 14.0f, 1.20f, 0.0f, 0.65f, 1.0f, 1.0f, 1.0f, 1.00f, 0.70f},
    {"Kodak Tri-X 7266 Reversal style (estimate)",      2, 16.0f, 1.10f, 0.0f, 0.60f, 1.0f, 1.0f, 1.0f, 1.00f, 0.75f},
    {"Kodak Ektachrome 100D 7294 style (E100 RMS 8)",   1, 8.0f, 1.00f, 0.85f, 0.60f, 1.0f, 1.0f, 1.0f, 1.00f, 0.40f},
};
static const int kGrainPresetCount = sizeof(kGrainPresets) / sizeof(kGrainPresets[0]);

struct PsdvLeakPreset
{
    const char* label;
    float opacity; int blend; float r, g, b; float frequency, spread, perfGlow, edgeGlow, flares, burn;
};
// Light leaks are a look, not a measurement: all values are aesthetic [E].
static const PsdvLeakPreset kLeakPresets[] = {
    {"Custom",         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Edge Leaks",     1.0f, 0, 1.00f, 0.28f, 0.04f, 0.3f, 1.0f, 0.90f, 0.70f, 0.00f, 0.00f},
    {"Warm Flares",    1.0f, 0, 1.00f, 0.38f, 0.08f, 1.0f, 2.0f, 0.30f, 0.30f, 0.80f, 0.00f},
    {"Organic Burn",   1.0f, 1, 1.00f, 0.22f, 0.02f, 0.5f, 1.5f, 0.40f, 0.40f, 0.20f, 0.90f},
    {"Subtle Glow",    0.6f, 0, 1.00f, 0.40f, 0.12f, 0.2f, 0.8f, 0.40f, 0.35f, 0.00f, 0.00f},
};
static const int kLeakPresetCount = sizeof(kLeakPresets) / sizeof(kLeakPresets[0]);

static const char* kFormatLabels[] = {
    "Regular 8mm", "Super 8mm", "Standard 16mm", "Super 16mm", "35mm Academy 4-perf",
    "Super 35mm 4-perf", "Super 35mm 3-perf", "35mm 2-perf (Techniscope)", "65mm 5-perf"};
static const int kFormatCount = 9;

struct PsdvGatePreset
{
    const char* label;
    int format, framing; float frameSize; int neighbors; float spacing;
    int perfOn, perfStyle; float edgeSoft, edgeDark, edgeDarkWidth, corner;
    float dust, fibres, gateHair, scratches;
    float irregularity, irrMotion, defocus;   // moving outline (um), 0..1, gate defocus
};
// Looks, not measurements: geometry comes from the gauge table, these only choose
// how the scan is framed and dressed.
static const PsdvGatePreset kGatePresets[] = {
    {"Custom",                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0f, 0.00f, 0.00f},
    {"Super 8 Scan - Dark Perforation",          1, 0, 0.76f, 2, 0.12f, 1, 0, 1.6f, 0.35f, 0.05f, 1.6f, 0.15f, 0.04f, 0.0f, 0.06f, 14.0f, 0.60f, 0.15f},
    {"Regular 8 Scan - Dark Perforation",        0, 0, 0.74f, 2, 0.10f, 1, 0, 1.4f, 0.35f, 0.05f, 1.3f, 0.20f, 0.12f, 0.0f, 0.08f, 16.0f, 0.60f, 0.20f},
    {"16mm Scan - Dark Perforations",            2, 0, 0.72f, 2, 0.05f, 1, 0, 1.2f, 0.25f, 0.04f, 1.2f, 0.10f, 0.06f, 0.0f, 0.04f, 10.0f, 0.50f, 0.10f},
    {"35mm Full Aperture Scan - Backlit",        5, 0, 0.70f, 2, 0.00f, 1, 1, 1.0f, 0.15f, 0.03f, 1.0f, 0.05f, 0.03f, 0.0f, 0.02f, 6.0f, 0.40f, 0.05f},
    {"Super 8 Picture Only (centred, no holes)", 1, 1, 1.00f, 0, 0.00f, 0, 0, 1.4f, 0.30f, 0.05f, 1.6f, 0.12f, 0.08f, 0.0f, 0.04f, 12.0f, 0.50f, 0.15f},
    {"Regular 8 Picture Only (centred, no holes)",0, 1, 1.00f, 0, 0.00f, 0, 0, 1.4f, 0.30f, 0.05f, 1.3f, 0.15f, 0.10f, 0.0f, 0.05f, 12.0f, 0.50f, 0.20f},
    {"Clean Gate (no dirt)",                     1, 0, 0.76f, 2, 0.12f, 1, 0, 1.0f, 0.10f, 0.05f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.30f, 0.00f},
};
static const int kGatePresetCount = sizeof(kGatePresets) / sizeof(kGatePresets[0]);
