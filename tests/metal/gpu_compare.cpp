// psdv_gpu_compare - renders the same frames with the GPU backend (Metal on macOS)
// and with the CPU core, then compares them pixel by pixel.
// Usage: psdv_gpu_compare OUT_DIR [--quick]
// Writes OUT_DIR/report.md, OUT_DIR/results.json, OUT_DIR/<case>_gpu.png,
// OUT_DIR/<case>_diff.png (|GPU-CPU| x 50) and OUT_DIR/psdv_kernel.metal.
// Exit code: 0 all cases pass, 1 a case fails, 2 no GPU / kernel did not compile.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "core/psdv_core.h"
#include "psdv_test_params.h"
#include "gpu_backend.h"

namespace {

// ------------------------------------------------------------------ test scene
// Smooth exposure ramp + colour patches + fine detail. Each frame is shifted so
// "previous", "current" and "next" are distinguishable.
void makeFrame(std::vector<float>& img, int w, int h, int frame)
{
    img.assign((size_t)w * h * 4, 1.0f);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float u = (float)x / (float)(w - 1), v = (float)y / (float)(h - 1);
            float ramp = 0.08f + 0.80f * u;                                   // DI-encoded ramp
            float patch = ((int)(u * 8 + frame) + (int)(v * 5)) % 3 == 0 ? 0.15f : 0.0f;
            float detail = 0.04f * std::sin(x * 0.9f + frame) * std::sin(y * 0.7f);
            float r = ramp + patch + detail;
            float g = ramp * 0.92f + 0.05f * v + detail;
            float b = ramp * (0.7f + 0.3f * v) + (frame % 3) * 0.03f;
            float* p = &img[((size_t)y * w + x) * 4];
            p[0] = r; p[1] = g; p[2] = b; p[3] = 1.0f;
        }
    }
}

// ------------------------------------------------------------------ CPU render
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

void cpuRender(const PsdvParams& p, const float* prev, const float* cur, const float* next, float* out)
{
    Src s; s.img[0] = prev; s.img[1] = cur; s.img[2] = next; s.w = p.width; s.h = p.height;
    unsigned n = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < n; ++t) {
        pool.emplace_back([&, t] {
            for (int y = (int)t; y < p.height; y += (int)n) {
                for (int x = 0; x < p.width; ++x) {
                    ps3 c = psdv_render_pixel(s, p, x, y);
                    size_t i = ((size_t)y * p.width + x) * 4;
                    out[i] = c.x; out[i + 1] = c.y; out[i + 2] = c.z; out[i + 3] = cur[i + 3];
                }
            }
        });
    }
    for (auto& th : pool) th.join();
}

// ------------------------------------------------------------------ minimal PNG writer (stored deflate)
uint32_t crc32(const uint8_t* d, size_t n, uint32_t c = 0)
{
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t v = i;
            for (int k = 0; k < 8; ++k) v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
            table[i] = v;
        }
        init = true;
    }
    c ^= 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) c = table[(c ^ d[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}
void put32(std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back(x >> 24); v.push_back(x >> 16); v.push_back(x >> 8); v.push_back(x);
}
void chunk(FILE* f, const char* type, const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> b;
    put32(b, (uint32_t)data.size());
    b.insert(b.end(), type, type + 4);
    b.insert(b.end(), data.begin(), data.end());
    uint32_t c = crc32(b.data() + 4, b.size() - 4);
    put32(b, c);
    fwrite(b.data(), 1, b.size(), f);
}
// rgb: 8-bit, top row first
void writePng(const std::string& path, const std::vector<uint8_t>& rgb, int w, int h)
{
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return;
    const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, f);
    std::vector<uint8_t> ihdr;
    put32(ihdr, w); put32(ihdr, h);
    ihdr.push_back(8); ihdr.push_back(2); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
    chunk(f, "IHDR", ihdr);
    std::vector<uint8_t> raw;
    raw.reserve((size_t)(w * 3 + 1) * h);
    for (int y = 0; y < h; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb.begin() + (size_t)y * w * 3, rgb.begin() + (size_t)(y + 1) * w * 3);
    }
    std::vector<uint8_t> z = {0x78, 0x01};
    uint32_t a = 1, b = 0;
    for (uint8_t c : raw) { a = (a + c) % 65521; b = (b + a) % 65521; }
    for (size_t pos = 0; pos < raw.size(); pos += 65535) {
        size_t len = std::min<size_t>(65535, raw.size() - pos);
        z.push_back(pos + len == raw.size() ? 1 : 0);
        z.push_back(len & 0xFF); z.push_back(len >> 8);
        z.push_back(~len & 0xFF); z.push_back((~len >> 8) & 0xFF);
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + len);
    }
    put32(z, (b << 16) | a);
    chunk(f, "IDAT", z);
    chunk(f, "IEND", {});
    fclose(f);
}
// OFX images are bottom-up; PNG is top-down. Output is downscaled by 'step'.
void savePreview(const std::string& path, const std::vector<float>& img, int w, int h, float gain, int step)
{
    int ow = w / step, oh = h / step;
    std::vector<uint8_t> rgb((size_t)ow * oh * 3);
    for (int y = 0; y < oh; ++y)
        for (int x = 0; x < ow; ++x)
            for (int c = 0; c < 3; ++c) {
                float v = img[((size_t)(h - 1 - y * step) * w + x * step) * 4 + c] * gain;
                if (!std::isfinite(v)) v = 1.0f;
                rgb[((size_t)y * ow + x) * 3 + c] = (uint8_t)(std::min(1.0f, std::max(0.0f, v)) * 255.0f + 0.5f);
            }
    writePng(path, rgb, ow, oh);
}

// ------------------------------------------------------------------ cases
struct Case
{
    std::string name;
    int w, h;
    std::vector<std::string> params;   // name=value overrides of the plugin defaults
    bool identity;                     // GPU output must also equal the input
};

std::vector<Case> cases(bool quick)
{
    std::vector<Case> c;
    c.push_back({"defaults_super8_edge_leaks", 1920, 1080, {}, false});
    c.push_back({"all_sections_off_identity", 1920, 1080, {"gateOn=0", "weaveOn=0", "grainOn=0", "leakOn=0"}, true});
    if (!quick) {
        for (int f = 0; f < 9; ++f)
            c.push_back({"format_" + std::to_string(f), 1920, 1080, {"format=" + std::to_string(f)}, false});
        const char* emu[] = {"emulsion=0 rms=6", "emulsion=1 rms=8", "emulsion=2 rms=16", "emulsion=3 rms=14 negGamma=0.65"};
        for (int e = 0; e < 4; ++e) {
            Case k{"grain_emulsion_" + std::to_string(e), 1920, 1080, {}, false};
            std::string s = emu[e];
            size_t pos = 0;
            while (pos != std::string::npos) {
                size_t sp = s.find(' ', pos);
                k.params.push_back(s.substr(pos, sp == std::string::npos ? std::string::npos : sp - pos));
                pos = sp == std::string::npos ? sp : sp + 1;
            }
            c.push_back(k);
        }
        c.push_back({"leaks_warm_flares", 1920, 1080, {"flares=0.8", "leakSpread=2", "perfGlow=0.3", "edgeGlow=0.3"}, false});
        c.push_back({"leaks_organic_burn_add", 1920, 1080, {"leakBlend=1", "burn=0.9", "flares=0.2"}, false});
        c.push_back({"leaks_soft_light", 1920, 1080, {"leakBlend=3"}, false});
        c.push_back({"perforations_backlit_both", 1920, 1080, {"perfStyle=1", "perfSide=3", "format=2"}, false});
        c.push_back({"perforations_outline", 1920, 1080, {"perfStyle=2"}, false});
        c.push_back({"picture_only_fit", 1920, 1080, {"gateFraming=1", "perfOn=0", "neighbors=0"}, false});
        c.push_back({"gate_off_with_grain_weave", 1920, 1080, {"gateOn=0"}, false});
        c.push_back({"image_zoom_pan", 1920, 1080, {"imageZoom=1.7", "panX=0.1", "panY=-0.05", "imageFit=1"}, false});
        c.push_back({"strong_weave_rotation", 1920, 1080, {"weaveH=60", "weaveV=90", "rotation=1.5", "scanJitter=20", "frameIndex=37"}, false});
        c.push_back({"dirt_heavy", 1920, 1080, {"dust=2", "fibres=2", "gateHair=1", "scratches=1", "dirtPolarity=2"}, false});
        c.push_back({"extremes", 1920, 1080, {"zoom=8", "frameSize=2", "edgeSoftness=30", "grainSize=10", "grainIntensity=20",
                                               "leakOpacity=5", "perfGlow=10", "edgeGlow=10", "flicker=2", "frameSpacing=2"}, false});
        c.push_back({"vertical_1080x1920", 1080, 1920, {}, false});
        c.push_back({"uhd_3840x2160", 3840, 2160, {}, false});
        c.push_back({"odd_size_1001x777", 1001, 777, {"format=0"}, false});
    }
    return c;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) { fprintf(stderr, "usage: psdv_gpu_compare OUT_DIR [--quick]\n"); return 2; }
    std::string out = argv[1];
    bool quick = argc > 2 && std::string(argv[2]) == "--quick";

    std::string devName, err;
    gpuDumpKernel(out + "/psdv_kernel.metal");   // before init: lets CI compile it offline even without a GPU
    if (!gpuInit(devName, err)) {
        fprintf(stderr, "GPU backend unavailable: %s\n", err.c_str());
        FILE* f = fopen((out + "/report.md").c_str(), "w");
        if (f) { fprintf(f, "# GPU test\n\n**GPU backend unavailable:** %s\n", err.c_str()); fclose(f); }
        return 2;
    }
    printf("GPU device: %s\n", devName.c_str());

    double compileMs = 0;
    if (!gpuCompile(compileMs, err)) {
        fprintf(stderr, "KERNEL COMPILE FAILED:\n%s\n", err.c_str());
        FILE* f = fopen((out + "/report.md").c_str(), "w");
        if (f) { fprintf(f, "# GPU test\n\nDevice: %s\n\n**Kernel compile failed:**\n\n```\n%s\n```\n", devName.c_str(), err.c_str()); fclose(f); }
        return 2;
    }
    printf("Kernel compiled at runtime in %.0f ms\n", compileMs);

    FILE* rep = fopen((out + "/report.md").c_str(), "w");
    FILE* js = fopen((out + "/results.json").c_str(), "w");
    fprintf(rep, "# GPU vs CPU - psdv-film gate emulation\n\nDevice: **%s**  \nRuntime kernel compile: %.0f ms\n\n", devName.c_str(), compileMs);
    fprintf(rep, "| Case | Size | max abs | mean abs | px > 0.01 | GPU NaN/Inf | GPU ms/frame | CPU ms | Result |\n|---|---|---|---|---|---|---|---|---|\n");
    fprintf(js, "{\"device\": \"%s\", \"compile_ms\": %.1f, \"cases\": [\n", devName.c_str(), compileMs);

    int failed = 0, idx = 0;
    std::vector<Case> list = cases(quick);
    for (const Case& k : list) {
        PsdvParams p;
        psdvDefaults(p);
        p.width = k.w; p.height = k.h; p.frameIndex = 12; p.havePrev = 1; p.haveNext = 1;
        for (const std::string& kv : k.params) {
            size_t eq = kv.find('=');
            if (!psdvSetParam(p, kv.substr(0, eq), atof(kv.substr(eq + 1).c_str()))) {
                fprintf(stderr, "unknown parameter in case %s: %s\n", k.name.c_str(), kv.c_str());
                return 1;
            }
        }
        psdv_prepare(p);
        std::vector<float> a, b, c, gpu((size_t)k.w * k.h * 4, -1.0f), cpu((size_t)k.w * k.h * 4);
        makeFrame(a, k.w, k.h, p.frameIndex - 1);
        makeFrame(b, k.w, k.h, p.frameIndex);
        makeFrame(c, k.w, k.h, p.frameIndex + 1);

        double gpuMs = 0;
        if (!gpuRender(p, a.data(), b.data(), c.data(), gpu.data(), gpuMs, err)) {
            fprintf(stderr, "GPU render failed (%s): %s\n", k.name.c_str(), err.c_str());
            fprintf(rep, "| %s | %dx%d | - | - | - | - | - | - | **FAIL: %s** |\n", k.name.c_str(), k.w, k.h, err.c_str());
            failed++;
            continue;
        }
        auto t0 = std::chrono::steady_clock::now();
        cpuRender(p, a.data(), b.data(), c.data(), cpu.data());
        double cpuMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();

        double maxAbs = 0, sumAbs = 0, maxId = 0;
        long over = 0, nonFinite = 0, n = 0;
        for (size_t i = 0; i < gpu.size(); ++i) {
            if (i % 4 == 3) continue;
            if (!std::isfinite(gpu[i])) { nonFinite++; continue; }
            double d = std::fabs((double)gpu[i] - (double)cpu[i]);
            maxAbs = std::max(maxAbs, d);
            sumAbs += d;
            if (d > 0.01) over++;
            if (k.identity) maxId = std::max(maxId, std::fabs((double)gpu[i] - (double)b[i]));
            n++;
        }
        long pixels = (long)k.w * k.h;
        double meanAbs = sumAbs / std::max<long>(1, n);
        double overFrac = (double)over / (3.0 * pixels);
        // Pass criteria: no NaN/Inf; average error far below one 10-bit code;
        // isolated differences allowed on < 0.1% of samples (procedural dirt/grain
        // cells can land on the other side of a cell boundary when the GPU rounds a
        // coordinate differently - visible as single specks in the diff image).
        bool ok = nonFinite == 0 && meanAbs < 2e-4 && overFrac < 1e-3;
        if (k.identity) ok = ok && maxId < 1e-4;   // vs the untouched input, absolute (0..1 code values)
        if (!ok) failed++;

        if (k.identity) printf("  identity vs input: max abs %.2e\n", maxId);
        printf("%-30s %4dx%-4d max %.2e mean %.2e >0.01 %ld nonfinite %ld gpu %.1f ms cpu %.0f ms %s\n", k.name.c_str(), k.w, k.h,
               maxAbs, meanAbs, over, nonFinite, gpuMs, cpuMs, ok ? "PASS" : "FAIL");
        fprintf(rep, "| %s | %dx%d | %.2e | %.2e | %ld (%.4f%%) | %ld | %.1f | %.0f | %s |\n", k.name.c_str(), k.w, k.h, maxAbs, meanAbs,
                over, overFrac * 100.0, nonFinite, gpuMs, cpuMs, ok ? "PASS" : "**FAIL**");
        fprintf(js, "%s{\"case\": \"%s\", \"w\": %d, \"h\": %d, \"max_abs\": %g, \"mean_abs\": %g, \"over_0_01\": %ld, \"nonfinite\": %ld, "
                    "\"gpu_ms\": %.2f, \"cpu_ms\": %.1f, \"identity_abs\": %g, \"pass\": %s}\n",
                idx++ ? "," : "", k.name.c_str(), k.w, k.h, maxAbs, meanAbs, over, nonFinite, gpuMs, cpuMs, maxId, ok ? "true" : "false");

        int step = k.w > 2000 ? 2 : 1;
        savePreview(out + "/" + k.name + "_gpu.png", gpu, k.w, k.h, 1.0f, step);
        std::vector<float> diff(gpu.size());
        for (size_t i = 0; i < gpu.size(); ++i) diff[i] = std::fabs(gpu[i] - cpu[i]);
        savePreview(out + "/" + k.name + "_diff_x50.png", diff, k.w, k.h, 50.0f, step);
    }
    fprintf(js, "], \"failed\": %d}\n", failed);
    fprintf(rep, "\n**%d / %d cases passed.**\n", (int)list.size() - failed, (int)list.size());
    fclose(js);
    fclose(rep);
    printf("%d / %d cases passed\n", (int)list.size() - failed, (int)list.size());
    return failed ? 1 : 0;
}
