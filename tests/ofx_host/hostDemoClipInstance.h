// Test host clips: float RGBA images; source content depends on time.
#ifndef HOST_DEMO_CLIP_INSTANCE_H
#define HOST_DEMO_CLIP_INSTANCE_H
#include <vector>
extern int gW, gH;              // project size in pixels
extern double gFirst, gLast;    // clip frame range
extern std::vector<float> gSourceOverride; // optional fixed RGBA source (same for all times)
namespace MyHost {
  class MyClipInstance;
  class MyImage : public OFX::Host::ImageEffect::Image
  {
  protected:
    std::vector<float> _data;
  public:
    explicit MyImage(MyClipInstance& clip, OfxTime t, bool isOutput);
    float* pixel(int x, int y);
    ~MyImage();
  };
  class MyClipInstance : public OFX::Host::ImageEffect::ClipInstance {
  protected:
    MyEffectInstance* _effect;
    std::string _name;
    MyImage* _outputImage;
  public:
    std::vector<double> fetchedTimes;   // every time the plugin asked for
    MyClipInstance(MyEffectInstance* effect, OFX::Host::ImageEffect::ClipDescriptor* desc);
    virtual ~MyClipInstance();
    MyImage* getOutputImage() { return _outputImage; }
    const std::string& getUnmappedBitDepth() const;
    virtual const std::string& getUnmappedComponents() const;
    virtual const std::string& getPremult() const;
    virtual double getAspectRatio() const;
    virtual double getFrameRate() const;
    virtual void getFrameRange(double& startFrame, double& endFrame) const;
    virtual const std::string& getFieldOrder() const;
    virtual bool getConnected() const;
    virtual double getUnmappedFrameRate() const;
    virtual void getUnmappedFrameRange(double& unmappedStartFrame, double& unmappedEndFrame) const;
    virtual bool getContinuousSamples() const;
    virtual OfxRectD getRegionOfDefinition(OfxTime time) const;
    virtual OFX::Host::ImageEffect::Image* getImage(OfxTime time, const OfxRectD* optionalBounds);
  };
}
#endif
