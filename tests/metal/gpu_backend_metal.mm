// Metal backend for psdv_gpu_compare: uses the plugin's PsdvRunMetal().
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <chrono>
#include <cstring>
#include <fstream>
#include "gpu_backend.h"
#include "psdv_kernel_source.h"

bool PsdvRunMetal(void* cmdQueue, const PsdvParams& p, const void* prev, const void* cur, const void* next, void* dst);
bool PsdvMetalPrepare(void* device);
const char* PsdvMetalLastError();

static id<MTLDevice> gDevice;
static id<MTLCommandQueue> gQueue;

bool gpuInit(std::string& name, std::string& error)
{
    gDevice = MTLCreateSystemDefaultDevice();
    if (!gDevice) { error = "MTLCreateSystemDefaultDevice returned nil (no Metal device on this machine)"; return false; }
    gQueue = [gDevice newCommandQueue];
    name = gDevice.name.UTF8String;
    return gQueue != nil;
}

void gpuDumpKernel(const std::string& path)
{
    std::ofstream(path) << kPsdvKernelSource;
}

bool gpuCompile(double& ms, std::string& error)
{
    auto t0 = std::chrono::steady_clock::now();
    bool ok = PsdvMetalPrepare((__bridge void*)gDevice);
    ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (!ok) error = PsdvMetalLastError();
    return ok;
}

bool gpuRender(const PsdvParams& p, const float* prev, const float* cur, const float* next, float* out, double& ms, std::string& error)
{
    @autoreleasepool {
        size_t bytes = (size_t)p.width * p.height * 4 * sizeof(float);
        id<MTLBuffer> a = [gDevice newBufferWithBytes:prev length:bytes options:MTLResourceStorageModeShared];
        id<MTLBuffer> b = [gDevice newBufferWithBytes:cur length:bytes options:MTLResourceStorageModeShared];
        id<MTLBuffer> c = [gDevice newBufferWithBytes:next length:bytes options:MTLResourceStorageModeShared];
        id<MTLBuffer> d = [gDevice newBufferWithLength:bytes options:MTLResourceStorageModeShared];
        if (!a || !b || !c || !d) { error = "buffer allocation failed"; return false; }
        float* dp = (float*)d.contents;
        for (size_t i = 0; i < bytes / sizeof(float); ++i) dp[i] = -1.0f;   // detects pixels never written

        auto sync = [&]() -> bool {
            id<MTLCommandBuffer> cb = [gQueue commandBuffer];
            [cb commit];
            [cb waitUntilCompleted];
            return cb.status == MTLCommandBufferStatusCompleted;
        };
        // warm-up + correctness run
        if (!PsdvRunMetal((__bridge void*)gQueue, p, (__bridge void*)a, (__bridge void*)b, (__bridge void*)c, (__bridge void*)d)) {
            error = PsdvMetalLastError();
            return false;
        }
        if (!sync()) { error = "command buffer did not complete"; return false; }
        std::memcpy(out, d.contents, bytes);
        // timing: 5 frames back to back, as Resolve would queue them
        const int runs = 5;
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < runs; ++i)
            PsdvRunMetal((__bridge void*)gQueue, p, (__bridge void*)a, (__bridge void*)b, (__bridge void*)c, (__bridge void*)d);
        sync();
        ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / runs;
        return true;
    }
}
