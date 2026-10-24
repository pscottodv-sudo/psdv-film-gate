// Test host parameters: values live in a global map (name -> components);
// unset parameters return their descriptor default. Based on the OpenFX hostDemo
// (BSD-3-Clause), rewritten so that get/set really work.
#include <map>
#include <string>
#include <vector>
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxhPropertySuite.h"
#include "ofxhParam.h"
#include "ofxhImageEffect.h"
#include "hostDemoHostDescriptor.h"
#include "hostDemoEffectInstance.h"
#include "hostDemoParamInstance.h"

std::map<std::string, std::vector<double>> gParamValues;

namespace MyHost {
static double defD(OFX::Host::Param::Descriptor& d, int i) { return d.getProperties().getDoubleProperty(kOfxParamPropDefault, i); }
static int defI(OFX::Host::Param::Descriptor& d, int i) { return d.getProperties().getIntProperty(kOfxParamPropDefault, i); }
static bool have(const std::string& n) { return gParamValues.count(n) != 0; }

MyPushbuttonInstance::MyPushbuttonInstance(MyEffectInstance* e, const std::string& n, OFX::Host::Param::Descriptor& d)
  : OFX::Host::Param::PushbuttonInstance(d, e), _effect(e), _descriptor(d) {}

#define CTOR(C, BASE) C::C(MyEffectInstance* e, const std::string& n, OFX::Host::Param::Descriptor& d) : OFX::Host::Param::BASE(d, e), _effect(e), _descriptor(d) {}
CTOR(MyIntegerInstance, IntegerInstance)
CTOR(MyDoubleInstance, DoubleInstance)
CTOR(MyBooleanInstance, BooleanInstance)
CTOR(MyChoiceInstance, ChoiceInstance)
CTOR(MyRGBAInstance, RGBAInstance)
CTOR(MyRGBInstance, RGBInstance)
CTOR(MyDouble2DInstance, Double2DInstance)
CTOR(MyInteger2DInstance, Integer2DInstance)

#define NAME _descriptor.getName()
OfxStatus MyIntegerInstance::get(int& v) { v = have(NAME) ? (int)gParamValues[NAME][0] : defI(_descriptor, 0); return kOfxStatOK; }
OfxStatus MyIntegerInstance::get(OfxTime, int& v) { return get(v); }
OfxStatus MyIntegerInstance::set(int v) { gParamValues[NAME] = {(double)v}; return kOfxStatOK; }
OfxStatus MyIntegerInstance::set(OfxTime, int v) { return set(v); }

OfxStatus MyDoubleInstance::get(double& v) { v = have(NAME) ? gParamValues[NAME][0] : defD(_descriptor, 0); return kOfxStatOK; }
OfxStatus MyDoubleInstance::get(OfxTime, double& v) { return get(v); }
OfxStatus MyDoubleInstance::set(double v) { gParamValues[NAME] = {v}; return kOfxStatOK; }
OfxStatus MyDoubleInstance::set(OfxTime, double v) { return set(v); }
OfxStatus MyDoubleInstance::derive(OfxTime, double& v) { v = 0; return kOfxStatOK; }
OfxStatus MyDoubleInstance::integrate(OfxTime t1, OfxTime t2, double& v) { double x; get(x); v = x * (t2 - t1); return kOfxStatOK; }

OfxStatus MyBooleanInstance::get(bool& v) { v = have(NAME) ? gParamValues[NAME][0] != 0 : defI(_descriptor, 0) != 0; return kOfxStatOK; }
OfxStatus MyBooleanInstance::get(OfxTime, bool& v) { return get(v); }
OfxStatus MyBooleanInstance::set(bool v) { gParamValues[NAME] = {v ? 1.0 : 0.0}; return kOfxStatOK; }
OfxStatus MyBooleanInstance::set(OfxTime, bool v) { return set(v); }

OfxStatus MyChoiceInstance::get(int& v) { v = have(NAME) ? (int)gParamValues[NAME][0] : defI(_descriptor, 0); return kOfxStatOK; }
OfxStatus MyChoiceInstance::get(OfxTime, int& v) { return get(v); }
OfxStatus MyChoiceInstance::set(int v) { gParamValues[NAME] = {(double)v}; return kOfxStatOK; }
OfxStatus MyChoiceInstance::set(OfxTime, int v) { return set(v); }

OfxStatus MyRGBAInstance::get(double& r, double& g, double& b, double& a)
{
    if (have(NAME)) { auto& v = gParamValues[NAME]; r = v[0]; g = v[1]; b = v[2]; a = v[3]; }
    else { r = defD(_descriptor, 0); g = defD(_descriptor, 1); b = defD(_descriptor, 2); a = defD(_descriptor, 3); }
    return kOfxStatOK;
}
OfxStatus MyRGBAInstance::get(OfxTime, double& r, double& g, double& b, double& a) { return get(r, g, b, a); }
OfxStatus MyRGBAInstance::set(double r, double g, double b, double a) { gParamValues[NAME] = {r, g, b, a}; return kOfxStatOK; }
OfxStatus MyRGBAInstance::set(OfxTime, double r, double g, double b, double a) { return set(r, g, b, a); }

OfxStatus MyRGBInstance::get(double& r, double& g, double& b)
{
    if (have(NAME)) { auto& v = gParamValues[NAME]; r = v[0]; g = v[1]; b = v[2]; }
    else { r = defD(_descriptor, 0); g = defD(_descriptor, 1); b = defD(_descriptor, 2); }
    return kOfxStatOK;
}
OfxStatus MyRGBInstance::get(OfxTime, double& r, double& g, double& b) { return get(r, g, b); }
OfxStatus MyRGBInstance::set(double r, double g, double b) { gParamValues[NAME] = {r, g, b}; return kOfxStatOK; }
OfxStatus MyRGBInstance::set(OfxTime, double r, double g, double b) { return set(r, g, b); }

OfxStatus MyDouble2DInstance::get(double& a, double& b) { a = defD(_descriptor, 0); b = defD(_descriptor, 1); return kOfxStatOK; }
OfxStatus MyDouble2DInstance::get(OfxTime, double& a, double& b) { return get(a, b); }
OfxStatus MyDouble2DInstance::set(double, double) { return kOfxStatOK; }
OfxStatus MyDouble2DInstance::set(OfxTime, double, double) { return kOfxStatOK; }
OfxStatus MyInteger2DInstance::get(int& a, int& b) { a = defI(_descriptor, 0); b = defI(_descriptor, 1); return kOfxStatOK; }
OfxStatus MyInteger2DInstance::get(OfxTime, int& a, int& b) { return get(a, b); }
OfxStatus MyInteger2DInstance::set(int, int) { return kOfxStatOK; }
OfxStatus MyInteger2DInstance::set(OfxTime, int, int) { return kOfxStatOK; }
} // namespace MyHost
