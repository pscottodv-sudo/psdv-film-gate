// psdv_ofx_host - minimal OFX host (OpenFX HostSupport) used to test the real
// psdv-film-gate.ofx bundle: loads it, lists its parameters, simulates user edits
// and renders frames. Not DaVinci Resolve and no GPU: CPU render path only.
//
//   psdv_ofx_host --list
//   psdv_ofx_host [--size W H] [--range A B] [--source rgba.f32]
//                 [--set name=v[,v,v]]... [--edit name=v]... --frames 0,1,2 --out prefix
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxhPropertySuite.h"
#include "ofxhClip.h"
#include "ofxhParam.h"
#include "ofxhImageEffect.h"
#include "ofxhPluginAPICache.h"
#include "ofxhPluginCache.h"
#include "ofxhHost.h"
#include "ofxhImageEffectAPI.h"
#include "hostDemoHostDescriptor.h"
#include "hostDemoEffectInstance.h"
#include "hostDemoClipInstance.h"

extern std::map<std::string, std::vector<double>> gParamValues;

static std::vector<double> parseList(const std::string& s)
{
    std::vector<double> v;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) v.push_back(atof(item.c_str()));
    return v;
}

int main(int argc, char** argv)
{
    bool list = false;
    std::vector<std::pair<std::string, std::vector<double>>> edits;
    std::vector<double> frames;
    std::string out = "frame";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--list") list = true;
        else if (a == "--size") { gW = atoi(argv[++i]); gH = atoi(argv[++i]); }
        else if (a == "--range") { gFirst = atof(argv[++i]); gLast = atof(argv[++i]); }
        else if (a == "--frames") frames = parseList(argv[++i]);
        else if (a == "--out") out = argv[++i];
        else if (a == "--source") {
            std::ifstream f(argv[++i], std::ios::binary);
            gSourceOverride.resize((size_t)gW * gH * 4);
            f.read(reinterpret_cast<char*>(gSourceOverride.data()), gSourceOverride.size() * sizeof(float));
        } else if (a == "--set" || a == "--edit") {
            std::string kv = argv[++i];
            size_t eq = kv.find('=');
            std::string k = kv.substr(0, eq);
            std::vector<double> v = parseList(kv.substr(eq + 1));
            if (a == "--set") gParamValues[k] = v; else edits.push_back({k, v});
        }
    }
    OFX::Host::PluginCache::getPluginCache()->setCacheVersion("psdvTestHostV1");
    MyHost::Host host;
    OFX::Host::ImageEffect::PluginCache cache(host);
    cache.registerInCache(*OFX::Host::PluginCache::getPluginCache());
    OFX::Host::PluginCache::getPluginCache()->scanPluginFiles();
    OFX::Host::ImageEffect::ImageEffectPlugin* plugin = cache.getPluginById("it.psdv.filmgate");
    if (!plugin) { fprintf(stderr, "plugin it.psdv.filmgate not found (set OFX_PLUGIN_PATH)\n"); return 2; }
    std::unique_ptr<OFX::Host::ImageEffect::Instance> inst(plugin->createInstance(kOfxImageEffectContextFilter, NULL));
    if (!inst) { fprintf(stderr, "createInstance failed\n"); return 3; }
    OfxStatus st = inst->createInstanceAction();
    if (st != kOfxStatOK && st != kOfxStatReplyDefault) { fprintf(stderr, "createInstanceAction %d\n", st); return 3; }
    if (!inst->getClipPreferences()) { fprintf(stderr, "getClipPreferences failed\n"); return 3; }
    printf("plugin: %s  version %d.%d  grouping: %s\n", plugin->getIdentifier().c_str(), plugin->getVersionMajor(),
           plugin->getVersionMinor(), plugin->getDescriptor().getProps().getStringProperty(kOfxImageEffectPluginPropGrouping).c_str());

    if (list) {
        for (OFX::Host::Param::Instance* p : inst->getParamList()) {
            std::string type = p->getType();
            std::string extra;
            if (type == kOfxParamTypeChoice) {
                int n = p->getProperties().getDimension(kOfxParamPropChoiceOption);
                extra = " [" + std::to_string(n) + " options]";
            }
            printf("%-22s %-28s parent=%-10s label=\"%s\"%s\n", p->getName().c_str(), type.c_str() + 12,
                   p->getParentName().c_str(), p->getLabel().c_str(), extra.c_str());
        }
        return 0;
    }
    OfxPointD scale; scale.x = scale.y = 1.0;
    for (auto& e : edits) {     // simulate a user edit in the UI
        gParamValues[e.first] = e.second;
        inst->beginInstanceChangedAction(kOfxChangeUserEdited);
        st = inst->paramInstanceChangedAction(e.first, kOfxChangeUserEdited, 0.0, scale);
        inst->endInstanceChangedAction(kOfxChangeUserEdited);
        printf("edit %s -> status %d\n", e.first.c_str(), st);
    }
    for (auto& kv : gParamValues) {
        printf("value %s =", kv.first.c_str());
        for (double d : kv.second) printf(" %g", d);
        printf("\n");
    }
    if (frames.empty()) return 0;
    MyHost::MyClipInstance* outClip = dynamic_cast<MyHost::MyClipInstance*>(inst->getClip("Output"));
    MyHost::MyClipInstance* srcClip = dynamic_cast<MyHost::MyClipInstance*>(inst->getClip("Source"));
    OfxRectI win; win.x1 = win.y1 = 0; win.x2 = gW; win.y2 = gH;
    st = inst->beginRenderAction(frames.front(), frames.back(), 1.0, false, scale, true, false);
    for (double t : frames) {
        OFX::Host::ImageEffect::RangeMap rm;
        inst->getFrameNeededAction(t, rm);
        for (auto& r : rm)
            for (auto& rr : r.second) printf("frame %g needs %s [%g, %g]\n", t, r.first->getName().c_str(), rr.min, rr.max);
        srcClip->fetchedTimes.clear();
        st = inst->renderAction(t, kOfxImageFieldBoth, win, scale, true, false, false);
        printf("render %g -> status %d, fetched source times:", t, st);
        for (double ft : srcClip->fetchedTimes) printf(" %g", ft);
        printf("\n");
        if (st != kOfxStatOK) return 4;
        MyHost::MyImage* img = outClip->getOutputImage();
        std::string path = out + "." + std::to_string((int)t) + ".f32";
        FILE* f = fopen(path.c_str(), "wb");
        fwrite(img->pixel(0, 0), sizeof(float), (size_t)gW * gH * 4, f);
        fclose(f);
    }
    inst->endRenderAction(frames.front(), frames.back(), 1.0, false, scale, true, false);
    return 0;
}
