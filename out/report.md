# GPU vs CPU - psdv-film gate emulation

Device: **Apple Paravirtual device**  
Runtime kernel compile: 945 ms

| Case | Size | max abs | mean abs | px > 0.01 | GPU NaN/Inf | GPU ms/frame | CPU ms | Result |
|---|---|---|---|---|---|---|---|---|
| defaults_super8_edge_leaks | 1920x1080 | 2.38e-02 | 1.84e-07 | 1 (0.0000%) | 0 | 9.0 | 344 | PASS |
| all_sections_off_identity | 1920x1080 | 2.26e-05 | 1.57e-07 | 0 (0.0000%) | 0 | 2.2 | 56 | PASS |
| format_0 | 1920x1080 | 2.20e-05 | 1.39e-07 | 0 (0.0000%) | 0 | 9.2 | 330 | PASS |
| format_1 | 1920x1080 | 2.38e-02 | 1.84e-07 | 1 (0.0000%) | 0 | 9.5 | 328 | PASS |
| format_2 | 1920x1080 | 2.08e-04 | 3.29e-07 | 0 (0.0000%) | 0 | 8.4 | 343 | PASS |
| format_3 | 1920x1080 | 2.01e-04 | 5.51e-07 | 0 (0.0000%) | 0 | 9.0 | 349 | PASS |
| format_4 | 1920x1080 | 3.68e-05 | 2.87e-07 | 0 (0.0000%) | 0 | 8.1 | 293 | PASS |
| format_5 | 1920x1080 | 1.45e-03 | 3.90e-07 | 0 (0.0000%) | 0 | 8.3 | 300 | PASS |
| format_6 | 1920x1080 | 5.99e-05 | 4.54e-07 | 0 (0.0000%) | 0 | 9.2 | 367 | PASS |
| format_7 | 1920x1080 | 5.16e-05 | 6.54e-07 | 0 (0.0000%) | 0 | 10.8 | 465 | PASS |
| format_8 | 1920x1080 | 8.53e-04 | 8.00e-07 | 0 (0.0000%) | 0 | 10.3 | 454 | PASS |
| grain_emulsion_0 | 1920x1080 | 2.63e-02 | 1.81e-07 | 1 (0.0000%) | 0 | 9.5 | 355 | PASS |
| grain_emulsion_1 | 1920x1080 | 2.38e-02 | 1.84e-07 | 1 (0.0000%) | 0 | 9.1 | 356 | PASS |
| grain_emulsion_2 | 1920x1080 | 6.79e-03 | 2.29e-07 | 0 (0.0000%) | 0 | 8.6 | 335 | PASS |
| grain_emulsion_3 | 1920x1080 | 8.81e-03 | 2.27e-07 | 0 (0.0000%) | 0 | 8.5 | 409 | PASS |
| perf_light_hot_white | 1920x1080 | 2.38e-02 | 1.84e-07 | 1 (0.0000%) | 0 | 9.0 | 373 | PASS |
| perf_light_unstable | 1920x1080 | 1.29e-04 | 1.53e-07 | 0 (0.0000%) | 0 | 9.0 | 368 | PASS |
| perf_light_16mm_both | 1920x1080 | 2.08e-04 | 3.29e-07 | 0 (0.0000%) | 0 | 7.7 | 322 | PASS |
| perforations_backlit_both | 1920x1080 | 2.08e-04 | 3.29e-07 | 0 (0.0000%) | 0 | 7.9 | 311 | PASS |
| perforations_outline | 1920x1080 | 2.38e-02 | 1.84e-07 | 1 (0.0000%) | 0 | 9.1 | 364 | PASS |
| picture_only_fit | 1920x1080 | 2.92e-04 | 4.18e-07 | 0 (0.0000%) | 0 | 10.2 | 356 | PASS |
| gate_off_with_grain_weave | 1920x1080 | 4.52e-05 | 9.00e-07 | 0 (0.0000%) | 0 | 6.6 | 298 | PASS |
| image_zoom_pan | 1920x1080 | 3.08e-02 | 2.72e-07 | 1 (0.0000%) | 0 | 9.0 | 340 | PASS |
| strong_weave_rotation | 1920x1080 | 3.90e-02 | 2.74e-07 | 1 (0.0000%) | 0 | 9.8 | 345 | PASS |
| dirt_heavy | 1920x1080 | 2.38e-02 | 1.88e-07 | 1 (0.0000%) | 0 | 9.8 | 374 | PASS |
| extremes | 1920x1080 | 5.56e-03 | 1.66e-03 | 0 (0.0000%) | 0 | 15.5 | 545 | PASS |
| vertical_1080x1920 | 1080x1920 | 1.45e-05 | 3.02e-07 | 0 (0.0000%) | 0 | 13.0 | 550 | PASS |
| uhd_3840x2160 | 3840x2160 | 1.50e-02 | 3.49e-07 | 1 (0.0000%) | 0 | 34.7 | 1610 | PASS |
| odd_size_1001x777 | 1001x777 | 1.91e-05 | 2.86e-07 | 0 (0.0000%) | 0 | 4.6 | 185 | PASS |

**29 / 29 cases passed.**
