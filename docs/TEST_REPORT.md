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

## Non eseguiti (impossibili qui) - da fare sul Mac

1. `./scripts/build_macos.sh`: build universale, link dei framework Metal e Foundation, firma ad-hoc.
2. **Compilazione Metal reale** del kernel (avviene al primo render dentro Resolve). Errori in `~/Library/Logs/psdv-film-gate.log`.
3. Caricamento in **DaVinci Resolve**, comparsa in OpenFX › PSDV, comportamento dell'interfaccia (gruppi, preset, sincronizzazione con Custom).
4. Accesso temporale (t±1) **insieme al render Metal** dentro Resolve.
5. Corrispondenza visiva CPU contro GPU e prestazioni GPU a 4K.
6. `make_installer_macos.sh`: pkgbuild/productbuild, installazione con Installer.app, firma e notarizzazione.

Prova consigliata dopo l'installazione: una clip Super 8 a 1080p, con il plugin con i default. Devi vedere il look di riferimento: foro scuro con bordo arancio e fotogrammi vicini sopra e sotto. Poi spegni le 4 sezioni: l'immagine deve risultare identica all'originale.
