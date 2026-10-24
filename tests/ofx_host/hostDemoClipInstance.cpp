#include <algorithm>
#include <cmath>
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxPixels.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhImageEffect.h"
#include "hostDemoHostDescriptor.h"
#include "hostDemoEffectInstance.h"
#include "hostDemoClipInstance.h"

int gW = 1920, gH = 1080;
double gFirst = 0, gLast = 47;
std::vector<float> gSourceOverride;

namespace MyHost {
// linear-light test scene; row 0 = bottom. A 12-bit barcode of the frame number
// fills the lower 30% of the picture (bright = 1, dark = 0).
static void scene(std::vector<float>& d, double time)
{
    int n = (int)std::floor(time + 0.5);
    for (int y = 0; y < gH; ++y) {
        float v = (y + 0.5f) / gH;
        for (int x = 0; x < gW; ++x) {
            float u = (x + 0.5f) / gW;
            float* p = &d[((size_t)y * gW + x) * 4];
            float sky = 0.25f + 0.6f * v;
            p[0] = sky * 0.6f; p[1] = sky * 0.75f; p[2] = sky; p[3] = 1.0f;
            if (v < 0.30f) {
                int bit = std::min(11, (int)(u * 12));
                float fr = u * 12 - bit;
                float val = ((n >> bit) & 1) ? 1.0f : 0.0f;
                if (fr < 0.08f || fr > 0.92f) val = 0.18f;
                p[0] = p[1] = p[2] = val;
            }
        }
    }
}

MyImage::MyImage(MyClipInstance& clip, OfxTime time, bool isOutput)
  : OFX::Host::ImageEffect::Image(clip), _data((size_t)gW * gH * 4, 0.0f)
{
    if (!isOutput) {
        if (!gSourceOverride.empty()) _data = gSourceOverride;
        else scene(_data, time);
    }
    setDoubleProperty(kOfxImageEffectPropRenderScale, 1.0, 0);
    setDoubleProperty(kOfxImageEffectPropRenderScale, 1.0, 1);
    setPointerProperty(kOfxImagePropData, _data.data());
    setIntProperty(kOfxImagePropBounds, 0, 0); setIntProperty(kOfxImagePropBounds, 0, 1);
    setIntProperty(kOfxImagePropBounds, gW, 2); setIntProperty(kOfxImagePropBounds, gH, 3);
    setIntProperty(kOfxImagePropRegionOfDefinition, 0, 0); setIntProperty(kOfxImagePropRegionOfDefinition, 0, 1);
    setIntProperty(kOfxImagePropRegionOfDefinition, gW, 2); setIntProperty(kOfxImagePropRegionOfDefinition, gH, 3);
    setIntProperty(kOfxImagePropRowBytes, gW * 4 * (int)sizeof(float));
    setStringProperty(kOfxImageEffectPropPixelDepth, kOfxBitDepthFloat);
    setStringProperty(kOfxImageEffectPropComponents, kOfxImageComponentRGBA);
}
float* MyImage::pixel(int x, int y) { return &_data[((size_t)y * gW + x) * 4]; }
MyImage::~MyImage() {}

MyClipInstance::MyClipInstance(MyEffectInstance* effect, OFX::Host::ImageEffect::ClipDescriptor* desc)
  : OFX::Host::ImageEffect::ClipInstance(effect, *desc), _effect(effect), _name(desc->getName()), _outputImage(NULL) {}
MyClipInstance::~MyClipInstance() { if (_outputImage) _outputImage->releaseReference(); }
const std::string& MyClipInstance::getUnmappedBitDepth() const { static const std::string v(kOfxBitDepthFloat); return v; }
const std::string& MyClipInstance::getUnmappedComponents() const { static const std::string v(kOfxImageComponentRGBA); return v; }
const std::string& MyClipInstance::getPremult() const { static const std::string v(kOfxImageUnPreMultiplied); return v; }
double MyClipInstance::getAspectRatio() const { return 1.0; }
double MyClipInstance::getFrameRate() const { return 24.0; }
void MyClipInstance::getFrameRange(double& s, double& e) const { s = gFirst; e = gLast; }
const std::string& MyClipInstance::getFieldOrder() const { static const std::string v(kOfxImageFieldNone); return v; }
bool MyClipInstance::getConnected() const { return true; }
double MyClipInstance::getUnmappedFrameRate() const { return 24.0; }
void MyClipInstance::getUnmappedFrameRange(double& s, double& e) const { s = gFirst; e = gLast; }
bool MyClipInstance::getContinuousSamples() const { return false; }
OfxRectD MyClipInstance::getRegionOfDefinition(OfxTime) const { OfxRectD v; v.x1 = v.y1 = 0; v.x2 = gW; v.y2 = gH; return v; }
OFX::Host::ImageEffect::Image* MyClipInstance::getImage(OfxTime time, const OfxRectD*)
{
    if (_name == "Output") {
        if (!_outputImage) _outputImage = new MyImage(*this, 0, true);
        _outputImage->addReference();
        return _outputImage;
    }
    fetchedTimes.push_back(time);
    if (time < gFirst - 1e-6 || time > gLast + 1e-6) return NULL;
    return new MyImage(*this, time, false);
}
} // namespace MyHost
