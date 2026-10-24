# Rapporto di test - psdv-film gate emulation 1.0

Ambiente di test: Linux x86-64, g++ 13 / clang 18, CMake 3.28, OpenFX SDK commit c3f1c2d. **Nessun Mac, nessuna GPU, nessun DaVinci Resolve.**

## Superati (eseguiti qui)

| Test | Risultato |
|---|---|
| Build Linux del plugin da zero (download dell'OpenFX SDK incluso) | OK, 0 warning nel codice del plugin |
| Simboli esportati dal binario | solo `OfxGetNumberOfPlugins` / `OfxGetPlugin` |
| Caricamento in un **host OpenFX reale** (HostSupport ufficiale, immagini float RGBA) | il plugin viene caricato: 75 parametri, 5 gruppi, id `it.psdv.filmgate` |
| Accesso temporale tramite l'host | frame 24 → l'host fornisce 23, 24, 25; agli estremi della clip si usa il frame corrente |
| Preset → slider; modifica manuale → Custom | OK (es. Grain Preset 5 → Emulsion 3, RMS 14) |
| Test numerici del core (`tests/run_tests.py`) | **30/30** - dettaglio in `test_results.json` |
| · identità con tutte le sezioni spente (3 encoding) | errore relativo ≤ 1.2·10⁻⁴ (arrotondamento delle coordinate float; < ¼ di un codice a 12 bit) |
| · grana RMS a 48 µm | ingresso 8 → misurato 7.81 (modello 7.93) |
| · legge dell'apertura 3-86 µm | scarto massimo 1.8% |
| · Double-X 5222 | 14.07 misurato contro 14.35 atteso alla densità del grigio |
| · grana nuova a ogni frame, deterministica | correlazione t/t+1 = 0.0006 |
| · Fit Gate To Screen a 1920×1080, 1440×1080, 1080×1920 | centrato al pixel, nero assoluto fuori dal gate |
| · fotogramma sopra = precedente, sotto = successivo; Off = nero | OK |
| · weave in µm riprodotto; perforazioni ferme mentre l'immagine balla | OK |
| · 324 combinazioni (formati × vicini × emulsioni × stili di perforazione), valori estremi, 4 risoluzioni | nessun NaN/Inf |
| Kernel Metal incluso: controllo **sintattico** con clang e una `metal_stdlib` simulata | OK - **non è una compilazione Metal reale** |
| Script shell (`bash -n`), YAML del workflow | sintassi OK |

## Superati su Mac (GitHub Actions, workflow `metal-test`)

Run del 24-10-2026 su runner Apple Silicon. Risultati completi nel branch `ci-results/macos-15` (`out/report.md`).

| Test | macOS 14 | macOS 15 |
|---|---|---|
| Build universale arm64 + x86_64, link Metal/Foundation, entry point OFX, firma ad-hoc | OK | OK |
| Compilazione **offline** del kernel con `xcrun metal` (+ `metallib`) | OK, 0 warning | OK, 0 warning |
| Compilazione **a runtime** con il codice del plugin (`newLibraryWithSource`, la stessa via di Resolve) | - (il runner non ha GPU Metal) | OK, 1.6 s al primo uso |
| Render Metal tramite `PsdvRunMetal` confrontato con la CPU, 29 casi (9 formati, 4 emulsioni, leaks, perforazioni, zoom/pan, estremi, verticale, 4K, 1001×777) | - | **29/29**, errore max 2.3·10⁻⁵, medio ~10⁻⁸, nessun NaN |
| Identità con tutte le sezioni spente (GPU contro ingresso) | - | errore max 5.5·10⁻⁶ |
| Caricamento del binario Mac in un host OFX + render CPU con t±1 | OK | OK |
| Creazione del `.pkg` (pkgbuild + productbuild) | - | OK |

Tempi sulla GPU virtualizzata del runner ("Apple Paravirtual device", molto più lenta di un Mac reale): 1080p circa 35 ms/frame, 4K circa 140 ms/frame. CPU dello stesso runner: circa 0.7 s e 3 s.

## Versione 1.1.0 (run GitHub 40130737602)

- GPU contro CPU su macOS 15: **29/29**. Con fast math restano differenze isolate su 3–6 pixel di bordo antialiasing (perforazioni/gate) e uno scarto medio di 1.7·10⁻³ solo nel caso di stress con valori estremi; con Metal in modalità precisa lo scarto massimo è 2·10⁻⁵ ovunque.
- Tempi sulla GPU virtualizzata del runner, 1080p con i default: **8 ms/frame** (1.0: 36 ms). Modalità precisa: 25 ms.
- Grana v2: RMS a 48 µm 7.90 contro 7.93, legge dell'apertura entro 0.7%, Double-X 14.33 contro 14.38.
- Fotogrammi vicini: in 150 inquadrature di prova nessun vicino visibile viene saltato; 180 richieste su 300 risparmiate.

## Non ancora eseguiti - da fare sul tuo Mac

1. Caricamento in **DaVinci Resolve**, comparsa in OpenFX › PSDV, comportamento dell'interfaccia (gruppi, preset, sincronizzazione con Custom).
2. Accesso temporale (t±1) **insieme al render Metal** dentro Resolve, e formato dei buffer che Resolve passa al plugin.
3. Prestazioni su una GPU Apple reale.
4. Installazione del `.pkg` con Installer.app; firma e notarizzazione (servono certificati Apple Developer).

Prova consigliata dopo l'installazione: una clip Super 8 a 1080p, con il plugin con i default. Devi vedere il look di riferimento: foro scuro con bordo arancio e fotogrammi vicini sopra e sotto. Poi spegni le 4 sezioni: l'immagine deve risultare identica all'originale.
