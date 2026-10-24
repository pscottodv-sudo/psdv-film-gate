// psdv-film gate emulation - Metal dispatch (compiled with ARC).
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <mutex>
#include <string>
#include <unordered_map>

#include "core/psdv_params.h"
#include "psdv_kernel_source.h"   // generated: const char* kPsdvKernelSource

namespace {
std::mutex gMutex;
std::unordered_map<void*, id<MTLComputePipelineState>> gPipelines;   // one per device
std::string gLastError;

void logError(const std::string& msg)
{
    gLastError = msg;
    NSString* path = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Logs/psdv-film-gate.log"];
    NSString* line = [NSString stringWithFormat:@"%@  %s\n", [NSDate date], msg.c_str()];
    NSFileHandle* fh = [NSFileHandle fileHandleForWritingAtPath:path];
    if (!fh) {
        [line writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:nil];
    } else {
        [fh seekToEndOfFile];
        [fh writeData:[line dataUsingEncoding:NSUTF8StringEncoding]];
        [fh closeFile];
    }
}

id<MTLComputePipelineState> pipelineFor(id<MTLDevice> device)
{
    std::lock_guard<std::mutex> lock(gMutex);
    void* key = (__bridge void*)device;
    auto it = gPipelines.find(key);
    if (it != gPipelines.end()) return it->second;
    NSError* err = nil;
    MTLCompileOptions* opt = [MTLCompileOptions new];
    opt.fastMathEnabled = NO;   // keep the hash and grain statistics identical to the CPU path
    id<MTLLibrary> lib = [device newLibraryWithSource:[NSString stringWithUTF8String:kPsdvKernelSource] options:opt error:&err];
    if (!lib) {
        logError(std::string("Metal compile failed: ") + (err ? err.localizedDescription.UTF8String : "unknown"));
        return nil;
    }
    id<MTLFunction> fn = [lib newFunctionWithName:@"psdv_gate_kernel"];
    if (!fn) { logError("Metal kernel psdv_gate_kernel not found"); return nil; }
    id<MTLComputePipelineState> ps = [device newComputePipelineStateWithFunction:fn error:&err];
    if (!ps) {
        logError(std::string("Metal pipeline failed: ") + (err ? err.localizedDescription.UTF8String : "unknown"));
        return nil;
    }
    gPipelines[key] = ps;
    return ps;
}
} // namespace

// Compiles the kernel for a device ahead of the first render (used by the tests).
bool PsdvMetalPrepare(void* device)
{
    @autoreleasepool {
        return pipelineFor((__bridge id<MTLDevice>)device) != nil;
    }
}

// Last compile / dispatch error message (also written to ~/Library/Logs/psdv-film-gate.log).
const char* PsdvMetalLastError()
{
    std::lock_guard<std::mutex> lock(gMutex);
    static std::string copy;
    copy = gLastError;
    return copy.c_str();
}

bool PsdvRunMetal(void* cmdQueue, const PsdvParams& p, const void* prev, const void* cur, const void* next, void* dst)
{
    @autoreleasepool {
        id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)cmdQueue;
        if (!queue) { logError("no Metal command queue"); return false; }
        id<MTLComputePipelineState> ps = pipelineFor(queue.device);
        if (!ps) return false;
        id<MTLBuffer> bPrev = (__bridge id<MTLBuffer>)prev;
        id<MTLBuffer> bCur = (__bridge id<MTLBuffer>)cur;
        id<MTLBuffer> bNext = (__bridge id<MTLBuffer>)next;
        id<MTLBuffer> bDst = (__bridge id<MTLBuffer>)dst;
        id<MTLCommandBuffer> cb = [queue commandBuffer];
        cb.label = @"psdv-film gate emulation";
        id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
        [enc setComputePipelineState:ps];
        [enc setBytes:&p length:sizeof(PsdvParams) atIndex:0];
        [enc setBuffer:bDst offset:0 atIndex:1];
        [enc setBuffer:bPrev offset:0 atIndex:2];
        [enc setBuffer:bCur offset:0 atIndex:3];
        [enc setBuffer:bNext offset:0 atIndex:4];
        NSUInteger tw = ps.threadExecutionWidth;
        NSUInteger th = ps.maxTotalThreadsPerThreadgroup / tw;
        if (th > 8) th = 8;
        MTLSize group = MTLSizeMake(tw, th, 1);
        MTLSize grid = MTLSizeMake(((NSUInteger)p.width + tw - 1) / tw, ((NSUInteger)p.height + th - 1) / th, 1);
        [enc dispatchThreadgroups:grid threadsPerThreadgroup:group];
        [enc endEncoding];
        [cb commit];
    }
    return true;
}
