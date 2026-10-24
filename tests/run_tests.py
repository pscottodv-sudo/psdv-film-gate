#!/usr/bin/env python3
"""
run_tests.py - numerical tests of the psdv-film gate emulation core (CPU reference).
These check the maths shared by the CPU and Metal paths. They do not prove that
Resolve accepts the plugin or that the Metal kernel compiles on macOS.

Usage: python3 tests/run_tests.py BUILD_DIR [report.json]
"""
import json
import math
import os
import subprocess
import sys
import tempfile
import time

import numpy as np

BUILD = sys.argv[1] if len(sys.argv) > 1 else "build"
RENDER = os.path.join(BUILD, "psdv_render")
RESULTS = []
OFF = dict(grainOn=0, weaveOn=0, leakOn=0, dust=0, fibres=0, scratches=0, gateHair=0, edgeDarkness=0, opticalSoft=0)


def record(name, ok, detail):
    RESULTS.append(dict(test=name, ok=bool(ok), detail=detail))
    print(("PASS " if ok else "FAIL ") + name + " :: " + detail, flush=True)


def render(img, params=None, prev=None, nxt=None):
    """img: (H, W, 3) linear float, row 0 = bottom. Returns (H, W, 3) output encoding values."""
    h, w, _ = img.shape
    with tempfile.TemporaryDirectory() as td:
        def save(a, n):
            rgba = np.dstack([a, np.ones(a.shape[:2], np.float32)]).astype(np.float32)
            path = os.path.join(td, n)
            rgba.tofile(path)
            return path
        args = [RENDER, save(img, "in.f32"), os.path.join(td, "out.f32"), str(w), str(h)]
        if prev is not None: args.append("prev=" + save(prev, "p.f32"))
        if nxt is not None: args.append("next=" + save(nxt, "n.f32"))
        args += [f"{k}={v}" for k, v in (params or {}).items()]
        r = subprocess.run(args, capture_output=True, text=True)
        if r.returncode not in (0, 5):
            raise RuntimeError(r.stderr)
        out = np.fromfile(os.path.join(td, "out.f32"), np.float32).reshape(h, w, 4)[..., :3]
        nf = int(r.stderr.strip().split("=")[-1])
        return out, nf


# encoding helpers (DaVinci Intermediate)
def di_enc(e):
    e = np.asarray(e, np.float64)
    return np.where(e <= 0.00262409, e * 10.44426855, (np.log2(np.maximum(e + 0.0075, 1e-12)) + 7.0) * 0.07329248).astype(np.float32)


def di_dec(v):
    v = np.asarray(v, np.float64)
    return np.where(v <= 0.02740668, v / 10.44426855, np.exp2(v / 0.07329248 - 7.0) - 0.0075)


def scene(w, h, n=0):
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    u, v = (x + 0.5) / w, (y + 0.5) / h
    img = np.zeros((h, w, 3), np.float32)
    img[..., 0] = 0.2 + 0.5 * v; img[..., 1] = 0.25 + 0.6 * v; img[..., 2] = 0.3 + 0.8 * v
    m = v < 0.3
    bits = np.minimum((u * 12).astype(int), 11)
    val = ((n >> bits) & 1).astype(np.float32)
    for c in range(3):
        img[..., c] = np.where(m, val, img[..., c])
    sun = ((u - 0.3) * w / h) ** 2 + (v - 0.75) ** 2 < 0.004
    img[sun] = 5.0
    return img


# ---------------------------------------------------------------- grain model mirror
def block_var(a, l):
    t = a / l
    if t < 1e-3: return 1.0
    vv = (math.sqrt(math.pi) * t * math.erf(t) + math.exp(-t * t) - 1) / (t * t)
    return vv * vv


def test_identity():
    sc = scene(480, 270)
    for enc, conv in ((0, di_enc), (1, lambda e: np.clip(e, 0, None) ** (1 / 2.4)), (2, lambda e: e)):
        src = conv(sc).astype(np.float32)
        out, nf = render(src, dict(OFF, gateOn=0, encoding=enc))
        err = float((np.abs(out - src) / np.maximum(np.abs(src), 0.05)).max())
        record(f"identity: every section off, encoding {enc}", err < 2e-4 and nf == 0, f"max relative error {err:.1e} (float coordinate round-trip; < 1/4 of a 12-bit code)")


def test_grain_calibration():
    W, H = 3840, 2160
    src = di_enc(np.full((H, W, 3), 0.18, np.float32))
    p = dict(OFF, grainOn=1, gateOn=0, emulsion=2, rms=8.0, grainSize=1.0, frameIndex=10)
    out, nf = render(src, p)
    E = di_dec(out[..., 1])
    dx = np.log10(np.maximum(E, 1e-9) / 0.18)
    u = 10 ** (0.95 * 0.455); gam = 3.15 * 0.95 * math.log(10) * u / (1 + u) ** 2
    dD = -dx * gam
    mm = min(5.69 / W, 4.22 / H)
    B = int(round(math.sqrt(math.pi) * 24.0 / (mm * 1000)))
    h2, w2 = (H // B) * B, (W // B) * B
    s48 = dD[:h2, :w2].reshape(h2 // B, B, w2 // B, B).mean(axis=(1, 3)).std()
    l1, l2 = 0.5 * 0.0065, 0.5 * 0.015
    aref = math.sqrt(math.pi) * 0.024
    pred = 0.008 * math.sqrt(0.5 * block_var(B * mm, l1) / block_var(aref, l1) + 0.5 * block_var(B * mm, l2) / block_var(aref, l2))
    record("grain: RMS measured through a 48 um aperture equals the data-sheet value", abs(s48 / pred - 1) < 0.04 and nf == 0,
           f"input RMS 8.0 -> measured {s48*1000:.2f} at {B*mm*1000:.1f} um (model {pred*1000:.2f})")
    rows, worst = [], 0
    for B in (2, 4, 8, 16, 29, 58):
        h2, w2 = (H // B) * B, (W // B) * B
        s = dD[:h2, :w2].reshape(h2 // B, B, w2 // B, B).mean(axis=(1, 3)).std()
        pr = 0.008 * math.sqrt(0.5 * block_var(B * mm, l1) / block_var(aref, l1) + 0.5 * block_var(B * mm, l2) / block_var(aref, l2))
        worst = max(worst, abs(s / pr - 1)); rows.append(f"{B*mm*1000:.0f}um {s*1000:.2f}/{pr*1000:.2f}")
    record("grain: aperture law (Selwyn) across apertures", worst < 0.06, ", ".join(rows) + f"; worst {worst*100:.1f}%")


def test_grain_double_x():
    """published preset: Double-X 5222 RMS 14, BW negative, gamma 0.65"""
    W, H = 3840, 2160
    src = di_enc(np.full((H, W, 3), 0.18, np.float32))
    p = dict(OFF, grainOn=1, gateOn=0, emulsion=3, rms=14.0, grainSize=1.2, negGamma=0.65, frameIndex=3)
    out, _ = render(src, p)
    E = di_dec(out[..., 1])
    x = 0.0
    e = 10 ** min(1.5 * (x + 1.5), 30); gam = 0.65 * e / (1 + e)
    dD = np.log10(np.maximum(E, 1e-9) / 0.18) * gam
    mm = min(5.69 / W, 4.22 / H)
    B = int(round(math.sqrt(math.pi) * 24.0 / (mm * 1000)))
    h2, w2 = (H // B) * B, (W // B) * B
    s48 = dD[:h2, :w2].reshape(h2 // B, B, w2 // B, B).mean(axis=(1, 3)).std()
    D = 0.15 + 0.65 * math.log10(1 + e) / 1.5
    g = math.sqrt(max(D, 0.12) * math.exp(-(max(D, 0.12) - 1) / 2.5))
    l1, l2 = 0.5 * 0.0065 * 1.2, 0.5 * 0.015 * 1.2
    aref = math.sqrt(math.pi) * 0.024
    pred = 0.014 * g * math.sqrt(0.5 * block_var(B * mm, l1) / block_var(aref, l1) + 0.5 * block_var(B * mm, l2) / block_var(aref, l2))
    record("grain: Double-X 5222 preset reproduces RMS 14 (scaled to the density of grey)", abs(s48 / pred - 1) < 0.05,
           f"measured {s48*1000:.2f}, model {pred*1000:.2f} (D at grey {D:.2f}; RMS 14 is quoted at D = 1.0)")


def test_grain_temporal():
    src = di_enc(np.full((270, 480, 3), 0.18, np.float32))
    p = dict(OFF, grainOn=1, gateOn=0)
    a, _ = render(src, dict(p, frameIndex=100)); b, _ = render(src, dict(p, frameIndex=101)); c, _ = render(src, dict(p, frameIndex=100))
    r = np.corrcoef((a - src).ravel(), (b - src).ravel())[0, 1]
    record("grain: new pattern every frame, deterministic", abs(r) < 0.03 and np.abs(a - c).max() == 0, f"corr t/t+1 {r:+.4f}, repeat diff {np.abs(a-c).max():.1e}")


def test_framing():
    for W, H in ((1920, 1080), (1440, 1080), (1080, 1920)):
        white = np.ones((H, W, 3), np.float32)
        for fmt, gw, gh in ((1, 5.69, 4.22), (0, 4.90, 3.60), (2, 10.26, 7.49), (5, 24.89, 18.67)):
            out, nf = render(white, dict(OFF, encoding=2, format=fmt, gateFraming=1, neighbors=0, perfOn=0, edgeSoftness=0))
            m = out[..., 1] > 0.5
            ys, xs = np.nonzero(m)
            cx, cy = (xs.min() + xs.max() + 1) / 2, (ys.min() + ys.max() + 1) / 2
            mm = max(gw / W, gh / H)
            far = np.ones_like(m); far[max(ys.min() - 3, 0):ys.max() + 4, max(xs.min() - 3, 0):xs.max() + 4] = False
            blk = float(np.abs(out[far]).max()) if far.any() else 0.0
            ok = abs(cx - W / 2) <= 1 and abs(cy - H / 2) <= 1 and abs((xs.max() - xs.min() + 1) - gw / mm) <= 2.5 and blk == 0.0
            record(f"framing: Fit Gate To Screen, format {fmt}, {W}x{H}", ok and nf == 0,
                   f"picture {xs.max()-xs.min()+1}x{ys.max()-ys.min()+1}px centre ({cx},{cy}); outside max {blk}")


def test_neighbors():
    W, H = 1920, 1080
    n = 345
    cur, prev, nxt = scene(W, H, n), scene(W, H, n - 1), scene(W, H, n + 1)
    p = dict(OFF, encoding=2, frameSize=0.28, frameSpacing=0.0, imageFit=1, frameIndex=n, perfOn=0)
    out, _ = render(cur, p, prev, nxt)
    gw, gh, pitch = 5.69, 4.22, 4.234
    mm = gh / (0.28 * H); vS = (W / H) / (gw / gh)

    def dec(slot):
        v = 0
        for b in range(12):
            x = int(W / 2 + ((b + 0.5) / 12 - 0.5) * gw / mm)
            y = int(H / 2 + (slot * pitch + (0.15 - 0.5) / vS * gh) / mm)
            v |= (out[y, x, 1] > 0.5) << b
        return v
    got = [dec(1), dec(0), dec(-1)]
    record("neighbours: above = previous, centre = current, below = next", got == [n - 1, n, n + 1], f"read {got}")
    out, _ = render(cur, dict(p, neighbors=0), prev, nxt)
    y = int(H / 2 + pitch / mm)
    record("neighbours Off: frames above/below are black", float(out[y, W // 2].max()) == 0.0, f"value {out[y, W//2]}")


def test_weave_um():
    W, H = 1920, 1080
    white = np.ones((H, W, 3), np.float32)
    gh = 4.22
    for amp in (10.0, 40.0):
        ys = []
        for f in range(1, 31):
            out, _ = render(white, dict(OFF, encoding=2, weaveOn=1, weaveV=amp, weaveH=0, rotation=0, scanJitter=0, flicker=0,
                                         frameIndex=f, neighbors=0, perfOn=0, edgeSoftness=0))
            m = out[:, W // 2, 1] > 0.5
            idx = np.nonzero(m)[0]
            ys.append((idx.min() + idx.max() + 1) / 2)
        mm = gh / (0.76 * H)
        sd = np.std(ys) * mm * 1000
        record(f"gate weave: Vertical Weave {amp} um rms produces that movement on the film", 0.6 * amp < sd < 1.4 * amp,
               f"measured {sd:.1f} um rms over 30 frames")


def test_perforations_fixed():
    W, H = 1920, 1080
    white = np.ones((H, W, 3), np.float32)
    tops = []
    for f in range(1, 9):
        out, _ = render(white, dict(OFF, encoding=2, weaveOn=1, weaveV=40, scanJitter=0, frameIndex=f, perfStyle=1, neighbors=0))
        # perforation column (Super 8, film left edge) in screen coords: centre gate
        mm = 4.22 / (0.76 * H)
        x = int(W / 2 + (-7.976 / 2 + 0.51 + 0.457 - 0.327) / mm)
        col = out[:, x, 1] > 0.5
        idx = np.nonzero(col)[0]
        tops.append(idx.min() if idx.size else -1)
    record("perforations stay fixed while the picture weaves", len(set(tops)) == 1 and tops[0] >= 0, f"perforation top rows {set(tops)}")


def test_sweep():
    t0 = time.time()
    src = di_enc(scene(320, 180))
    bad = 0; count = 0
    for fmt in range(9):
        for nb in range(3):
            for emu in range(4):
                for style in range(3):
                    _, nf = render(src, dict(format=fmt, neighbors=nb, emulsion=emu, perfStyle=style, perfSide=(fmt + style) % 4,
                                             flares=1, burn=1, gateHair=1, leakBlend=(emu + nb) % 4, frameIndex=fmt * 13 + nb))
                    count += 1; bad += nf != 0
    record("sweep: 9 formats x 3 neighbour modes x 4 emulsions x 3 perforation styles", bad == 0, f"{count} renders, {bad} with NaN/Inf, {time.time()-t0:.0f}s")
    out, nf = render(di_enc(scene(1920, 1080)), dict(rms=60, grainIntensity=20, grainSize=10, shadowResponse=10, weaveH=500, weaveV=500,
                                                     rotation=5, scanJitter=500, flicker=2, dust=10, fibres=10, scratches=2, leakOpacity=5,
                                                     perfGlow=10, edgeGlow=10, flares=10, burn=10, zoom=8, imageZoom=8, frameSize=2))
    record("extreme values on every slider", nf == 0, f"nonfinite {nf}, range {out.min():.3f}..{out.max():.3f}")
    for W, H in ((1440, 1080), (3840, 2160), (1080, 1920), (720, 480)):
        _, nf = render(di_enc(scene(W, H)), dict(frameIndex=7))
        record(f"defaults render at {W}x{H}", nf == 0, f"nonfinite {nf}")


def main():
    t = time.time()
    for fn in (test_identity, test_grain_calibration, test_grain_double_x, test_grain_temporal, test_framing, test_neighbors,
               test_weave_um, test_perforations_fixed, test_sweep):
        fn()
    ok = sum(r["ok"] for r in RESULTS)
    print(f"\n{ok}/{len(RESULTS)} checks passed in {time.time()-t:.0f}s")
    if len(sys.argv) > 2:
        json.dump(RESULTS, open(sys.argv[2], "w"), indent=1)
    return 0 if ok == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
