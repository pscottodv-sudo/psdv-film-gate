// psdv-film gate emulation - per-pixel core (C++ and Metal).
// Coordinates: OFX convention, pixel (0,0) is bottom-left, y grows upwards.
// Units on film are millimetres. Data provenance: see docs/PRESETS.md.
#pragma once
#include "psdv_prelude.h"
#include "psdv_params.h"

// =============================================================== random numbers
PS_INLINE uint psdv_hash(uint x)
{
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}
PS_INLINE uint psdv_hash3(int a, int b, int c)
{
    return psdv_hash((uint)a * 0x8da6b343u ^ psdv_hash((uint)b * 0xd8163841u ^ psdv_hash((uint)c * 0xcb1ab31fu + 0x9e3779b9u)));
}
PS_INLINE float psdv_rand(int a, int b, int c)
{
    return (float)(psdv_hash3(a, b, c) >> 8) * (1.0f / 16777216.0f);
}
PS_INLINE ps3 psdv_cell(int a, int b, int c)
{
    uint h = psdv_hash3(a, b, c);
    return P3(((float)(h & 1023u) + 0.5f) * (1.0f / 1024.0f),
              ((float)((h >> 10) & 1023u) + 0.5f) * (1.0f / 1024.0f),
              ((float)(h >> 20) + 0.5f) * (1.0f / 4096.0f));
}
// smooth value noise over frames, [0,1)
PS_INLINE float psdv_noiseT(float t, int salt, int seed)
{
    float f = ps_floor(t);
    int i = (int)f;
    float a = t - f;
    a = a * a * (3.0f - 2.0f * a);
    return ps_lerp(psdv_rand(i, salt, seed), psdv_rand(i + 1, salt, seed), a);
}
// smooth 2D value noise, [0,1)
PS_INLINE float psdv_noise2(float x, float y, int seed)
{
    float fx = ps_floor(x), fy = ps_floor(y);
    int ix = (int)fx, iy = (int)fy;
    float ax = x - fx, ay = y - fy;
    ax = ax * ax * (3.0f - 2.0f * ax);
    ay = ay * ay * (3.0f - 2.0f * ay);
    float a = ps_lerp(psdv_rand(ix, iy, seed), psdv_rand(ix + 1, iy, seed), ax);
    float b = ps_lerp(psdv_rand(ix, iy + 1, seed), psdv_rand(ix + 1, iy + 1, seed), ax);
    return ps_lerp(a, b, ay);
}
PS_INLINE float psdv_fbm(float x, float y, int seed)
{
    return 0.55f * psdv_noise2(x, y, seed) + 0.30f * psdv_noise2(x * 2.03f + 7.1f, y * 2.03f - 3.3f, seed + 1)
         + 0.15f * psdv_noise2(x * 4.11f - 1.7f, y * 4.11f + 9.2f, seed + 2);
}

// =============================================================== encodings
// DaVinci Intermediate (Blackmagic DWG/Intermediate whitepaper constants)
PS_INLINE float psdv_decode1(float v, int enc)
{
    if (enc == 0) return v <= 0.02740668f ? v / 10.44426855f : ps_exp2(v / 0.07329248f - 7.0f) - 0.0075f;
    if (enc == 1) return v <= 0.0f ? v : ps_pow(v, 2.4f);
    return v;
}
PS_INLINE float psdv_encode1(float e, int enc)
{
    if (enc == 0) return e <= 0.00262409f ? e * 10.44426855f : (ps_log2(e + 0.0075f) + 7.0f) * 0.07329248f;
    if (enc == 1) return e <= 0.0f ? e : ps_pow(e, 1.0f / 2.4f);
    return e;
}
PS_INLINE ps3 psdv_decode(ps3 v, int enc) { return P3(psdv_decode1(v.x, enc), psdv_decode1(v.y, enc), psdv_decode1(v.z, enc)); }
PS_INLINE ps3 psdv_encode(ps3 e, int enc) { return P3(psdv_encode1(e.x, enc), psdv_encode1(e.y, enc), psdv_encode1(e.z, enc)); }

// =============================================================== gauges (mm)
struct PsdvFormat
{
    float filmW, gateW, gateH, pitch, gateCx;
    float perfW, perfH, perfR, perfEdge, perfPhase, gateR, gateSoft, softK;
    int perfN, perfBoth, perfShape;
};
// 0 Regular 8, 1 Super 8, 2 16mm, 3 Super 16, 4 35 Academy, 5 Super 35 4p,
// 6 Super 35 3p, 7 35mm 2p, 8 65mm 5p.  Values and sources: docs/PRESETS.md
PS_INLINE PsdvFormat psdv_format(int f)
{
    PsdvFormat g;
    g.filmW = 7.976f; g.gateW = 5.69f; g.gateH = 4.22f; g.pitch = 4.234f; g.gateCx = 0.327f;
    g.perfW = 0.914f; g.perfH = 1.143f; g.perfR = 0.13f; g.perfEdge = 0.51f; g.perfPhase = 0.0f;
    g.gateR = 0.13f; g.gateSoft = 0.020f; g.softK = 1.5f; g.perfN = 1; g.perfBoth = 0; g.perfShape = 0;
    if (f == 0) {
        g.filmW = 7.975f; g.gateW = 4.90f; g.gateH = 3.60f; g.pitch = 3.810f; g.gateCx = 1.2425f;
        g.perfW = 1.83f; g.perfH = 1.27f; g.perfR = 0.25f; g.perfEdge = 0.90f; g.perfPhase = 0.5f;
        g.gateR = 0.25f; g.gateSoft = 0.015f;
    } else if (f == 2 || f == 3) {
        g.filmW = 16.0f; g.gateW = 10.26f; g.gateH = 7.49f; g.pitch = 7.605f; g.gateCx = 0.0f;
        g.perfW = 1.829f; g.perfH = 1.27f; g.perfR = 0.25f; g.perfEdge = 0.90f; g.perfPhase = 0.5f;
        g.gateR = 0.25f; g.softK = 1.2f; g.perfBoth = 1;
        if (f == 3) { g.gateW = 12.52f; g.gateH = 7.41f; g.gateCx = 1.13f; g.perfBoth = 0; }
    } else if (f >= 4 && f <= 7) {
        g.filmW = 35.0f; g.perfW = 2.794f; g.perfH = 1.854f; g.perfR = 0.127f; g.perfEdge = 2.01f;
        g.perfBoth = 1; g.perfShape = 1; g.gateR = 0.40f; g.gateSoft = 0.030f; g.softK = 1.0f;
        g.gateW = 24.89f; g.gateH = 18.67f; g.pitch = 18.96f; g.gateCx = 0.0f; g.perfN = 4; g.perfPhase = 0.5f;
        if (f == 4) { g.gateW = 22.00f; g.gateH = 16.00f; g.gateCx = 1.445f; }
        if (f == 6) { g.gateH = 13.87f; g.pitch = 14.22f; g.perfN = 3; g.perfPhase = 0.0f; }
        if (f == 7) { g.gateW = 22.00f; g.gateH = 9.47f; g.pitch = 9.48f; g.perfN = 2; g.gateCx = 1.445f; }
    } else if (f == 8) {
        g.filmW = 65.0f; g.gateW = 52.48f; g.gateH = 23.01f; g.pitch = 23.75f; g.gateCx = 0.0f;
        g.perfW = 2.794f; g.perfH = 1.981f; g.perfR = 0.50f; g.perfEdge = 2.85f; g.perfPhase = 0.0f;
        g.perfN = 5; g.perfBoth = 1; g.gateR = 0.50f; g.gateSoft = 0.040f; g.softK = 1.0f;
    }
    return g;
}

// =============================================================== shapes (signed distance, mm)
PS_INLINE float psdv_sd_roundrect(float x, float y, float w, float h, float r)
{
    float qx = ps_abs(x) - (w * 0.5f - r);
    float qy = ps_abs(y) - (h * 0.5f - r);
    float ox = ps_max(qx, 0.0f), oy = ps_max(qy, 0.0f);
    return ps_sqrt(ox * ox + oy * oy) + ps_min(ps_max(qx, qy), 0.0f) - r;
}
PS_INLINE float psdv_sd_bh(float x, float y, float w, float h)
{
    return ps_max(ps_sqrt(x * x + y * y) - w * 0.5f, ps_abs(y) - h * 0.5f);
}

// =============================================================== mechanics
// registration error of physical frame 'frame' in mm (x, y) and rotation (radians)
PS_INLINE ps3 psdv_registration(int frame, PS_CONSTANT_REF PsdvParams& p)
{
    float period = ps_max(p.weaveCycle, 1.0f);
    float t = (float)frame / period;
    int seed = 17 + p.weaveSeed * 7919;
    float a = 1.0f - ps_sat(p.weaveRandom), b = ps_sat(p.weaveRandom);
    float k = 3.4641f / ps_sqrt(a * a + b * b + 1e-6f);
    float sx = psdv_noiseT(t, 11, seed) - 0.5f, jx = psdv_rand(frame, 37, seed) - 0.5f;
    float sy = psdv_noiseT(t, 23, seed) - 0.5f, jy = psdv_rand(frame, 41, seed) - 0.5f;
    float sr = psdv_noiseT(t, 29, seed) - 0.5f, jr = psdv_rand(frame, 43, seed) - 0.5f;
    float x = (a * sx + b * jx) * k * p.weaveH * 0.001f;
    float y = (a * sy + b * jy) * k * p.weaveV * 0.001f;
    float r = (a * sr + b * jr) * k * p.rotation * 0.01745329f;
    return P3(x, y, r);
}
PS_INLINE float psdv_flicker(int frame, PS_CONSTANT_REF PsdvParams& p)
{
    int seed = 59 + p.weaveSeed * 7919;
    float a = 1.0f - ps_sat(p.flickerRandom), b = ps_sat(p.flickerRandom);
    float slow = psdv_noiseT((float)frame / 5.0f, 57, seed) - 0.5f;
    float fast = psdv_rand(frame, 61, seed) - 0.5f;
    return p.flicker * (a * slow + b * fast) * 3.4641f / ps_sqrt(a * a + b * b + 1e-6f);
}

// =============================================================== emulsion grain
// Dye-cloud field: sparse convolution of Gaussian clouds, one per jittered cell,
// sigma = 0.35 cell, unit variance (constant measured on 16M samples).
PS_INLINE float psdv_cloud(float px, float py, int seed)
{
    float fx = ps_floor(px), fy = ps_floor(py);
    int ix = (int)fx, iy = (int)fy;
    float acc = 0.0f;
    for (int dj = -1; dj <= 1; dj++) {
        for (int di = -1; di <= 1; di++) {
            ps3 c = psdv_cell(ix + di, iy + dj, seed);
            float dx = px - (fx + (float)di + c.x);
            float dy = py - (fy + (float)dj + c.y);
            acc += (c.z - 0.5f) * ps_exp(-(dx * dx + dy * dy) * 4.0816327f);
        }
    }
    return acc * 5.5843f;
}
PS_INLINE float psdv_erf(float x)
{
    float t = 1.0f / (1.0f + 0.3275911f * x);
    float y = ((((1.061405429f * t - 1.453152027f) * t + 1.421413741f) * t - 0.284496736f) * t + 0.254829592f) * t;
    return 1.0f - y * ps_exp(-x * x);
}
// variance left after averaging a unit Gaussian-correlated field (length l) over a square of side a
PS_INLINE float psdv_blockvar(float a, float l)
{
    float t = a / l;
    if (t < 0.001f) return 1.0f;
    float v = (1.7724539f * t * psdv_erf(t) + ps_exp(-t * t) - 1.0f) / (t * t);
    return v * v;
}
// characteristic curve used only as a metric for grain: (density, |dD/dlogE|)
PS_INLINE ps2 psdv_curve(float x, int reversal, float negGamma)
{
    x = ps_clamp(x, -4.0f, 4.0f);
    if (reversal) {
        float u = ps_pow(10.0f, 0.95f * (x + 0.455f));
        float inv = 1.0f / (1.0f + u);
        return P2(0.15f + 3.15f * inv, 3.15f * 0.95f * 2.3025851f * u * inv * inv);
    }
    float e = ps_pow(10.0f, ps_min(1.5f * (x + 1.5f), 30.0f));
    return P2(0.15f + negGamma * ps_log10(1.0f + e) / 1.5f, negGamma * e / (1.0f + e));
}
PS_INLINE float psdv_gshape(float d)
{
    float dc = ps_max(d, 0.12f);
    return ps_sqrt(dc * ps_exp(-(dc - 1.0f) / 2.5f));
}
PS_INLINE float psdv_layer(float fx, float fy, float c1, float c2, float k1, float k2, int seed)
{
    return k1 * psdv_cloud(fx / c1, fy / c1, seed) + k2 * psdv_cloud(fx / c2 + 0.371f, fy / c2 + 0.713f, seed + 1);
}
// E: linear exposure; (fx, fy): film mm of this frame; mmPerPx: film mm per pixel
PS_INLINE ps3 psdv_grain(ps3 E, float fx, float fy, int frame, float mmPerPx, PS_CONSTANT_REF PsdvParams& p)
{
    float sigRef = p.rms * 0.001f * p.grainIntensity;
    if (sigRef <= 0.0f) return E;
    int bw = p.emulsion >= 2 ? 1 : 0;
    int reversal = (p.emulsion == 1 || p.emulsion == 2) ? 1 : 0;
    float d1 = 0.0065f * p.grainSize, d2 = 0.015f * p.grainSize;     // dye cloud diameters (mm)
    float aRef = 0.0425397f;                                          // 48 um circle as a square
    float l1 = 0.5f * d1, l2 = 0.5f * d2;
    float k1 = 0.70710678f * ps_sqrt(psdv_blockvar(mmPerPx, l1) / psdv_blockvar(aRef, l1));
    float k2 = 0.70710678f * ps_sqrt(psdv_blockvar(mmPerPx, l2) / psdv_blockvar(aRef, l2));
    float c1 = ps_max(0.25f * d1, 0.5f * mmPerPx) / 0.35f;
    float c2 = ps_max(0.25f * d2, 0.5f * mmPerPx) / 0.35f;
    int seed = frame * 8 + p.grainSeed * 100003;
    float efl = 0.00036f;
    ps3 Ef = P3(ps_max(E.x, efl), ps_max(E.y, efl), ps_max(E.z, efl));
    float lum = 0.2126f * Ef.x + 0.7152f * Ef.y + 0.0722f * Ef.z;
    float xl = ps_log10(lum / 0.18f);
    float zS = ps_smooth(-0.4f, -1.6f, xl);
    float zH = ps_smooth(0.3f, 1.2f, xl);
    float trim = zS * p.shadowResponse + zH * p.highlightResponse + (1.0f - zS - zH) * p.midResponse;
    float gfloor = reversal ? 0.25f : 0.12f;
    float sgn = reversal ? -1.0f : 1.0f;
    ps3 dx;
    if (bw) {
        ps2 cv = psdv_curve(xl, reversal, p.negGamma);
        float n = psdv_layer(fx, fy, c1, c2, k1, k2, seed);
        float v = sgn * sigRef * trim * psdv_gshape(cv.x) * n / ps_max(cv.y, gfloor);
        dx = P3(v, v, v);
    } else {
        ps2 cr = psdv_curve(ps_log10(Ef.x / 0.18f), reversal, p.negGamma);
        ps2 cg = psdv_curve(ps_log10(Ef.y / 0.18f), reversal, p.negGamma);
        ps2 cb = psdv_curve(ps_log10(Ef.z / 0.18f), reversal, p.negGamma);
        float dC = 1.00f * psdv_gshape(cr.x) * psdv_layer(fx, fy, c1, c2, k1, k2, seed + 0);
        float dM = 0.90f * psdv_gshape(cg.x) * psdv_layer(fx, fy, c1, c2, k1, k2, seed + 2);
        float dY = 1.20f * psdv_gshape(cb.x) * psdv_layer(fx, fy, c1, c2, k1, k2, seed + 4);
        float dR = dC + 0.10f * dM + 0.02f * dY;
        float dG = 0.20f * dC + dM + 0.08f * dY;
        float dB = 0.05f * dC + 0.35f * dM + dY;
        float dMono = 0.25f * dR + 0.60f * dG + 0.15f * dB;
        float ind = ps_sat(p.colorGrain);
        dR = dMono + ind * (dR - dMono);
        dG = dMono + ind * (dG - dMono);
        dB = dMono + ind * (dB - dMono);
        float s = sigRef * trim * sgn;
        dx = P3(s * dR / ps_max(cr.y, gfloor), s * dG / ps_max(cg.y, gfloor), s * dB / ps_max(cb.y, gfloor));
    }
    dx = P3(ps_clamp(dx.x, -0.6f, 0.6f), ps_clamp(dx.y, -0.6f, 0.6f), ps_clamp(dx.z, -0.6f, 0.6f));
    return P3(E.x + Ef.x * (ps_pow(10.0f, dx.x) - 1.0f), E.y + Ef.y * (ps_pow(10.0f, dx.y) - 1.0f),
              E.z + Ef.z * (ps_pow(10.0f, dx.z) - 1.0f));
}

// =============================================================== dirt (opacity 0..1)
PS_INLINE float psdv_dust(float fx, float fy, int frame, float amount, float fibres, float frameArea, float px)
{
    float op = 0.0f;
    float norm = ps_sqrt(ps_max(frameArea / 24.0f, 1.0f));
    if (amount > 0.001f) {
        float cell = 0.30f;
        float pCell = ps_min(amount * 0.55f / norm * cell * cell, 0.9f);
        float gx = fx / cell, gy = fy / cell;
        float cxf = ps_floor(gx), cyf = ps_floor(gy);
        int ci = (int)cxf, cj = (int)cyf;
        if (psdv_rand(ci, cj, frame * 5 + 1) < pCell) {
            float ux = 0.2f + 0.6f * psdv_rand(ci, cj, frame * 5 + 2);
            float uy = 0.2f + 0.6f * psdv_rand(ci, cj, frame * 5 + 3);
            float rnd = psdv_rand(ci, cj, frame * 5 + 4);
            float rad = 0.010f + 0.045f * rnd * rnd * rnd;
            float dx = (gx - cxf - ux) * cell, dy = (gy - cyf - uy) * cell;
            float ang = ps_atan2(dy, dx);
            float wob = 1.0f + 0.30f * ps_sin(3.0f * ang + 6.2832f * rnd) + 0.15f * ps_sin(5.0f * ang + 2.0f);
            float d = ps_sqrt(dx * dx + dy * dy) - rad * wob;
            op = (0.55f + 0.45f * rnd) * (1.0f - ps_smooth(-px, px, d));
        }
    }
    if (fibres > 0.001f) {
        float fcell = 1.2f;
        int hi = (int)ps_floor(fx / fcell), hj = (int)ps_floor(fy / fcell);
        float pFib = ps_min(fibres * 0.12f / norm, 0.5f);
        for (int dj = -1; dj <= 1; dj++) {
            for (int di = -1; di <= 1; di++) {
                int a = hi + di, b = hj + dj;
                if (psdv_rand(a, b, frame * 5 + 7) < pFib) {
                    float ox = ((float)a + psdv_rand(a, b, frame * 5 + 8)) * fcell;
                    float oy = ((float)b + psdv_rand(a, b, frame * 5 + 9)) * fcell;
                    float R = 0.15f + 0.6f * psdv_rand(a, b, frame * 5 + 10);
                    float a0 = 6.2832f * psdv_rand(a, b, frame * 5 + 11);
                    float span = 0.6f + 1.6f * psdv_rand(a, b, frame * 5 + 12);
                    float th = 0.005f + 0.007f * psdv_rand(a, b, frame * 5 + 13);
                    float vx = fx - ox, vy = fy - oy;
                    float ang = ps_atan2(vy, vx) - a0;
                    ang = ang - 6.2832f * ps_floor(ang / 6.2832f);
                    float inArc = 1.0f - ps_smooth(span, span + 0.15f, ang);
                    float dist = ps_abs(ps_sqrt(vx * vx + vy * vy) - R) - th;
                    op = ps_max(op, inArc * (1.0f - ps_smooth(-px, px, dist)) * 0.85f);
                }
            }
        }
    }
    return op;
}
PS_INLINE float psdv_scratches(float fx, float yRoll, int frame, float amount, float gateW, float gateCx, float pitch, float px)
{
    if (amount <= 0.001f) return 0.0f;
    float op = 0.0f;
    for (int j = 0; j < 3; j++) {
        float len = 9.0f + 31.0f * psdv_rand(j, 3, 911);
        float off = 97.0f * psdv_rand(j, 5, 911);
        int run = (int)ps_floor(((float)frame + off) / len);
        if (psdv_rand(run, j, 913) < amount * 0.55f) {
            float base = (psdv_rand(run, j, 917) - 0.5f) * gateW * 0.9f + gateCx;
            float wander = 0.02f * gateW * (psdv_noiseT(-yRoll / (6.0f * pitch), j + 19, 919) - 0.5f);
            float w = 0.004f + 0.006f * psdv_rand(run, j, 921);
            float d = ps_abs(fx - base - wander) - w;
            float along = 0.35f + 0.65f * psdv_noise2(yRoll * 1.7f, (float)(run * 3 + j), 923);
            op = ps_max(op, along * (1.0f - ps_smooth(-px, px, d)));
        }
    }
    return op;
}
PS_INLINE float psdv_gatehair(float gx, float gy, int frame, float amount, float gateW, float gateH, float px)
{
    if (amount <= 0.001f) return 0.0f;
    float R = 0.55f * gateH;
    float cx = -0.5f * gateW - 0.45f * gateH;
    float cy = 0.18f * gateH + 0.004f * gateH * (psdv_rand(frame, 71, 3) - 0.5f);
    float vx = gx - cx, vy = gy - cy;
    float wig = 0.006f * gateH * ps_sin(ps_atan2(vy, vx) * 23.0f + 1.3f);
    float dist = ps_abs(ps_sqrt(vx * vx + vy * vy) - R + wig) - 0.006f - 0.004f * amount;
    float inArc = 1.0f - ps_smooth(0.40f, 0.55f, ps_abs(ps_atan2(vy, vx)));
    return ps_sat(amount) * inArc * (1.0f - ps_smooth(-px, px, dist));
}

// =============================================================== light leaks
PS_INLINE ps3 psdv_blend(ps3 E, ps3 L, int mode)
{
    if (mode == 1) return E + L;                                          // add
    if (mode == 2) return P3(ps_max(E.x, L.x), ps_max(E.y, L.y), ps_max(E.z, L.z));   // lighten
    if (mode == 3) {                                                      // soft light (relative)
        return P3(E.x * (1.0f + 2.0f * L.x) + 0.25f * L.x * L.x, E.y * (1.0f + 2.0f * L.y) + 0.25f * L.y * L.y,
                  E.z * (1.0f + 2.0f * L.z) + 0.25f * L.z * L.z);
    }
    return P3(E.x + L.x * (1.0f - ps_sat(E.x)), E.y + L.y * (1.0f - ps_sat(E.y)), E.z + L.z * (1.0f - ps_sat(E.z)));  // screen
}

// =============================================================== image sampling
// SRC must provide: int w, h; ps3 fetch(int k, int x, int y) const  (k: 0 prev, 1 current, 2 next)
template <class SRC>
PS_INLINE ps3 psdv_fetch_bilinear(PS_THREAD const SRC& s, int k, float sx, float sy, int enc)
{
    float xmax = (float)(s.w - 1), ymax = (float)(s.h - 1);
    sx = ps_clamp(sx, 0.0f, xmax);
    sy = ps_clamp(sy, 0.0f, ymax);
    float fx = ps_floor(sx), fy = ps_floor(sy);
    float ax = sx - fx, ay = sy - fy;
    int x0 = (int)fx, y0 = (int)fy;
    int x1 = (int)ps_min(fx + 1.0f, xmax), y1 = (int)ps_min(fy + 1.0f, ymax);
    ps3 a = psdv_decode(s.fetch(k, x0, y0), enc), b = psdv_decode(s.fetch(k, x1, y0), enc);
    ps3 c = psdv_decode(s.fetch(k, x0, y1), enc), d = psdv_decode(s.fetch(k, x1, y1), enc);
    return ps_mix3(ps_mix3(a, b, ax), ps_mix3(c, d, ax), ay);
}
template <class SRC>
PS_INLINE ps3 psdv_sample(PS_THREAD const SRC& s, int k, float su, float sv, float sgx, float sgy, int enc)
{
    float cx = su * (float)s.w - 0.5f;
    float cy = sv * (float)s.h - 0.5f;
    ps3 acc = psdv_fetch_bilinear(s, k, cx, cy, enc);
    if (sgx < 0.3f && sgy < 0.3f) return acc;
    float wsum = 1.0f;
    for (int ring = 1; ring <= 2; ring++) {
        float rr = (float)ring;
        float w = ps_exp(-0.5f * rr * rr);
        float rot = ring == 1 ? 0.0f : 0.5235988f;
        for (int j = 0; j < 6; j++) {
            float ang = rot + 1.0471976f * (float)j;
            acc = acc + psdv_fetch_bilinear(s, k, cx + ps_cos(ang) * rr * sgx, cy + ps_sin(ang) * rr * sgy, enc) * w;
            wsum += w;
        }
    }
    return acc * (1.0f / wsum);
}

// =============================================================== the pixel
template <class SRC>
PS_INLINE ps3 psdv_render_pixel(PS_THREAD const SRC& s, PS_CONSTANT_REF PsdvParams& p, int px, int py)
{
    PsdvFormat g = psdv_format(p.format);
    float W = (float)p.width, H = (float)p.height;
    float canvasAR = W / H;
    int T = p.frameIndex;
    int enc = p.encoding;
    int bw = (p.grainOn && p.emulsion >= 2) ? 1 : 0;
    float gateH = g.gateH;
    float gateW = p.aspect > 0.0f ? g.gateH * p.aspect : g.gateW;
    float radius = ps_min(g.gateR * ps_max(p.cornerRadius, 0.0f), 0.5f * ps_min(gateW, gateH));
    float pitch = ps_max(g.pitch + p.frameSpacing * gateH, gateH);
    int gateOn = p.gateOn;

    // ---- scale: film mm per output pixel
    float mmPerPx;
    if (!gateOn || p.gateFraming == 2) mmPerPx = ps_min(gateW / W, gateH / H);   // picture fills the screen
    else if (p.gateFraming == 1) mmPerPx = ps_max(gateW / W, gateH / H);          // whole gate visible
    else mmPerPx = gateH / (ps_max(p.frameSize, 0.05f) * H);                       // manual frame size
    mmPerPx = mmPerPx / ps_max(p.zoom, 0.01f);

    // ---- scanner jitter moves everything
    float jit = p.weaveOn ? p.scanJitter * 0.001f : 0.0f;
    float tjx = (psdv_rand(T, 401, 5 + p.weaveSeed) - 0.5f) * 3.4641f * jit;
    float tjy = (psdv_rand(T, 409, 5 + p.weaveSeed) - 0.5f) * 3.4641f * jit;
    float xs = ((float)px + 0.5f - 0.5f * W - p.posX * W) * mmPerPx - tjx;
    float yy = ((float)py + 0.5f - 0.5f * H - p.posY * H) * mmPerPx - tjy;
    float xx = xs + ((gateOn && p.centreOn == 0) || !gateOn ? g.gateCx : 0.0f);

    // ---- frame slot: 0 current, +1 above = previous, -1 below = next
    int slot = gateOn ? (int)ps_floor(yy / pitch + 0.5f) : 0;
    float yl = yy - (float)slot * pitch;
    int frame = T - slot;
    float yRoll = yl - (float)frame * pitch;

    // ---- registration (picture and gate shadow move together relative to the perforations)
    ps3 reg = p.weaveOn ? psdv_registration(frame, p) : P3(0.0f, 0.0f, 0.0f);
    float rx = xx - g.gateCx - reg.x, ry = yl - reg.y;
    float cr = ps_cos(reg.z), sr = ps_sin(reg.z);
    float gx = cr * rx + sr * ry, gy = -sr * rx + cr * ry;

    float pen = ps_max(p.edgeSoftness * g.gateSoft, 0.6f * mmPerPx);
    float edgeIrr = (psdv_noise2(gy * 3.0f, gx * 3.0f, 777) - 0.5f) * 0.012f * p.edgeSoftness;
    float dGate = gateOn ? psdv_sd_roundrect(gx + edgeIrr, gy - edgeIrr, gateW, gateH, radius) : -1.0e6f;
    float inside = gateOn ? 1.0f - ps_smooth(-pen, pen, dGate) : 1.0f;

    // ---- which picture is in this slot
    int k = 1;
    int have = 1;
    if (slot != 0) {
        if (p.neighbors == 0 || slot > 1 || slot < -1) have = 0;
        else if (p.neighbors == 1) k = slot > 0 ? (p.havePrev ? 0 : 1) : (p.haveNext ? 2 : 1);
    }

    ps3 E = P3(0.0f, 0.0f, 0.0f);
    float pic = 0.0f;
    if (have && inside > 0.0001f) {
        float nx = gx / gateW, ny = gy / gateH;
        float gateAR = gateW / gateH;
        float hS = 1.0f, vS = 1.0f;
        if (!gateOn) { hS = 1.0f; vS = 1.0f; }
        else if (p.imageFit == 1) { if (canvasAR > gateAR) vS = canvasAR / gateAR; else hS = gateAR / canvasAR; }
        else { if (canvasAR > gateAR) hS = gateAR / canvasAR; else vS = canvasAR / gateAR; }
        float iz = ps_max(p.imageZoom, 0.01f);
        hS = hS / iz; vS = vS / iz;
        float su = 0.5f + nx * hS + p.panX;
        float sv = 0.5f + ny * vS + p.panY;
        float srcPerMMy = vS * H / gateH, srcPerMMx = hS * W / gateW;
        if (!gateOn) {
            // no gate: the clip keeps the whole screen (1:1), weave and zoom still apply
            float spanX = W * mmPerPx, spanY = H * mmPerPx;
            su = 0.5f + gx / spanX / iz + p.panX;
            sv = 0.5f + gy / spanY / iz + p.panY;
            srcPerMMx = W / spanX / iz; srcPerMMy = H / spanY / iz;
        }
        float letter = (su < 0.0f || su > 1.0f || sv < 0.0f || sv > 1.0f) ? 0.0f : 1.0f;
        float blurMM = gateOn ? 0.0045f * g.softK * ps_max(p.opticalSoft, 0.0f) : 0.0f;   // gate section off = no gate softness
        float aaY = 0.5f * srcPerMMy * mmPerPx, aaX = 0.5f * srcPerMMx * mmPerPx;
        float sgy = ps_sqrt(blurMM * blurMM * srcPerMMy * srcPerMMy + (aaY > 0.5f ? aaY * aaY : 0.0f));
        float sgx = ps_sqrt(blurMM * blurMM * srcPerMMx * srcPerMMx + (aaX > 0.5f ? aaX * aaX : 0.0f));
        E = psdv_sample(s, k, ps_sat(su), ps_sat(sv), sgx, sgy, enc);
        if (bw) { float l = 0.2126f * E.x + 0.7152f * E.y + 0.0722f * E.z; E = P3(l, l, l); }
        if (p.weaveOn) E = E * ps_exp2(psdv_flicker(frame, p));
        // darkening along the gate edge (light fall-off at the aperture)
        if (gateOn && p.edgeDarkness > 0.0f) {
            float w = ps_max(p.edgeDarkWidth, 0.001f) * gateH;
            E = E * (1.0f - ps_sat(p.edgeDarkness) * ps_exp(ps_min(dGate, 0.0f) / w));
        }
        float hair = gateOn ? psdv_gatehair(gx, gy, frame, p.gateHair, gateW, gateH, mmPerPx) : 0.0f;
        E = E * (1.0f - 0.97f * hair);
        if (p.grainOn) E = psdv_grain(E, xx, yl, frame, mmPerPx, p);
        // dust and scratches are on the film
        float dust = gateOn ? psdv_dust(xx, yl, frame, p.dust, p.fibres, gateW * gateH, mmPerPx) : 0.0f;
        float scr = gateOn ? psdv_scratches(xx, yRoll, frame, p.scratches, gateW, g.gateCx, pitch, mmPerPx) : 0.0f;
        int reversal = (p.grainOn && (p.emulsion == 1 || p.emulsion == 2)) || !p.grainOn ? 1 : 0;
        int bright = p.dirtPolarity == 2 ? 1 : (p.dirtPolarity == 1 ? 0 : (reversal ? 0 : 1));
        if (bright) E = ps_mix3(E, P3(0.85f, 0.88f, 0.90f), dust);
        else E = E * (1.0f - 0.96f * dust);
        E = ps_mix3(E, reversal ? P3(0.55f, 0.75f, 0.95f) : P3(0.0f, 0.0f, 0.0f), scr * 0.8f);
        pic = inside * letter;
        E = E * pic;
    }

    // ---- perforations (always on the film, they do not weave with the picture)
    float hole = 0.0f, dPerf = 1.0e6f;
    if (gateOn && p.perfOn) {
        float sc = ps_max(p.perfScale, 0.05f);
        float pw = g.perfW * sc, ph = g.perfH * sc;
        float spp = pitch / (float)g.perfN;
        float rel = yy / spp - g.perfPhase;
        float hy = yy - (ps_floor(rel + 0.5f) + g.perfPhase) * spp;
        float pxL = -0.5f * g.filmW + g.perfEdge + 0.5f * g.perfW;
        int both = p.perfSide == 3 ? 1 : (p.perfSide == 0 ? g.perfBoth : 0);
        int left = p.perfSide == 2 ? 0 : 1;
        float xL = left ? xx - pxL : xx + pxL;
        float dA = g.perfShape == 1 ? psdv_sd_bh(xL, hy, pw, ph) : psdv_sd_roundrect(xL, hy, pw, ph, ps_min(g.perfR * sc, 0.5f * ps_min(pw, ph)));
        dPerf = dA;
        if (both) {
            float xR = xx + pxL;
            float dB = g.perfShape == 1 ? psdv_sd_bh(xR, hy, pw, ph) : psdv_sd_roundrect(xR, hy, pw, ph, ps_min(g.perfR * sc, 0.5f * ps_min(pw, ph)));
            dPerf = ps_min(dA, dB);
        }
        float aa = 0.6f * mmPerPx;
        hole = (1.0f - ps_smooth(-aa, aa, dPerf)) * ps_sat(p.perfOpacity);
        if (p.perfStyle == 0) E = E * (1.0f - hole);                                   // dark hole
        else if (p.perfStyle == 1) E = ps_mix3(E, P3(1.0f, 0.96f, 0.90f), hole);        // backlit
        else {                                                                           // outline
            float ring = (1.0f - ps_smooth(0.0f, 2.0f * aa, ps_abs(dPerf))) * ps_sat(p.perfOpacity);
            E = ps_mix3(E, P3(0.6f, 0.6f, 0.6f), ring);
        }
    }

    // ---- light leaks (light on the film: they also glow over the black areas)
    if (p.leakOn && p.leakOpacity > 0.0f) {
        int lseed = 300 + p.leakSeed * 131;
        float tt = (float)T * 0.08f * p.leakFrequency;
        float spread = ps_max(p.leakSpread, 0.05f);
        float L = 0.0f;
        if (gateOn && p.edgeGlow > 0.0f) {
            // thin saturated rim on the gate edge plus a faint wide halo over the black
            float w = 0.0030f * gateH * spread;
            float side = ps_abs(gx) / (0.5f * gateW);                 // a little stronger on the sides
            float flick = 0.75f + 0.25f * psdv_noiseT(tt * 3.0f + (float)frame * 0.37f, 3, lseed);
            float d = ps_abs(dGate);
            float rim = ps_exp(-d / w) + 0.18f * ps_exp(-d / (5.0f * w));
            if (dGate < 0.0f) rim *= 0.6f;                              // less inside the picture
            L += p.edgeGlow * rim * flick * (0.75f + 0.25f * side);
        }
        if (gateOn && p.perfOn && p.perfGlow > 0.0f) {
            // bright rim hugging the hole, quick fall-off, faint wide halo; dark inside
            float w = 0.030f * g.perfH * spread;
            float d = ps_abs(dPerf);
            float rim = ps_exp(-d / w) + 0.20f * ps_exp(-d / (5.0f * w));
            if (dPerf < 0.0f) rim = ps_exp(dPerf / (0.4f * w)) * 0.9f;
            float flick = 0.8f + 0.2f * psdv_noiseT(tt * 2.0f + (float)frame * 0.21f, 5, lseed);
            L += p.perfGlow * rim * flick;
        }
        if (p.flares > 0.0f) {
            for (int side = 0; side < 2; ++side) {                        // left and right edge, independent
                float sd = (float)side;
                float dEdge = 0.5f * gateW + (side == 0 ? gx : -gx);       // >0 inside the picture
                float act = ps_smooth(0.35f, 0.9f, psdv_noiseT(tt + sd * 13.0f, 7, lseed));
                float along = psdv_noise2(yRoll / gateH * 1.2f + sd * 5.0f, tt * 0.5f, lseed + 9);
                float fall = dEdge >= 0.0f ? ps_exp(-dEdge / (0.22f * gateW * spread))
                                           : ps_exp(dEdge / (0.06f * gateW * spread));
                L += p.flares * 2.0f * act * along * along * fall;
            }
        }
        if (p.burn > 0.0f) {
            float n = psdv_fbm(gx / gateH * 1.8f / spread + tt * 0.6f, yRoll / gateH * 1.8f / spread, lseed + 21);
            float b = ps_sat((n - 0.56f) * 4.0f);
            float edge = ps_exp(-ps_max(0.5f * gateW - ps_abs(gx), 0.0f) / (0.45f * gateW));
            L += p.burn * 3.0f * b * b * (0.35f + 0.65f * edge);
        }
        L = L * p.leakOpacity;
        ps3 tint = P3(ps_max(p.tintR, 0.0f), ps_max(p.tintG, 0.0f), ps_max(p.tintB, 0.0f));
        E = psdv_blend(E, tint * L, p.leakBlend);
    }
    return psdv_encode(E, enc);
}
