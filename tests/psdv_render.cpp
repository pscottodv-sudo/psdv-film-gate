// psdv_render - CPU reference renderer of the plugin core (tests and previews).
// Usage: psdv_render in.f32 out.f32 W H [prev.f32] [next.f32] [name=value ...]
// Images: float32 RGBA, row 0 = bottom (OFX convention). Parameters use the
// PsdvParams member names; defaults equal the plugin defaults.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "core/psdv_core.h"
#include "psdv_test_params.h"

struct Src
{
    const float* img[3];
    int w, h;
    ps3 fetch(int k, int x, int y) const
    {
        const float* p = img[k] + ((size_t)y * (size_t)w + (size_t)x) * 4;
        return P3(p[0], p[1], p[2]);
    }
};

static bool readImg(const char* path, std::vector<float>& v, size_t n)
{
    v.resize(n);
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    bool ok = fread(v.data(), sizeof(float), n, f) == n;
    fclose(f);
    return ok;
}

int main(int argc, char** argv)
{
    if (argc < 5) { fprintf(stderr, "usage: psdv_render in out W H [prev=] [next=] [k=v...]\n"); return 2; }
    int w = atoi(argv[3]), h = atoi(argv[4]);
    size_t n = (size_t)w * (size_t)h * 4;
    std::vector<float> cur, prev, next, out(n);
    if (!readImg(argv[1], cur, n)) { fprintf(stderr, "bad input\n"); return 3; }
    PsdvParams p;
    psdvDefaults(p);
    p.width = w; p.height = h;
    for (int i = 5; i < argc; ++i) {
        std::string a = argv[i];
        size_t eq = a.find('=');
        if (eq == std::string::npos) continue;
        std::string k = a.substr(0, eq), val = a.substr(eq + 1);
        if (k == "prev") { if (!readImg(val.c_str(), prev, n)) return 3; p.havePrev = 1; continue; }
        if (k == "next") { if (!readImg(val.c_str(), next, n)) return 3; p.haveNext = 1; continue; }
        if (!psdvSetParam(p, k, atof(val.c_str()))) { fprintf(stderr, "unknown parameter %s\n", k.c_str()); return 4; }
    }
    Src s;
    s.img[0] = p.havePrev ? prev.data() : cur.data();
    s.img[1] = cur.data();
    s.img[2] = p.haveNext ? next.data() : cur.data();
    s.w = w; s.h = h;
    long bad = 0;
#pragma omp parallel for schedule(dynamic, 8) reduction(+ : bad)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            ps3 c = psdv_render_pixel(s, p, x, y);
            size_t i = ((size_t)y * (size_t)w + (size_t)x) * 4;
            if (!std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.z)) bad++;
            out[i] = c.x; out[i + 1] = c.y; out[i + 2] = c.z; out[i + 3] = cur[i + 3];
        }
    }
    FILE* f = fopen(argv[2], "wb");
    fwrite(out.data(), sizeof(float), n, f);
    fclose(f);
    fprintf(stderr, "nonfinite=%ld\n", bad);
    return bad ? 5 : 0;
}
