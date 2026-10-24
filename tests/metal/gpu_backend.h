// GPU backend used by psdv_gpu_compare. macOS: Metal through the plugin's own
// PsdvRunMetal() (same code path Resolve uses). Elsewhere: a CPU stub that only
// exercises the comparison harness.
#pragma once
#include <string>
#include "core/psdv_params.h"

bool gpuInit(std::string& deviceName, std::string& error);
bool gpuCompile(double& ms, std::string& error);
void gpuDumpKernel(const std::string& path);
// Images: float RGBA, width*height*4, row 0 = bottom.
bool gpuRender(const PsdvParams& p, const float* prev, const float* cur, const float* next, float* out,
               double& msPerFrame, std::string& error);
