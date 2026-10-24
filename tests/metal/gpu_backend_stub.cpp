// Non-Apple stand-in for the Metal backend: renders with the CPU core so the
// comparison harness itself can be tested on Linux. Says nothing about Metal.
#include <chrono>
#include <fstream>
#include "core/psdv_core.h"
#include "gpu_backend.h"

namespace {
struct Src
{
    const float* img[3];
    int w, h;
    ps3 fetch(int k, int x, int y) const
    {
        const float* q = img[k] + ((size_t)y * (size_t)w + (size_t)x) * 4;
        return P3(q[0], q[1], q[2]);
    }
};
}

bool gpuInit(std::string& name, std::string&) { name = "CPU stub (not a GPU)"; return true; }
void gpuDumpKernel(const std::string& path) { std::ofstream(path) << "// stub backend: no Metal kernel\n"; }
bool gpuCompile(double& ms, std::string&) { ms = 0; return true; }
bool gpuRender(const PsdvParams& p, const float* prev, const float* cur, const float* next, float* out, double& ms, std::string&)
{
    auto t0 = std::chrono::steady_clock::now();
    Src s; s.img[0] = prev; s.img[1] = cur; s.img[2] = next; s.w = p.width; s.h = p.height;
    for (int y = 0; y < p.height; ++y)
        for (int x = 0; x < p.width; ++x) {
            ps3 c = psdv_render_pixel(s, p, x, y);
            size_t i = ((size_t)y * p.width + x) * 4;
            out[i] = c.x; out[i + 1] = c.y; out[i + 2] = c.z; out[i + 3] = cur[i + 3];
        }
    ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return true;
}
