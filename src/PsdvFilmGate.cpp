// psdv-film gate emulation - OpenFX plugin (CPU + Apple Metal).
// Independent implementation for DaVinci Resolve. Uses only the OpenFX SDK
// Support library (BSD-3-Clause).
#include <cstdio>
#include <cstring>
#include <memory>
#include <set>
#include <string>

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.h"

#include "core/psdv_core.h"
#include "core/psdv_presets.h"

#define kPluginName "psdv-film gate emulation"
#define kPluginGrouping "PSDV"
#define kPluginDescription \
    "Film gate, perforations, gate weave, emulsion grain and light leaks for 8mm, Super 8, 16mm, 35mm and 65mm.\n" \
    "Each section has its own Enable switch, a preset menu and manual controls.\n" \
    "Grain presets: only Double-X 5222 (RMS 14) and the Ektachrome/E100 preset (RMS 8) use published values; the others are estimates."
#define kPluginIdentifier "it.psdv.filmgate"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 2

// ------------------------------------------------------------------ parameter names
// input
#define P_ENCODING "inputEncoding"
// gate
#define P_GATE_ON "gateEnable"
#define P_GATE_PRESET "gatePreset"
#define P_FORMAT "filmFormat"
#define P_FRAMING "gateFraming"
#define P_FRAME_SIZE "frameSize"
#define P_ASPECT "aspectRatio"
#define P_ASPECT_CUSTOM "customAspect"
#define P_ZOOM "zoom"
#define P_CENTRE "centreOn"
#define P_POS_X "positionX"
#define P_POS_Y "positionY"
#define P_IMAGE_FIT "imageFit"
#define P_IMAGE_ZOOM "imageZoom"
#define P_PAN_X "imagePanX"
#define P_PAN_Y "imagePanY"
#define P_NEIGHBORS "neighborFrames"
#define P_SPACING "frameSpacing"
#define P_EDGE_SOFT "edgeSoftness"
#define P_EDGE_DARK "edgeDarkness"
#define P_EDGE_DARK_W "edgeDarkWidth"
#define P_CORNER "cornerRadius"
#define P_EDGE_IRR "edgeIrregularity"
#define P_EDGE_IRR_SIZE "irregularitySize"
#define P_EDGE_IRR_MOTION "irregularityMotion"
#define P_DEFOCUS "gateDefocus"
#define P_OPTICAL "opticalSoftness"
#define P_PERF_ON "showPerforations"
#define P_PERF_STYLE "perforationStyle"
#define P_PERF_SIDE "perforationSide"
#define P_PERF_SIZE "perforationSize"
#define P_PERF_OPACITY "perforationOpacity"
#define P_DUST "dust"
#define P_FIBRES "fibres"
#define P_GATE_HAIR "gateHair"
#define P_SCRATCHES "scratches"
#define P_DIRT_POL "dirtPolarity"
// weave
#define P_WEAVE_ON "weaveEnable"
#define P_WEAVE_PRESET "weavePreset"
#define P_WEAVE_AMOUNT "weaveAmount"
#define P_WEAVE_H "weaveHorizontal"
#define P_WEAVE_V "weaveVertical"
#define P_WEAVE_RANDOM "weaveRandomness"
#define P_WEAVE_CYCLE "weaveCycle"
#define P_ROTATION "rotationalWeave"
#define P_SCAN_JITTER "scannerJitter"
#define P_FLICKER "exposureFlicker"
#define P_FLICKER_RANDOM "flickerRandomness"
#define P_WEAVE_SEED "weaveSeed"
// grain
#define P_GRAIN_ON "grainEnable"
#define P_GRAIN_PRESET "grainPreset"
#define P_EMULSION "emulsionType"
#define P_RMS "rmsGranularity"
#define P_GRAIN_INT "grainIntensity"
#define P_GRAIN_SIZE "grainSize"
#define P_SHADOW "shadowResponse"
#define P_MID "midtoneResponse"
#define P_HIGH "highlightResponse"
#define P_COLOR_GRAIN "colorGrain"
#define P_NEG_GAMMA "negativeGamma"
#define P_GRAIN_SOFT "grainSoftness"
#define P_GRAIN_CLUMP "grainClumping"
#define P_GRAIN_SEED "grainSeed"
// leaks
#define P_LEAK_ON "leakEnable"
#define P_LEAK_PRESET "leakPreset"
#define P_LEAK_OPACITY "leakOpacity"
#define P_LEAK_BLEND "leakBlendMode"
#define P_LEAK_COLOR "leakColor"
#define P_LEAK_FREQ "leakFrequency"
#define P_LEAK_SPREAD "leakSize"
#define P_PERF_GLOW "perforationGlow"
#define P_EDGE_GLOW "gateEdgeGlow"
#define P_FLARES "warmFlares"
#define P_BURN "organicBurn"
#define P_LEAK_SEED "leakSeed"

static const char* kGateSectionParams[] = {P_FORMAT, P_FRAMING, P_FRAME_SIZE, P_NEIGHBORS, P_SPACING, P_PERF_ON, P_PERF_STYLE,
                                           P_EDGE_SOFT, P_EDGE_DARK, P_EDGE_DARK_W, P_CORNER, P_EDGE_IRR, P_EDGE_IRR_SIZE, P_EDGE_IRR_MOTION, P_DEFOCUS, P_DUST, P_FIBRES, P_GATE_HAIR, P_SCRATCHES};
static const char* kWeaveSectionParams[] = {P_WEAVE_H, P_WEAVE_V, P_WEAVE_RANDOM, P_WEAVE_CYCLE, P_ROTATION, P_SCAN_JITTER, P_FLICKER, P_FLICKER_RANDOM};
static const char* kGrainSectionParams[] = {P_EMULSION, P_RMS, P_GRAIN_SIZE, P_COLOR_GRAIN, P_NEG_GAMMA, P_GRAIN_SOFT, P_GRAIN_CLUMP, P_SHADOW, P_MID, P_HIGH};
static const char* kLeakSectionParams[] = {P_LEAK_OPACITY, P_LEAK_BLEND, P_LEAK_COLOR, P_LEAK_FREQ, P_LEAK_SPREAD, P_PERF_GLOW, P_EDGE_GLOW, P_FLARES, P_BURN};

static bool inList(const std::string& n, const char* const* list, size_t count)
{
    for (size_t i = 0; i < count; ++i) if (n == list[i]) return true;
    return false;
}
#define LIST_SIZE(a) (sizeof(a) / sizeof(a[0]))

#ifdef __APPLE__
extern bool PsdvRunMetal(void* cmdQueue, const PsdvParams& p, const void* prev, const void* cur, const void* next, void* dst);
#endif

// ------------------------------------------------------------------ CPU processor
struct PsdvCpuSource
{
    const OFX::Image* img[3];
    int w, h;
    ps3 fetch(int k, int x, int y) const
    {
        const OFX::Image* im = img[k] ? img[k] : img[1];
        const OfxRectI& b = im->getBounds();
        const float* px = static_cast<const float*>(im->getPixelAddress(b.x1 + x, b.y1 + y));
        if (!px) return P3(0.0f, 0.0f, 0.0f);
        return P3(px[0], px[1], px[2]);
    }
};

class PsdvProcessor : public OFX::ImageProcessor
{
public:
    PsdvProcessor(OFX::ImageEffect& e) : OFX::ImageProcessor(e) {}
    PsdvParams params;
    PsdvCpuSource src;
    OFX::Image* cur = nullptr;
    void multiThreadProcessImages(OfxRectI win) override
    {
        const OfxRectI& b = _dstImg->getBounds();
        for (int y = win.y1; y < win.y2; ++y) {
            if (_effect.abort()) break;
            float* d = static_cast<float*>(_dstImg->getPixelAddress(win.x1, y));
            for (int x = win.x1; x < win.x2; ++x, d += 4) {
                ps3 c = psdv_render_pixel(src, params, x - b.x1, y - b.y1);
                const float* s = cur ? static_cast<const float*>(cur->getPixelAddress(x, y)) : nullptr;
                d[0] = c.x; d[1] = c.y; d[2] = c.z; d[3] = s ? s[3] : 1.0f;
            }
        }
    }
};

// ------------------------------------------------------------------ the effect
class PsdvFilmGate : public OFX::ImageEffect
{
public:
    explicit PsdvFilmGate(OfxImageEffectHandle h) : OFX::ImageEffect(h)
    {
        dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
        srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    }
    void render(const OFX::RenderArguments& args) override;
    void changedParam(const OFX::InstanceChangedArgs& args, const std::string& name) override;
    void getFramesNeeded(const OFX::FramesNeededArguments& args, OFX::FramesNeededSetter& frames) override;

private:
    OFX::Clip* dstClip_;
    OFX::Clip* srcClip_;
    bool applyingPreset_ = false;

    double dbl(const char* n, double t) { return fetchDoubleParam(n)->getValueAtTime(t); }
    int integer(const char* n, double t) { return fetchIntParam(n)->getValueAtTime(t); }
    int choice(const char* n, double t) { int v = 0; fetchChoiceParam(n)->getValueAtTime(t, v); return v; }
    bool boolean(const char* n, double t) { return fetchBooleanParam(n)->getValueAtTime(t); }
    void setD(const char* n, double v) { fetchDoubleParam(n)->setValue(v); }
    void setC(const char* n, int v) { fetchChoiceParam(n)->setValue(v); }
    void setB(const char* n, bool v) { fetchBooleanParam(n)->setValue(v); }
    void fillParams(PsdvParams& p, double t, int w, int h, double renderScale);
};

void PsdvFilmGate::fillParams(PsdvParams& p, double t, int w, int h, double renderScale)
{
    std::memset(&p, 0, sizeof(p));
    p.width = w; p.height = h;
    p.frameIndex = (int)(t >= 0.0 ? t + 0.5 : t - 0.5);
    p.encoding = choice(P_ENCODING, t);
    p.renderScale = (float)renderScale;
    // gate
    p.gateOn = boolean(P_GATE_ON, t);
    p.format = choice(P_FORMAT, t);
    p.gateFraming = choice(P_FRAMING, t);
    static const float aspects[] = {0.0f, 4.0f / 3.0f, 1.375f, 1.66f, 16.0f / 9.0f, 1.85f, 2.39f};
    int a = choice(P_ASPECT, t);
    p.aspect = a < 7 ? aspects[a] : (float)dbl(P_ASPECT_CUSTOM, t);
    p.frameSize = (float)dbl(P_FRAME_SIZE, t);
    p.zoom = (float)dbl(P_ZOOM, t);
    p.centreOn = choice(P_CENTRE, t);
    p.posX = (float)dbl(P_POS_X, t); p.posY = (float)dbl(P_POS_Y, t);
    p.imageFit = choice(P_IMAGE_FIT, t);
    p.imageZoom = (float)dbl(P_IMAGE_ZOOM, t);
    p.panX = (float)dbl(P_PAN_X, t); p.panY = (float)dbl(P_PAN_Y, t);
    p.neighbors = choice(P_NEIGHBORS, t);
    p.frameSpacing = (float)dbl(P_SPACING, t);
    p.edgeSoftness = (float)dbl(P_EDGE_SOFT, t);
    p.edgeDarkness = (float)dbl(P_EDGE_DARK, t);
    p.edgeDarkWidth = (float)dbl(P_EDGE_DARK_W, t);
    p.cornerRadius = (float)dbl(P_CORNER, t);
    p.edgeIrregularity = (float)dbl(P_EDGE_IRR, t);
    p.edgeIrrSize = (float)dbl(P_EDGE_IRR_SIZE, t);
    p.edgeIrrMotion = (float)dbl(P_EDGE_IRR_MOTION, t);
    p.gateDefocus = (float)dbl(P_DEFOCUS, t);
    p.opticalSoft = (float)dbl(P_OPTICAL, t);
    p.perfOn = boolean(P_PERF_ON, t);
    p.perfStyle = choice(P_PERF_STYLE, t);
    p.perfSide = choice(P_PERF_SIDE, t);
    p.perfScale = (float)dbl(P_PERF_SIZE, t);
    p.perfOpacity = (float)dbl(P_PERF_OPACITY, t);
    p.dust = (float)dbl(P_DUST, t); p.fibres = (float)dbl(P_FIBRES, t);
    p.gateHair = (float)dbl(P_GATE_HAIR, t); p.scratches = (float)dbl(P_SCRATCHES, t);
    p.dirtPolarity = choice(P_DIRT_POL, t);
    if (!p.gateOn) { p.perfOn = 0; }
    // weave
    p.weaveOn = boolean(P_WEAVE_ON, t);
    float amt = (float)dbl(P_WEAVE_AMOUNT, t);
    p.weaveH = (float)dbl(P_WEAVE_H, t) * amt;
    p.weaveV = (float)dbl(P_WEAVE_V, t) * amt;
    p.weaveRandom = (float)dbl(P_WEAVE_RANDOM, t);
    p.weaveCycle = (float)dbl(P_WEAVE_CYCLE, t);
    p.rotation = (float)dbl(P_ROTATION, t) * amt;
    p.scanJitter = (float)dbl(P_SCAN_JITTER, t) * amt;
    p.flicker = (float)dbl(P_FLICKER, t);
    p.flickerRandom = (float)dbl(P_FLICKER_RANDOM, t);
    p.weaveSeed = integer(P_WEAVE_SEED, t);
    // grain
    p.grainOn = boolean(P_GRAIN_ON, t);
    p.emulsion = choice(P_EMULSION, t);
    p.rms = (float)dbl(P_RMS, t);
    p.grainIntensity = (float)dbl(P_GRAIN_INT, t);
    p.grainSize = (float)dbl(P_GRAIN_SIZE, t);
    p.shadowResponse = (float)dbl(P_SHADOW, t);
    p.midResponse = (float)dbl(P_MID, t);
    p.highlightResponse = (float)dbl(P_HIGH, t);
    p.colorGrain = (float)dbl(P_COLOR_GRAIN, t);
    p.negGamma = (float)dbl(P_NEG_GAMMA, t);
    p.grainSoftness = (float)dbl(P_GRAIN_SOFT, t);
    p.grainClumping = (float)dbl(P_GRAIN_CLUMP, t);
    p.grainSeed = integer(P_GRAIN_SEED, t);
    // leaks
    p.leakOn = boolean(P_LEAK_ON, t);
    p.leakOpacity = (float)dbl(P_LEAK_OPACITY, t);
    p.leakBlend = choice(P_LEAK_BLEND, t);
    double r = 1, g = 0.28, b = 0.04;
    fetchRGBParam(P_LEAK_COLOR)->getValueAtTime(t, r, g, b);
    p.tintR = (float)r; p.tintG = (float)g; p.tintB = (float)b;
    p.leakFrequency = (float)dbl(P_LEAK_FREQ, t);
    p.leakSpread = (float)dbl(P_LEAK_SPREAD, t);
    p.perfGlow = (float)dbl(P_PERF_GLOW, t);
    p.edgeGlow = (float)dbl(P_EDGE_GLOW, t);
    p.flares = (float)dbl(P_FLARES, t);
    p.burn = (float)dbl(P_BURN, t);
    p.leakSeed = integer(P_LEAK_SEED, t);
}

void PsdvFilmGate::getFramesNeeded(const OFX::FramesNeededArguments& args, OFX::FramesNeededSetter& frames)
{
    OfxRangeD r;
    r.min = r.max = args.time;
    if (choice(P_NEIGHBORS, args.time) == 1 && boolean(P_GATE_ON, args.time)) {
        OfxRectD rod = srcClip_->getRegionOfDefinition(args.time);
        int w = (int)(rod.x2 - rod.x1), h = (int)(rod.y2 - rod.y1);
        bool above = true, below = true;
        if (w > 0 && h > 0) {
            PsdvParams p;
            fillParams(p, args.time, w, h, 1.0);
            psdv_neighbors_visible(p, above, below);
        }
        if (above) r.min = args.time - 1.0;
        if (below) r.max = args.time + 1.0;
    }
    frames.setFramesNeeded(*srcClip_, r);
}

void PsdvFilmGate::render(const OFX::RenderArguments& args)
{
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> cur(srcClip_->fetchImage(args.time));
    if (!dst.get() || !cur.get()) OFX::throwSuiteStatusException(kOfxStatFailed);
    if (dst->getPixelDepth() != OFX::eBitDepthFloat || dst->getPixelComponents() != OFX::ePixelComponentRGBA ||
        cur->getPixelDepth() != OFX::eBitDepthFloat || cur->getPixelComponents() != OFX::ePixelComponentRGBA)
        OFX::throwSuiteStatusException(kOfxStatErrUnsupported);

    const OfxRectI& db = dst->getBounds();
    PsdvParams p;
    fillParams(p, args.time, db.x2 - db.x1, db.y2 - db.y1, args.renderScale.x);
    psdv_prepare(p);

    std::unique_ptr<OFX::Image> prev, next;
    if (p.gateOn && p.neighbors == 1) {
        bool above = true, below = true;
        psdv_neighbors_visible(p, above, below);      // only fetch what is on screen
        OfxRangeD range = srcClip_->getFrameRange();
        if (above && args.time - 1.0 >= range.min - 1e-6) prev.reset(srcClip_->fetchImage(args.time - 1.0));
        if (below && args.time + 1.0 <= range.max + 1e-6) next.reset(srcClip_->fetchImage(args.time + 1.0));
        // neighbours must match the current frame geometry, otherwise they are not used
        if (prev.get()) { const OfxRectI& b = prev->getBounds(); if (b.x2 - b.x1 != p.width || b.y2 - b.y1 != p.height) prev.reset(); }
        if (next.get()) { const OfxRectI& b = next->getBounds(); if (b.x2 - b.x1 != p.width || b.y2 - b.y1 != p.height) next.reset(); }
    }
    p.havePrev = prev.get() ? 1 : 0;
    p.haveNext = next.get() ? 1 : 0;

#ifdef __APPLE__
    if (args.isEnabledMetalRender) {
        const void* c = cur->getPixelData();
        const void* a = prev.get() ? prev->getPixelData() : c;
        const void* n = next.get() ? next->getPixelData() : c;
        if (!PsdvRunMetal(args.pMetalCmdQ, p, a, c, n, dst->getPixelData()))
            OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }
#endif
    PsdvProcessor proc(*this);
    proc.params = p;
    proc.src.img[0] = prev.get();
    proc.src.img[1] = cur.get();
    proc.src.img[2] = next.get();
    const OfxRectI& cb = cur->getBounds();
    proc.src.w = cb.x2 - cb.x1;
    proc.src.h = cb.y2 - cb.y1;
    proc.cur = cur.get();
    proc.setDstImg(dst.get());
    proc.setRenderWindow(args.renderWindow);
    proc.process();
}

void PsdvFilmGate::changedParam(const OFX::InstanceChangedArgs& args, const std::string& name)
{
    if (applyingPreset_) return;
    if (args.reason != OFX::eChangeUserEdit) return;
    applyingPreset_ = true;
    if (name == P_GATE_PRESET) {
        int i = choice(P_GATE_PRESET, args.time);
        if (i > 0 && i < kGatePresetCount) {
            const PsdvGatePreset& q = kGatePresets[i];
            setC(P_FORMAT, q.format); setC(P_FRAMING, q.framing); setD(P_FRAME_SIZE, q.frameSize);
            setC(P_NEIGHBORS, q.neighbors); setD(P_SPACING, q.spacing); setB(P_PERF_ON, q.perfOn != 0);
            setC(P_PERF_STYLE, q.perfStyle); setD(P_EDGE_SOFT, q.edgeSoft); setD(P_EDGE_DARK, q.edgeDark);
            setD(P_EDGE_DARK_W, q.edgeDarkWidth); setD(P_CORNER, q.corner); setD(P_DUST, q.dust);
            setD(P_EDGE_IRR, q.irregularity); setD(P_EDGE_IRR_MOTION, q.irrMotion); setD(P_DEFOCUS, q.defocus);
            setD(P_FIBRES, q.fibres); setD(P_GATE_HAIR, q.gateHair); setD(P_SCRATCHES, q.scratches);
        }
    } else if (name == P_WEAVE_PRESET) {
        int i = choice(P_WEAVE_PRESET, args.time);
        if (i > 0 && i < kWeavePresetCount) {
            const PsdvWeavePreset& q = kWeavePresets[i];
            setD(P_WEAVE_H, q.weaveH); setD(P_WEAVE_V, q.weaveV); setD(P_WEAVE_RANDOM, q.randomness);
            setD(P_WEAVE_CYCLE, q.cycle); setD(P_ROTATION, q.rotation); setD(P_SCAN_JITTER, q.scanJitter);
            setD(P_FLICKER, q.flicker); setD(P_FLICKER_RANDOM, q.flickerRandom); setD(P_WEAVE_AMOUNT, 1.0);
        }
    } else if (name == P_GRAIN_PRESET) {
        int i = choice(P_GRAIN_PRESET, args.time);
        if (i > 0 && i < kGrainPresetCount) {
            const PsdvGrainPreset& q = kGrainPresets[i];
            setC(P_EMULSION, q.emulsion); setD(P_RMS, q.rms); setD(P_GRAIN_SIZE, q.size);
            setD(P_COLOR_GRAIN, q.colorGrain); setD(P_NEG_GAMMA, q.negGamma);
            setD(P_GRAIN_SOFT, q.softness); setD(P_GRAIN_CLUMP, q.clumping);
            setD(P_SHADOW, q.shadow); setD(P_MID, q.mid); setD(P_HIGH, q.highlight);
        }
    } else if (name == P_LEAK_PRESET) {
        int i = choice(P_LEAK_PRESET, args.time);
        if (i > 0 && i < kLeakPresetCount) {
            const PsdvLeakPreset& q = kLeakPresets[i];
            setD(P_LEAK_OPACITY, q.opacity); setC(P_LEAK_BLEND, q.blend);
            fetchRGBParam(P_LEAK_COLOR)->setValue(q.r, q.g, q.b);
            setD(P_LEAK_FREQ, q.frequency); setD(P_LEAK_SPREAD, q.spread); setD(P_PERF_GLOW, q.perfGlow);
            setD(P_EDGE_GLOW, q.edgeGlow); setD(P_FLARES, q.flares); setD(P_BURN, q.burn);
        }
    } else if (inList(name, kGateSectionParams, LIST_SIZE(kGateSectionParams))) {
        if (choice(P_GATE_PRESET, args.time) != 0) setC(P_GATE_PRESET, 0);
    } else if (inList(name, kWeaveSectionParams, LIST_SIZE(kWeaveSectionParams))) {
        if (choice(P_WEAVE_PRESET, args.time) != 0) setC(P_WEAVE_PRESET, 0);
    } else if (inList(name, kGrainSectionParams, LIST_SIZE(kGrainSectionParams))) {
        if (choice(P_GRAIN_PRESET, args.time) != 0) setC(P_GRAIN_PRESET, 0);
    } else if (inList(name, kLeakSectionParams, LIST_SIZE(kLeakSectionParams))) {
        if (choice(P_LEAK_PRESET, args.time) != 0) setC(P_LEAK_PRESET, 0);
    }
    applyingPreset_ = false;
}

// ------------------------------------------------------------------ factory
mDeclarePluginFactory(PsdvFilmGateFactory, {}, {});

using namespace OFX;

void PsdvFilmGateFactory::describe(ImageEffectDescriptor& desc)
{
    desc.setLabels(kPluginName, kPluginName, kPluginName);
    desc.setPluginGrouping(kPluginGrouping);
    desc.setPluginDescription(kPluginDescription);
    desc.addSupportedContext(eContextFilter);
    desc.addSupportedContext(eContextGeneral);
    desc.addSupportedBitDepth(eBitDepthFloat);
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(false);
    desc.setTemporalClipAccess(true);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);
    desc.setRenderThreadSafety(eRenderFullySafe);
#ifdef __APPLE__
    desc.setSupportsMetalRender(true);
#endif
}

namespace {
GroupParamDescriptor* group(ImageEffectDescriptor& d, PageParamDescriptor* page, const char* name, const char* label, bool open)
{
    GroupParamDescriptor* g = d.defineGroupParam(name);
    g->setLabels(label, label, label);
    g->setOpen(open);
    if (page) page->addChild(*g);
    return g;
}
DoubleParamDescriptor* dparam(ImageEffectDescriptor& d, PageParamDescriptor* page, GroupParamDescriptor* g, const char* name,
                              const char* label, const char* hint, double def, double mn, double mx, double dmn, double dmx)
{
    DoubleParamDescriptor* p = d.defineDoubleParam(name);
    p->setLabels(label, label, label);
    p->setHint(hint);
    p->setDefault(def);
    p->setRange(mn, mx);
    p->setDisplayRange(dmn, dmx);
    p->setDoubleType(eDoubleTypePlain);
    p->setParent(*g);
    if (page) page->addChild(*p);
    return p;
}
IntParamDescriptor* iparam(ImageEffectDescriptor& d, PageParamDescriptor* page, GroupParamDescriptor* g, const char* name,
                           const char* label, const char* hint, int def, int mn, int mx)
{
    IntParamDescriptor* p = d.defineIntParam(name);
    p->setLabels(label, label, label);
    p->setHint(hint);
    p->setDefault(def);
    p->setRange(mn, mx);
    p->setDisplayRange(mn, mx);
    p->setParent(*g);
    if (page) page->addChild(*p);
    return p;
}
BooleanParamDescriptor* bparam(ImageEffectDescriptor& d, PageParamDescriptor* page, GroupParamDescriptor* g, const char* name,
                               const char* label, const char* hint, bool def)
{
    BooleanParamDescriptor* p = d.defineBooleanParam(name);
    p->setLabels(label, label, label);
    p->setHint(hint);
    p->setDefault(def);
    if (g) p->setParent(*g);
    if (page) page->addChild(*p);
    return p;
}
ChoiceParamDescriptor* cparam(ImageEffectDescriptor& d, PageParamDescriptor* page, GroupParamDescriptor* g, const char* name,
                              const char* label, const char* hint, const char* const* options, int count, int def)
{
    ChoiceParamDescriptor* p = d.defineChoiceParam(name);
    p->setLabels(label, label, label);
    p->setHint(hint);
    for (int i = 0; i < count; ++i) p->appendOption(options[i]);
    p->setDefault(def);
    p->setAnimates(false);
    if (g) p->setParent(*g);
    if (page) page->addChild(*p);
    return p;
}
template <class T>
std::vector<const char*> labels(const T* table, int count)
{
    std::vector<const char*> v;
    for (int i = 0; i < count; ++i) v.push_back(table[i].label);
    return v;
}
} // namespace

void PsdvFilmGateFactory::describeInContext(ImageEffectDescriptor& desc, ContextEnum /*context*/)
{
    ClipDescriptor* src = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    src->addSupportedComponent(ePixelComponentRGBA);
    src->setTemporalClipAccess(true);
    src->setSupportsTiles(false);
    src->setIsMask(false);
    ClipDescriptor* dst = desc.defineClip(kOfxImageEffectOutputClipName);
    dst->addSupportedComponent(ePixelComponentRGBA);
    dst->setSupportsTiles(false);

    PageParamDescriptor* page = desc.definePageParam("Controls");

    // ---------------- input
    GroupParamDescriptor* gIn = group(desc, page, "grpInput", "Input", true);
    static const char* enc[] = {"DaVinci Intermediate", "Rec.709 (Gamma 2.4)", "Linear"};
    cparam(desc, page, gIn, P_ENCODING, "Input Encoding",
           "How the incoming image is encoded. With a DaVinci Wide Gamut / Intermediate pipeline choose DaVinci Intermediate.",
           enc, 3, 0);

    // ---------------- film gate & perforations
    GroupParamDescriptor* gG = group(desc, page, "grpGate", "Film Gate & Perforations", true);
    bparam(desc, page, gG, P_GATE_ON, "Enable Film Gate", "Gate, perforations, edge, optical softness, dust and scratches. Off = clean full-frame picture.", true);
    std::vector<const char*> gp = labels(kGatePresets, kGatePresetCount);
    cparam(desc, page, gG, P_GATE_PRESET, "Gate Preset", "Loads a complete gate look into the controls below. Editing any control switches back to Custom.",
           gp.data(), (int)gp.size(), 1);
    cparam(desc, page, gG, P_FORMAT, "Film Format", "Gauge: real gate size, frame pitch and perforation geometry.", kFormatLabels, kFormatCount, 1);
    static const char* framing[] = {"Manual Frame Size", "Fit Gate To Screen", "Fill Screen With Picture"};
    cparam(desc, page, gG, P_FRAMING, "Gate Framing", "Fit = whole gate centred on screen; Fill = picture covers the screen.", framing, 3, 0);
    dparam(desc, page, gG, P_FRAME_SIZE, "Frame Size", "Height of the gate as a fraction of the screen height (Manual Frame Size).", 0.76, 0.05, 2.0, 0.2, 1.2);
    static const char* asp[] = {"Native Gate", "4:3 (1.33)", "Academy (1.37)", "1.66", "16:9 (1.78)", "1.85", "Scope (2.39)", "Custom"};
    cparam(desc, page, gG, P_ASPECT, "Gate Aspect Ratio", "Shape of the gate opening. Native uses the real gauge aperture.", asp, 8, 0);
    dparam(desc, page, gG, P_ASPECT_CUSTOM, "Custom Aspect Ratio", "Width / height when Gate Aspect Ratio is Custom.", 1.33, 0.5, 4.0, 1.0, 2.6);
    dparam(desc, page, gG, P_ZOOM, "Zoom", "Scales the whole gate and perforations.", 1.0, 0.1, 8.0, 0.25, 4.0);
    static const char* centre[] = {"Picture Centre", "Film Centre Line"};
    cparam(desc, page, gG, P_CENTRE, "Centre On", "Picture Centre keeps the image centred whatever the gauge offset.", centre, 2, 0);
    dparam(desc, page, gG, P_POS_X, "Position X", "Moves the gate horizontally (fraction of the screen width).", 0.0, -1.0, 1.0, -0.5, 0.5);
    dparam(desc, page, gG, P_POS_Y, "Position Y", "Moves the gate vertically (fraction of the screen height).", 0.0, -1.0, 1.0, -0.5, 0.5);
    static const char* fit[] = {"Fill Gate (crop)", "Fit Whole Image"};
    cparam(desc, page, gG, P_IMAGE_FIT, "Image In Gate", "How the clip is placed inside the gate before Image Zoom.", fit, 2, 0);
    dparam(desc, page, gG, P_IMAGE_ZOOM, "Image Zoom", "Zooms the clip inside the gate.", 1.0, 0.1, 8.0, 0.25, 4.0);
    dparam(desc, page, gG, P_PAN_X, "Image Pan X", "Moves the clip inside the gate.", 0.0, -1.0, 1.0, -0.5, 0.5);
    dparam(desc, page, gG, P_PAN_Y, "Image Pan Y", "Moves the clip inside the gate.", 0.0, -1.0, 1.0, -0.5, 0.5);
    static const char* nb[] = {"Off (black)", "Real Previous / Next Frames (slower)", "Repeat Current Frame (fast)"};
    cparam(desc, page, gG, P_NEIGHBORS, "Frames Above / Below",
           "Repeat is real-time friendly. Real asks Resolve for the previous and next frames too: up to 3x the decoding and upstream "
           "work, use it for the final render. Frames that are off screen are never requested.", nb, 3, 2);
    dparam(desc, page, gG, P_SPACING, "Frame Spacing", "Extra distance between frames, as a fraction of the gate height. 0 = real gauge pitch.", 0.12, -0.2, 2.0, 0.0, 1.0);
    dparam(desc, page, gG, P_EDGE_SOFT, "Gate Edge Softness", "Blur of the gate edge (out-of-focus aperture shadow).", 1.6, 0.0, 30.0, 0.0, 8.0);
    dparam(desc, page, gG, P_EDGE_DARK, "Gate Edge Darkness", "Light fall-off of the picture along the gate edge.", 0.35, 0.0, 1.0, 0.0, 1.0);
    dparam(desc, page, gG, P_EDGE_DARK_W, "Edge Darkness Width", "Width of the fall-off, fraction of the gate height.", 0.05, 0.001, 0.5, 0.005, 0.2);
    dparam(desc, page, gG, P_CORNER, "Corner Roundness", "Multiplier of the real corner radius of the gauge.", 1.6, 0.0, 10.0, 0.0, 4.0);
    dparam(desc, page, gG, P_EDGE_IRR, "Edge Irregularity (µm)",
           "Small bumps in and out along the gate outline, in micrometres on the film.", 14.0, 0.0, 500.0, 0.0, 80.0);
    dparam(desc, page, gG, P_EDGE_IRR_SIZE, "Irregularity Size", "Length of the bumps along the edge (multiplier).", 1.0, 0.05, 10.0, 0.2, 4.0);
    dparam(desc, page, gG, P_EDGE_IRR_MOTION, "Irregularity Motion",
           "0 = the outline is fixed (it belongs to the gate), 1 = a new outline every frame. Not weave, not flicker: only the outline changes.",
           0.6, 0.0, 1.0, 0.0, 1.0);
    dparam(desc, page, gG, P_DEFOCUS, "Gate Defocus",
           "Out-of-focus gate: wider soft edges and rounded, darker corners, like an aperture far from the focal plane.", 0.15, 0.0, 5.0, 0.0, 2.0);
    dparam(desc, page, gG, P_OPTICAL, "Optical Softness", "Lens and emulsion softness, physical size on the film.", 1.0, 0.0, 20.0, 0.0, 5.0);
    bparam(desc, page, gG, P_PERF_ON, "Show Perforations", "Sprocket holes beside the gate.", true);
    static const char* pst[] = {"Dark Hole", "Backlit (Scanner Light)", "Outline Only"};
    cparam(desc, page, gG, P_PERF_STYLE, "Perforation Style", "Dark Hole matches scans with a black surround.", pst, 3, 0);
    static const char* psd[] = {"Format Default", "Left", "Right", "Both Sides"};
    cparam(desc, page, gG, P_PERF_SIDE, "Perforation Side", "Where the perforations are drawn.", psd, 4, 0);
    dparam(desc, page, gG, P_PERF_SIZE, "Perforation Size", "Multiplier of the real perforation size.", 1.0, 0.1, 4.0, 0.5, 2.0);
    dparam(desc, page, gG, P_PERF_OPACITY, "Perforation Opacity", "", 1.0, 0.0, 1.0, 0.0, 1.0);
    dparam(desc, page, gG, P_DUST, "Dust", "Dust specks on the film (same physical size on every gauge).", 0.15, 0.0, 10.0, 0.0, 2.0);
    dparam(desc, page, gG, P_FIBRES, "Fibres & Hairs", "Curved fibres on the film.", 0.04, 0.0, 10.0, 0.0, 2.0);
    dparam(desc, page, gG, P_GATE_HAIR, "Hair In Gate", "A hair caught in the camera gate: fixed to the gate edge.", 0.0, 0.0, 1.0, 0.0, 1.0);
    dparam(desc, page, gG, P_SCRATCHES, "Scratches", "Continuous vertical scratches along the film.", 0.06, 0.0, 2.0, 0.0, 1.0);
    static const char* pol[] = {"Auto (from emulsion)", "Dark", "Bright"};
    cparam(desc, page, gG, P_DIRT_POL, "Dirt Polarity", "Dust is bright on negative scans and dark on reversal or prints.", pol, 3, 0);

    // ---------------- weave
    GroupParamDescriptor* gW = group(desc, page, "grpWeave", "Gate Weave, Jitter & Flicker", false);
    bparam(desc, page, gW, P_WEAVE_ON, "Enable Gate Weave", "Mechanical instability of the camera and scanner.", true);
    std::vector<const char*> wp = labels(kWeavePresets, kWeavePresetCount);
    cparam(desc, page, gW, P_WEAVE_PRESET, "Weave Preset", "Camera movement types (estimates ordered by mechanism, see documentation).",
           wp.data(), (int)wp.size(), 5);
    dparam(desc, page, gW, P_WEAVE_AMOUNT, "Weave Amount", "Master multiplier of all movements.", 1.0, 0.0, 20.0, 0.0, 5.0);
    dparam(desc, page, gW, P_WEAVE_H, "Horizontal Weave (µm)", "Side-to-side registration error, micrometres rms on the film.", 8.0, 0.0, 500.0, 0.0, 60.0);
    dparam(desc, page, gW, P_WEAVE_V, "Vertical Weave (µm)", "Up-down registration error (pull-down), micrometres rms on the film.", 18.0, 0.0, 500.0, 0.0, 100.0);
    dparam(desc, page, gW, P_WEAVE_RANDOM, "Weave Randomness", "0 = smooth slow drift, 1 = independent jump every frame.", 0.65, 0.0, 1.0, 0.0, 1.0);
    dparam(desc, page, gW, P_WEAVE_CYCLE, "Drift Cycle (frames)", "Length of the slow drift in frames (frequency).", 6.0, 1.0, 200.0, 1.0, 48.0);
    dparam(desc, page, gW, P_ROTATION, "Rotational Weave (°)", "Small rotation of the picture, degrees rms.", 0.05, 0.0, 5.0, 0.0, 0.5);
    dparam(desc, page, gW, P_SCAN_JITTER, "Scanner Jitter (µm)", "Movement of the whole frame, gate and perforations included.", 2.0, 0.0, 500.0, 0.0, 30.0);
    dparam(desc, page, gW, P_FLICKER, "Exposure Flicker (stops)", "Frame-to-frame exposure change, stops rms.", 0.10, 0.0, 2.0, 0.0, 0.5);
    dparam(desc, page, gW, P_FLICKER_RANDOM, "Flicker Randomness", "0 = slow pumping, 1 = frame-to-frame.", 0.6, 0.0, 1.0, 0.0, 1.0);
    iparam(desc, page, gW, P_WEAVE_SEED, "Weave Seed", "Changes the random pattern.", 0, 0, 100000);

    // ---------------- grain
    GroupParamDescriptor* gR = group(desc, page, "grpGrain", "Film Grain", false);
    bparam(desc, page, gR, P_GRAIN_ON, "Enable Film Grain", "Structured, density-dependent emulsion grain.", true);
    std::vector<const char*> rp = labels(kGrainPresets, kGrainPresetCount);
    cparam(desc, page, gR, P_GRAIN_PRESET, "Grain Preset", "Film stocks. Labels say whether the RMS value is published or estimated.",
           rp.data(), (int)rp.size(), 7);
    static const char* emu[] = {"Colour Negative", "Colour Reversal", "B&W Reversal", "B&W Negative"};
    cparam(desc, page, gR, P_EMULSION, "Emulsion Type", "Changes how grain varies with density and its colour structure.", emu, 4, 1);
    dparam(desc, page, gR, P_RMS, "RMS Granularity", "Diffuse RMS granularity x1000 at density 1.0, 48 µm aperture, as printed in Kodak data sheets.", 8.0, 0.0, 60.0, 0.0, 30.0);
    dparam(desc, page, gR, P_GRAIN_INT, "Grain Intensity", "Multiplier of the physical grain amplitude.", 1.0, 0.0, 20.0, 0.0, 4.0);
    dparam(desc, page, gR, P_GRAIN_SIZE, "Grain Size", "Size of the dye clouds / silver clumps.", 1.0, 0.1, 10.0, 0.3, 3.0);
    dparam(desc, page, gR, P_SHADOW, "Shadow Response", "Grain multiplier in the shadows.", 1.0, 0.0, 10.0, 0.0, 3.0);
    dparam(desc, page, gR, P_MID, "Midtone Response", "Grain multiplier in the midtones.", 1.0, 0.0, 10.0, 0.0, 3.0);
    dparam(desc, page, gR, P_HIGH, "Highlight Response", "Grain multiplier in the highlights.", 1.0, 0.0, 10.0, 0.0, 3.0);
    dparam(desc, page, gR, P_COLOR_GRAIN, "Colour Grain", "Independence of the three dye layers: 0 = monochrome grain, 1 = fully independent.", 0.85, 0.0, 1.0, 0.0, 1.0);
    dparam(desc, page, gR, P_GRAIN_CLUMP, "Grain Clumping", "How much of the grain sits in large clumps: low = fine even texture, high = coarse organic clumps.", 0.40, 0.0, 1.0, 0.0, 1.0);
    dparam(desc, page, gR, P_GRAIN_SOFT, "Grain Softness", "Scanner / print softness of the grain. 0 = razor sharp (digital looking), 1 = typical film scan.", 1.0, 0.0, 5.0, 0.0, 3.0);
    dparam(desc, page, gR, P_NEG_GAMMA, "Negative Gamma", "Contrast of the negative characteristic curve (negative emulsions only).", 0.60, 0.2, 1.5, 0.4, 1.0);
    iparam(desc, page, gR, P_GRAIN_SEED, "Grain Seed", "Changes the grain pattern.", 0, 0, 100000);

    // ---------------- light leaks
    GroupParamDescriptor* gL = group(desc, page, "grpLeaks", "Light Leaks", false);
    bparam(desc, page, gL, P_LEAK_ON, "Enable Light Leaks", "Light leaking around the perforations and the gate edges.", true);
    std::vector<const char*> lp = labels(kLeakPresets, kLeakPresetCount);
    cparam(desc, page, gL, P_LEAK_PRESET, "Light Leak Preset", "Loads a light leak look into the controls below.", lp.data(), (int)lp.size(), 1);
    dparam(desc, page, gL, P_LEAK_OPACITY, "Opacity", "", 1.0, 0.0, 5.0, 0.0, 2.0);
    static const char* bl[] = {"Screen", "Add", "Lighten", "Soft Light"};
    cparam(desc, page, gL, P_LEAK_BLEND, "Blend Mode", "", bl, 4, 0);
    RGBParamDescriptor* col = desc.defineRGBParam(P_LEAK_COLOR);
    col->setLabels("Colour Tint", "Colour Tint", "Colour Tint");
    col->setHint("Colour of the leaked light.");
    col->setDefault(1.0, 0.28, 0.04);
    col->setParent(*gL);
    page->addChild(*col);
    dparam(desc, page, gL, P_LEAK_FREQ, "Frequency", "How fast the leaks change over time.", 0.3, 0.0, 20.0, 0.0, 4.0);
    dparam(desc, page, gL, P_LEAK_SPREAD, "Size", "Size of glows and flares.", 1.0, 0.05, 20.0, 0.2, 5.0);
    dparam(desc, page, gL, P_PERF_GLOW, "Perforation Glow", "Warm glow around the sprocket holes.", 0.9, 0.0, 10.0, 0.0, 3.0);
    dparam(desc, page, gL, P_EDGE_GLOW, "Gate Edge Glow", "Warm rim along the gate edges.", 0.7, 0.0, 10.0, 0.0, 3.0);
    dparam(desc, page, gL, P_FLARES, "Warm Flares", "Flares entering from the sides of the frame.", 0.0, 0.0, 10.0, 0.0, 3.0);
    dparam(desc, page, gL, P_BURN, "Organic Burn", "Patchy burns from fogged film.", 0.0, 0.0, 10.0, 0.0, 3.0);
    iparam(desc, page, gL, P_LEAK_SEED, "Leak Seed", "Changes the leak pattern.", 0, 0, 100000);
}

ImageEffect* PsdvFilmGateFactory::createInstance(OfxImageEffectHandle handle, ContextEnum /*context*/)
{
    return new PsdvFilmGate(handle);
}

namespace OFX {
namespace Plugin {
void getPluginIDs(OFX::PluginFactoryArray& ids)
{
    static PsdvFilmGateFactory p(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor);
    ids.push_back(&p);
}
} // namespace Plugin
} // namespace OFX
