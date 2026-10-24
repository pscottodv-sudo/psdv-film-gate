// psdv-film gate emulation - portable prelude.
// The same core is compiled as C++ (CPU render, tests) and as Metal Shading
// Language (GPU render). This header hides the few differences.
#pragma once

#if defined(__METAL_VERSION__)
  #include <metal_stdlib>
  using namespace metal;
  typedef float2 ps2;
  typedef float3 ps3;
  #define PS_INLINE inline
  #define PS_CONSTANT_REF constant
  #define PS_THREAD thread
  inline ps2 P2(float a, float b) { return float2(a, b); }
  inline ps3 P3(float a, float b, float c) { return float3(a, b, c); }
  inline float ps_floor(float x) { return floor(x); }
  inline float ps_ceil(float x) { return ceil(x); }
  inline float ps_sqrt(float x) { return sqrt(x); }
  inline float ps_exp(float x) { return exp(x); }
  inline float ps_exp2(float x) { return exp2(x); }
  inline float ps_log2(float x) { return log2(x); }
  inline float ps_log10(float x) { return log10(x); }
  inline float ps_pow(float x, float y) { return pow(x, y); }
  inline float ps_sin(float x) { return sin(x); }
  inline float ps_cos(float x) { return cos(x); }
  inline float ps_atan2(float y, float x) { return atan2(y, x); }
  inline float ps_abs(float x) { return fabs(x); }
  inline float ps_min(float a, float b) { return fmin(a, b); }
  inline float ps_max(float a, float b) { return fmax(a, b); }
#else
  #include <cmath>
  #include <cstdint>
  typedef unsigned int uint;
  struct ps2 { float x, y; };
  struct ps3 { float x, y, z; };
  #define PS_INLINE inline
  #define PS_CONSTANT_REF const
  #define PS_THREAD
  inline ps2 P2(float a, float b) { ps2 r; r.x = a; r.y = b; return r; }
  inline ps3 P3(float a, float b, float c) { ps3 r; r.x = a; r.y = b; r.z = c; return r; }
  inline ps3 operator+(ps3 a, ps3 b) { return P3(a.x + b.x, a.y + b.y, a.z + b.z); }
  inline ps3 operator-(ps3 a, ps3 b) { return P3(a.x - b.x, a.y - b.y, a.z - b.z); }
  inline ps3 operator*(ps3 a, ps3 b) { return P3(a.x * b.x, a.y * b.y, a.z * b.z); }
  inline ps3 operator*(ps3 a, float s) { return P3(a.x * s, a.y * s, a.z * s); }
  inline ps3 operator*(float s, ps3 a) { return P3(a.x * s, a.y * s, a.z * s); }
  inline float ps_floor(float x) { return std::floor(x); }
  inline float ps_ceil(float x) { return std::ceil(x); }
  inline float ps_sqrt(float x) { return std::sqrt(x); }
  inline float ps_exp(float x) { return std::exp(x); }
  inline float ps_exp2(float x) { return std::exp2(x); }
  inline float ps_log2(float x) { return std::log2(x); }
  inline float ps_log10(float x) { return std::log10(x); }
  inline float ps_pow(float x, float y) { return std::pow(x, y); }
  inline float ps_sin(float x) { return std::sin(x); }
  inline float ps_cos(float x) { return std::cos(x); }
  inline float ps_atan2(float y, float x) { return std::atan2(y, x); }
  inline float ps_abs(float x) { return std::fabs(x); }
  inline float ps_min(float a, float b) { return a < b ? a : b; }   // inlined (fmin is a library call)
  inline float ps_max(float a, float b) { return a > b ? a : b; }
#endif

PS_INLINE float ps_sat(float v) { return ps_min(ps_max(v, 0.0f), 1.0f); }
PS_INLINE float ps_clamp(float v, float a, float b) { return ps_min(ps_max(v, a), b); }
PS_INLINE float ps_lerp(float a, float b, float t) { return a + (b - a) * t; }
PS_INLINE ps3 ps_mix3(ps3 a, ps3 b, float t) { return a + (b - a) * t; }
PS_INLINE float ps_smooth(float e0, float e1, float v)
{
    float t = ps_sat((v - e0) / (e1 - e0));
    return t * t * (3.0f - 2.0f * t);
}
