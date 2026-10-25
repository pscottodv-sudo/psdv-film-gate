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
// Dye-cloud field (v2): sparse Gaussian clouds, one candidate per jittered cell,
// sigma = 0.35 cell. The cloud positions are shared by the three dye layers and
// each layer draws its own presence for every cloud, so the layers are
// statistically independent while the cost is paid once.
// Presence is a soft binary event with probability 'occ' (rising with density):
// thin areas show separate specks, dense areas a fine overlapping texture -
// the sparse, skewed look of real emulsion instead of symmetric digital noise.
// Returns the three layer fields, each zero mean and unit variance.
PS_INLINE ps3 psdv_clouds(float px, float py, int seed, ps3 occ)
{
    float fx = ps_floor(px), fy = ps_floor(py);
    int ix = (int)fx, iy = (int)fy;
    ps3 acc = P3(0.0f, 0.0f, 0.0f);
    for (int dj = -1; dj <= 1; dj++) {
        for (int di = -1; di <= 1; di++) {
            uint h = psdv_hash3(ix + di, iy + dj, seed);
            float cx = ((float)(h & 1023u) + 0.5f) * (1.0f / 1024.0f);
            float cy = ((float)((h >> 10) & 1023u) + 0.5f) * (1.0f / 1024.0f);
            float dx = px - (fx + (float)di + cx);
            float dy = py - (fy + (float)dj + cy);
            float w = ps_exp(-(dx * dx + dy * dy) * 4.0816327f);
            uint h2 = psdv_hash(h ^ 0x9E3779B9u);
            float u0 = ((float)(h2 & 1023u) + 0.5f) * (1.0f / 1024.0f);
            float u1 = ((float)((h2 >> 10) & 1023u) + 0.5f) * (1.0f / 1024.0f);
            float u2 = ((float)((h2 >> 20) & 1023u) + 0.5f) * (1.0f / 1024.0f);
            // presence minus its mean: zero-mean amplitudes independent of the
            // position keep the Gaussian spectrum (the aperture law) exact
            acc = acc + P3(ps_smooth(u0 - 0.06f, u0 + 0.06f, occ.x) - occ.x,
                           ps_smooth(u1 - 0.06f, u1 + 0.06f, occ.y) - occ.y,
                           ps_smooth(u2 - 0.06f, u2 + 0.06f, occ.z) - occ.z) * w;
        }
    }
    // unit variance: 5.5843 is the constant for amplitude variance 1/12; here the
    // amplitude variance is that of the soft presence, occ(1-occ) - 0.0158 (measured)
    return P3(acc.x * 1.61205f / ps_sqrt(occ.x * (1.0f - occ.x) - 0.0158f),
              acc.y * 1.61205f / ps_sqrt(occ.y * (1.0f - occ.y) - 0.0158f),
              acc.z * 1.61205f / ps_sqrt(occ.z * (1.0f - occ.z) - 0.0158f));
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
PS_INLINE float psdv_occ(float d) { return ps_clamp(0.18f + 0.14f * d, 0.18f, 0.55f); }
// E: linear exposure; (fx, fy): film mm of this frame; mmPerPx: film mm per pixel
PS_INLINE ps3 psdv_grain(ps3 E, float fx, float fy, int frame, float mmPerPx, PS_CONSTANT_REF PsdvParams& p)
{
    float sigRef = p.rms * 0.001f * p.grainIntensity;
    if (sigRef <= 0.0f) return E;
    int bw = p.emulsion >= 2 ? 1 : 0;
    int reversal = (p.emulsion == 1 || p.emulsion == 2) ? 1 : 0;
    float gs = ps_max(p.grainSize, 0.05f);
    // three structure scales (correlation length, mm): dye clouds, cloud groups, clumps
    float l1 = 0.0035f * gs, l2 = 0.0080f * gs, l3 = 0.0200f * gs;
    // share of the variance measured through the 48 um aperture
    float cl = ps_sat(p.grainClumping);
    float f3 = 0.15f + 0.55f * cl, f2 = 0.40f, f1 = ps_max(1.0f - f2 - f3, 0.05f);
    float aRef = 0.0425397f;                                          // 48 um circle as a square
    // Pixel area: the variance seen by one pixel is the block variance of the
    // physical field (Selwyn), and the rendered blobs are widened to the pixel
    // footprint (0.6 px) so the field is never point-sampled below its size -
    // sub-pixel blobs alias into salt-and-pepper speckle.
    // Scanner / print softness then widens every scale further and lowers its
    // variance by (l_pixel / l_soft)^2, like an optical blur of the grain.
    float pa = 0.6f * mmPerPx, sb = 1.0f * ps_max(p.grainSoftness, 0.0f) * mmPerPx;
    float q1 = ps_sqrt(l1 * l1 + pa * pa), q2 = ps_sqrt(l2 * l2 + pa * pa), q3 = ps_sqrt(l3 * l3 + pa * pa);
    float e1 = ps_sqrt(q1 * q1 + sb * sb), e2 = ps_sqrt(q2 * q2 + sb * sb), e3 = ps_sqrt(q3 * q3 + sb * sb);
    float k1 = ps_sqrt(f1 * psdv_blockvar(mmPerPx, l1) / psdv_blockvar(aRef, l1)) * (q1 / e1);
    float k2 = ps_sqrt(f2 * psdv_blockvar(mmPerPx, l2) / psdv_blockvar(aRef, l2)) * (q2 / e2);
    float k3 = ps_sqrt(f3 * psdv_blockvar(mmPerPx, l3) / psdv_blockvar(aRef, l3)) * (q3 / e3);
    // cell = length / 0.7 (sigma 0.35 cell -> correlation length 0.7 cell)
    float c1 = e1 / 0.7f, c2 = e2 / 0.7f, c3 = e3 / 0.7f;
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
        float o = psdv_occ(cv.x);
        ps3 oc = P3(o, o, o);
        ps3 dense = P3(0.5f, 0.5f, 0.5f);
        float n = k1 * psdv_clouds(fx / c1, fy / c1, seed, dense).x + k2 * psdv_clouds(fx / c2 + 0.371f, fy / c2 + 0.713f, seed + 1, dense).x
                + k3 * psdv_clouds(fx / c3 + 0.193f, fy / c3 + 0.557f, seed + 2, oc).x;
        float v = sgn * sigRef * trim * psdv_gshape(cv.x) * n / ps_max(cv.y, gfloor);
        dx = P3(v, v, v);
    } else {
        ps2 cr = psdv_curve(ps_log10(Ef.x / 0.18f), reversal, p.negGamma);
        ps2 cg = psdv_curve(ps_log10(Ef.y / 0.18f), reversal, p.negGamma);
        ps2 cb = psdv_curve(ps_log10(Ef.z / 0.18f), reversal, p.negGamma);
        ps3 oc = P3(psdv_occ(cr.x), psdv_occ(cg.x), psdv_occ(cb.x));
        ps3 dense = P3(0.5f, 0.5f, 0.5f);   // dye clouds and groups: dense, symmetric
        ps3 n1 = psdv_clouds(fx / c1, fy / c1, seed, dense);
        ps3 n2 = psdv_clouds(fx / c2 + 0.371f, fy / c2 + 0.713f, seed + 1, dense);
        ps3 n3 = psdv_clouds(fx / c3 + 0.193f, fy / c3 + 0.557f, seed + 2, oc);
        // The finest scale is shared by the three layers: at the scale of a pixel the
        // scanner and the overlap of the dye spectra leave mostly luminance texture,
        // and independent RGB speckle at that scale is what reads as digital noise.
        // Colour variation lives in the cloud groups and clumps.
        // The yellow (blue-sensitive) layer is the fastest and coarsest.
        float dC = 1.00f * psdv_gshape(cr.x) * (k1 * n1.x + k2 * n2.x + k3 * n3.x);
        float dM = 0.90f * psdv_gshape(cg.x) * (k1 * n1.x + k2 * n2.y + k3 * n3.y);
        float dY = 1.20f * psdv_gshape(cb.x) * (0.80f * k1 * n1.x + k2 * n2.z + 1.25f * k3 * n3.z);
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

// =============================================================== image sampling
// SRC must provide: int w, h; ps3 fetch(int k, int x, int y) const  (k: 0 prev, 1 current, 2 next)
// Filtering happens on the encoded values and the result is decoded once:
// 1 decode per pixel instead of one per texel (the dominant cost before).
template <class SRC>
PS_INLINE ps3 psdv_fetch_bilinear(PS_THREAD const SRC& s, int k, float sx, float sy)
{
    float xmax = (float)(s.w - 1), ymax = (float)(s.h - 1);
    sx = ps_clamp(sx, 0.0f, xmax);
    sy = ps_clamp(sy, 0.0f, ymax);
    float fx = ps_floor(sx), fy = ps_floor(sy);
    float ax = sx - fx, ay = sy - fy;
    int x0 = (int)fx, y0 = (int)fy;
    int x1 = (int)ps_min(fx + 1.0f, xmax), y1 = (int)ps_min(fy + 1.0f, ymax);
    ps3 a = s.fetch(k, x0, y0), b = s.fetch(k, x1, y0);
    ps3 c = s.fetch(k, x0, y1), d = s.fetch(k, x1, y1);
    return ps_mix3(ps_mix3(a, b, ax), ps_mix3(c, d, ax), ay);
}
// Gaussian-weighted sampling: 1 tap when sharp, 7 taps (centre + hexagon) for a
// blur up to ~1.2 px, 13 taps (two rings) above that.
template <class SRC>
PS_INLINE ps3 psdv_sample(PS_THREAD const SRC& s, int k, float su, float sv, float sgx, float sgy, int enc)
{
    float cx = su * (float)s.w - 0.5f;
    float cy = sv * (float)s.h - 0.5f;
    ps3 acc = psdv_fetch_bilinear(s, k, cx, cy);
    float sg = ps_max(sgx, sgy);
    if (sg < 0.3f) return psdv_decode(acc, enc);
    float wsum = 1.0f;
    int rings = sg < 1.2f ? 1 : 2;
    for (int ring = 1; ring <= rings; ring++) {
        float rr = rings == 1 ? 1.15f : (float)ring;
        float w = rings == 1 ? 0.52f : ps_exp(-0.5f * rr * rr);
        float rot = ring == 1 ? 0.0f : 0.5235988f;
        for (int j = 0; j < 6; j++) {
            float ang = rot + 1.0471976f * (float)j;
            acc = acc + psdv_fetch_bilinear(s, k, cx + ps_cos(ang) * rr * sgx, cy + ps_sin(ang) * rr * sgy) * w;
            wsum += w;
        }
    }
    return psdv_decode(acc * (1.0f / wsum), enc);
}

// Gate Defocus 1.0 = blur sigma of 1.5% of the gate height
#define PSDV_DEFOCUS_MM 0.015f

// Film millimetres per output pixel for the chosen framing. "Fit Gate To Screen"
// keeps the whole gate visible including its soft / defocused / irregular edge.
PS_INLINE float psdv_mm_per_px(PS_CONSTANT_REF PsdvParams& p, PsdvFormat g, float gateW, float gateH, float W, float H)
{
    float m;
    if (!p.gateOn || p.gateFraming == 2) m = ps_min(gateW / W, gateH / H);         // picture fills the screen
    else if (p.gateFraming == 1) {                                                  // whole gate visible
        float pen = p.edgeSoftness * g.gateSoft;
        float defo = ps_max(p.gateDefocus, 0.0f) * PSDV_DEFOCUS_MM * gateH;
        float edge = 2.5f * ps_sqrt(0.25f * pen * pen + defo * defo) + 1.5f * ps_max(p.edgeIrregularity, 0.0f) * 0.001f;
        m = ps_max((gateW + 2.0f * edge) / W, (gateH + 2.0f * edge) / H);
    }
    else m = gateH / (ps_max(p.frameSize, 0.05f) * H);                               // manual frame size
    return m / ps_max(p.zoom, 0.01f);
}

// One frame slot of the strip: gate shadow + picture (+ grain, dirt) of frame T - slot.
// Returns the picture already multiplied by the gate coverage; dGateOut = signed
// distance to that slot's gate outline (mm, < 0 inside).
template <class SRC>
PS_INLINE ps3 psdv_slot(PS_THREAD const SRC& s, PS_CONSTANT_REF PsdvParams& p, PsdvFormat g, int slot, float xx, float yy,
                        float mmPerPx, float pitch, float gateW, float gateH, float radius, float canvasAR, int bw,
                        PS_THREAD float& dGateOut, PS_THREAD float& gxOut)
{
    int T = p.frameIndex;
    int enc = p.encoding;
    float W = (float)p.width, H = (float)p.height;
    int gateOn = p.gateOn;
    float yl = yy - (float)slot * pitch;
    int frame = T - slot;
    float yRoll = yl - (float)frame * pitch;

    // ---- registration (picture and gate shadow move together relative to the perforations)
    ps3 reg = P3(0.0f, 0.0f, 0.0f);
    if (p.weaveOn) {
        if (slot >= -1 && slot <= 1) reg = P3(p.regX[slot + 1], p.regY[slot + 1], p.regR[slot + 1]);   // precomputed
        else reg = psdv_registration(frame, p);
    }
    float rx = xx - g.gateCx - reg.x, ry = yl - reg.y;
    float cr = ps_cos(reg.z), sr = ps_sin(reg.z);
    float gx = cr * rx + sr * ry, gy = -sr * rx + cr * ry;

    float pen = ps_max(p.edgeSoftness * g.gateSoft, 0.6f * mmPerPx);
    float defo = ps_max(p.gateDefocus, 0.0f) * PSDV_DEFOCUS_MM * gateH;   // defocus blur (sigma, mm)
    float dGate = gateOn ? psdv_sd_roundrect(gx, gy, gateW, gateH, radius) : -1.0e6f;
    // irregular outline: small bumps in and out along the edge (filed gate, emulsion
    // build-up, dirt). The fixed part belongs to the gate, the moving part changes
    // from frame to frame. Only evaluated near the edge.
    float irrA = ps_max(p.edgeIrregularity, 0.0f) * 0.001f;
    if (gateOn && irrA > 0.0f && ps_abs(dGate) < 3.0f * irrA + 4.0f * (pen + defo) + 0.05f) {
        float sc = 0.22f * ps_max(p.edgeIrrSize, 0.05f);
        float u = gx / sc, v = gy / sc;
        float st = psdv_noise2(u, v, 777) + 0.5f * psdv_noise2(u * 2.3f + 5.1f, v * 2.3f - 1.7f, 778);
        int fs = 991 + frame * 13;
        float mv = psdv_noise2(u, v, fs) + 0.5f * psdv_noise2(u * 2.3f + 5.1f, v * 2.3f - 1.7f, fs + 1);
        float n = (ps_lerp(st, mv, ps_sat(p.edgeIrrMotion)) - 0.75f) * 1.6f;   // about -1..1
        dGate = dGate + n * irrA;                                             // + = edge moves inward
    }
    float inside = 1.0f;
    if (gateOn) {
        // Gaussian edge profile (long tail): with a compact smoothstep the fade
        // ended abruptly, which shows as a hard line once the image is log encoded.
        float sgE = ps_sqrt(0.25f * pen * pen + defo * defo);
        float z = dGate * 0.70710678f / sgE;
        inside = 0.5f - 0.5f * (z >= 0.0f ? psdv_erf(z) : -psdv_erf(-z));
        if (defo > 0.0f) {
            // Gaussian blur of a rectangle is separable: the product of the two edge
            // profiles darkens and rounds the corners like a real defocused aperture.
            float k = 0.70710678f / defo;
            float ax = (0.5f * gateW - ps_abs(gx)) * k, ay = (0.5f * gateH - ps_abs(gy)) * k;
            float px1 = 0.5f + 0.5f * (ax >= 0.0f ? psdv_erf(ax) : -psdv_erf(-ax));
            float py1 = 0.5f + 0.5f * (ay >= 0.0f ? psdv_erf(ay) : -psdv_erf(-ay));
            inside = inside * ps_max(px1, py1);
        }
        // Perceptual fade: the scan is viewed in a log/gamma encoding, where a linear
        // Gaussian fade looks like the gate growing outwards. Cubing the coverage keeps
        // the visible transition centred on the gate outline.
        inside = inside * inside * inside;
    }

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
        // Fill Gate: the clip covers the gate exactly; its border pixels are extended so
        // the soft / defocused / irregular gate edge fades the picture instead of the
        // picture being cut sharply where the clip ends.
        if (gateOn && p.imageFit == 0 && p.imageZoom >= 1.0f) letter = 1.0f;
        float blurMM = gateOn ? 0.0045f * g.softK * ps_max(p.opticalSoft, 0.0f) : 0.0f;   // gate section off = no gate softness
        float aaY = 0.5f * srcPerMMy * mmPerPx, aaX = 0.5f * srcPerMMx * mmPerPx;
        float sgy = ps_sqrt(blurMM * blurMM * srcPerMMy * srcPerMMy + (aaY > 0.5f ? aaY * aaY : 0.0f));
        float sgx = ps_sqrt(blurMM * blurMM * srcPerMMx * srcPerMMx + (aaX > 0.5f ? aaX * aaX : 0.0f));
        // beyond the clip border (under a soft gate edge) the border pixels are
        // extended; blur them across the stretch direction so they do not streak
        float ox = ps_max(ps_max(-su, su - 1.0f), 0.0f) * W, oy = ps_max(ps_max(-sv, sv - 1.0f), 0.0f) * H;
        sgx = sgx + 0.5f * oy;
        sgy = sgy + 0.5f * ox;
        E = psdv_sample(s, k, ps_sat(su), ps_sat(sv), sgx, sgy, enc);
        if (bw) { float l = 0.2126f * E.x + 0.7152f * E.y + 0.0722f * E.z; E = P3(l, l, l); }
        if (p.weaveOn) E = E * ps_exp2(slot >= -1 && slot <= 1 ? p.flick[slot + 1] : psdv_flicker(frame, p));
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
    dGateOut = dGate;
    gxOut = gx;
    return E;

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
    float mmPerPx = psdv_mm_per_px(p, g, gateW, gateH, W, H);

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
    float dGate, gx;
    ps3 E = psdv_slot(s, p, g, slot, xx, yy, mmPerPx, pitch, gateW, gateH, radius, canvasAR, bw, dGate, gx);
    // a soft / defocused / irregular gate edge can reach across the frame line:
    // add the neighbouring slot when this pixel is within reach of it
    if (gateOn) {
        float pen = ps_max(p.edgeSoftness * g.gateSoft, 0.6f * mmPerPx);
        float defo = ps_max(p.gateDefocus, 0.0f) * PSDV_DEFOCUS_MM * gateH;
        float reach = 4.0f * ps_sqrt(0.25f * pen * pen + defo * defo) + 3.0f * ps_max(p.edgeIrregularity, 0.0f) * 0.001f
                    + (p.weaveOn ? 0.004f * (p.weaveV + p.scanJitter) : 0.0f);
        // distance from this pixel to the outline of the adjacent slot's gate
        if (pitch - ps_abs(yl) - 0.5f * gateH < reach) {
            float d2, gx2;
            int s2 = slot + (yl > 0.0f ? 1 : -1);
            E = E + psdv_slot(s, p, g, s2, xx, yy, mmPerPx, pitch, gateW, gateH, radius, canvasAR, bw, d2, gx2);
            dGate = ps_min(dGate, d2);
        }
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

        // ---- perforation light: the scanner backlight pours through the hole and
        // halates into the emulsion and base around it. In a negative scan the hole
        // itself is black and the edge glows: white core, orange glow, red halo.
        // It is never static: intensity and the hot side change from frame to frame.
        if (p.leakOn) {
            int useB = (both && dPerf < dA) ? 1 : 0;
            float rx = useB ? xx + pxL : xL;                      // position relative to the hole centre
            float rxs = useB ? -rx : rx;                          // mirrored for the right-hand hole
            float d = dPerf;
            float holeId = ps_floor(rel + 0.5f) * 2.0f + (float)useB;
            int ls = 300 + p.leakSeed * 131;
            float rimW = 0.0045f * ph * ps_max(p.perfRim, 0.05f);
            float hal = ps_max(p.perfHalo, 0.05f);
            float dout = ps_max(d, 0.0f), din = ps_max(-d, 0.0f);
            // Gaussian fall-offs: the light must die out within a fraction of the hole
            // height, otherwise (log-encoded) it washes over the whole film
            float inner = ps_exp(-din / (0.6f * rimW));         // quick fall-off into the hole
            float c1 = dout / rimW, g1 = dout / (0.016f * ph * hal), h1 = dout / (0.05f * ph * hal);
            float core = ps_exp(-c1 * c1) * inner;
            float glow = ps_exp(-g1 * g1) * inner;
            float halo = ps_exp(-h1 * h1) * inner;
            // variation over time: slow drift mixed with frame-to-frame changes
            float sp = ps_sat(p.perfSpeed), va = ps_sat(p.perfVariation);
            float tSlow = (float)T / (1.0f + 23.0f * (1.0f - sp));
            float n1 = psdv_noiseT(tSlow + holeId * 0.37f, 11, ls);
            float n2 = psdv_rand(T, (int)holeId + 1000, ls + 3);
            float inten = 1.0f + va * 0.9f * (ps_lerp(n1, n2, sp) - 0.5f) * 2.0f;
            // the hot side wanders around the hole
            float ang = ps_atan2(hy / ph, rxs / pw);
            float ph0 = 6.2831853f * (psdv_noiseT(tSlow * 0.5f + holeId * 0.11f, 13, ls) + va * 0.5f * n2);
            float side = 0.5f + 0.5f * ps_cos(ang - ph0);
            float ca = ps_cos(ang), sa = ps_sin(ang);                 // noise on a circle: no seam at +-pi
            side = side * (0.75f + 0.5f * psdv_noise2(ca * 0.6f + 7.0f + holeId, sa * 0.6f + tSlow * 0.7f, ls + 5));
            float asym = ps_sat(p.perfAsym);
            float sideK = ps_lerp(1.0f, 0.03f + 2.2f * side * side, asym);
            // organic unevenness along the rim (changes with the frame like the rest)
            sideK = sideK * (0.65f + 0.7f * psdv_noise2(ca * 3.5f + 3.0f + holeId * 2.0f, sa * 3.5f + tSlow * 1.3f, ls + 7));
            float k = ps_exp2(p.perfLightEV) * ps_max(inten, 0.0f) * sideK;
            ps3 coreC = P3(ps_max(p.coreR, 0.0f), ps_max(p.coreG, 0.0f), ps_max(p.coreB, 0.0f));
            ps3 glowC = P3(ps_max(p.glowR, 0.0f), ps_max(p.glowG, 0.0f), ps_max(p.glowB, 0.0f));
            ps3 outC = P3(ps_max(p.outerR, 0.0f), ps_max(p.outerG, 0.0f), ps_max(p.outerB, 0.0f));
            ps3 Lp = (coreC * (0.40f * core) + glowC * (0.32f * glow) + outC * (0.025f * halo)) * k;
            // cool chromatic fringe just outside the rim, on the dim side
            if (p.perfFringe > 0.0f) {
                float fz = (dout - 1.6f * rimW) / (0.9f * rimW);
                float fr = ps_exp(-fz * fz) * inner * (1.0f - side);
                Lp = Lp + P3(0.20f, 0.42f, 1.0f) * (0.35f * p.perfFringe * fr * ps_exp2(p.perfLightEV));
            }
            E = E + Lp;
        }
    }
    return psdv_encode(E, enc);
}

#if !defined(__METAL_VERSION__)
// Host side: is the previous (above) / next (below) frame visible on screen?
// Used to avoid asking the host for t-1 / t+1 when they would not be seen -
// every extra frame costs the host a full decode + upstream render.
inline void psdv_neighbors_visible(const PsdvParams& p, bool& above, bool& below)
{
    above = below = false;
    if (!p.gateOn || p.neighbors == 0) return;
    PsdvFormat g = psdv_format(p.format);
    float W = (float)p.width, H = (float)p.height;
    float gateH = g.gateH;
    float gateW = p.aspect > 0.0f ? g.gateH * p.aspect : g.gateW;
    float pitch = ps_max(g.pitch + p.frameSpacing * gateH, gateH);
    float mmPerPx = psdv_mm_per_px(p, g, gateW, gateH, W, H);
    // weave, jitter and the soft gate edge can pull a neighbour in: generous margin
    float margin = 0.004f * (p.weaveOn ? (p.weaveV + p.scanJitter + 1.0f) : 1.0f) + 0.05f * gateH;
    float yTop = (0.5f * H - p.posY * H) * mmPerPx + margin;
    float yBot = (-0.5f * H - p.posY * H) * mmPerPx - margin;
    // the neighbour picture starts half a pitch away from the centre minus half the gate
    above = yTop > pitch - 0.5f * gateH;
    below = yBot < -(pitch - 0.5f * gateH);
}

// Host side, once per frame: values that depend only on the frame number.
inline void psdv_prepare(PsdvParams& p)
{
    for (int i = 0; i < 3; ++i) {
        int frame = p.frameIndex - (i - 1);          // index = slot + 1, frame = T - slot
        ps3 r = psdv_registration(frame, p);
        p.regX[i] = r.x; p.regY[i] = r.y; p.regR[i] = r.z;
        p.flick[i] = psdv_flicker(frame, p);
    }
}
#endif
