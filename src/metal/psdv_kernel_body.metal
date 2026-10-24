// psdv-film gate emulation - Metal kernel entry point.
// At build time this file is appended to psdv_prelude.h + psdv_params.h +
// psdv_core.h (includes stripped) and embedded as a string; Resolve's Metal
// device compiles it at first use.

struct PsdvMetalSource
{
    device const float4* a;   // previous frame
    device const float4* b;   // current frame
    device const float4* c;   // next frame
    int w;
    int h;
    ps3 fetch(int k, int x, int y) const
    {
        int i = y * w + x;
        float4 v = k == 0 ? a[i] : (k == 2 ? c[i] : b[i]);
        return float3(v.x, v.y, v.z);
    }
};

kernel void psdv_gate_kernel(constant PsdvParams& p [[buffer(0)]],
                             device float4* dst [[buffer(1)]],
                             device const float4* prev [[buffer(2)]],
                             device const float4* cur [[buffer(3)]],
                             device const float4* next [[buffer(4)]],
                             uint2 gid [[thread_position_in_grid]])
{
    if ((int)gid.x >= p.width || (int)gid.y >= p.height) return;
    PsdvMetalSource s;
    s.a = prev; s.b = cur; s.c = next; s.w = p.width; s.h = p.height;
    ps3 c = psdv_render_pixel(s, p, (int)gid.x, (int)gid.y);
    int i = (int)gid.y * p.width + (int)gid.x;
    dst[i] = float4(c.x, c.y, c.z, cur[i].w);
}
