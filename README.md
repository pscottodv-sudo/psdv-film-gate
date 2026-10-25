# psdv-film gate emulation 1.3

Plugin **OpenFX** per **DaVinci Resolve su macOS** (rendering **Metal**, bundle universale Apple Silicon + Intel) che emula la finestra di ripresa di una cinepresa: gate e perforazioni, gate weave, grana di emulsione e luce delle perforazioni. Ogni sezione si attiva da sola, ha i suoi preset e un controllo manuale completo. L'interfaccia è in inglese.

> **Stato della verifica:** build universale, compilazione Metal (offline e a runtime) e render Metal sono verificati su Mac Apple Silicon di GitHub (macOS 15): **29/29 casi GPU identici alla CPU** (errore massimo 2·10⁻⁵). **Non è ancora stato provato dentro DaVinci Resolve.** Dettagli in [docs/TEST_REPORT.md](docs/TEST_REPORT.md).

---

## 1. Struttura del progetto

```
psdv-film-gate/
├── CMakeLists.txt                  build (scarica l'OpenFX SDK a un commit fissato)
├── src/
│   ├── PsdvFilmGate.cpp            plugin OFX: parametri, preset, accesso temporale, CPU/Metal
│   ├── core/
│   │   ├── psdv_prelude.h          astrazione C++ / Metal Shading Language
│   │   ├── psdv_params.h           struct dei parametri (identica su CPU e GPU)
│   │   ├── psdv_core.h             TUTTO il rendering: formati, gate, weave, grana, leaks
│   │   └── psdv_presets.h          tabelle dei preset, con marcatura [M] misurato / [E] stima
│   └── metal/
│       ├── psdv_kernel_body.metal  kernel GPU (chiama lo stesso core)
│       └── PsdvMetal.mm            dispatch Metal sulla command queue di Resolve
├── cmake/embed_kernel.cmake        inserisce il sorgente Metal nel binario
├── packaging/
│   ├── Info.plist.in
│   └── macos/                      distribution.xml, preinstall, testo di benvenuto
├── scripts/
│   ├── build_macos.sh              build universale + firma ad-hoc + controlli
│   ├── make_installer_macos.sh     crea dist/psdv-film-gate-1.0.0.pkg (firma/notarizzazione opzionali)
│   ├── install_local_macos.sh      copia il bundle in /Library/OFX/Plugins
│   ├── uninstall_macos.sh
│   └── build_linux_and_test.sh     build CPU + tutti i test automatici
├── ci/github-actions-build-macos.yml   MODELLO di workflow per un tuo nuovo repository
├── tests/                          renderer di riferimento, test numerici, host OFX di prova
└── docs/                           PRESETS.md, TEST_REPORT.md
```

Lo stesso file `psdv_core.h` viene compilato come C++ (percorso CPU e test) e come Metal (GPU). Ciò che i test verificano sulla CPU è quindi lo stesso codice che gira sulla GPU. Restano possibili differenze di compilazione Metal, che vanno controllate sul Mac.

Il kernel Metal viene compilato **al primo render**, partendo dal sorgente incluso nel plugin. Per questo **non serve Xcode completo**: bastano i Command Line Tools. Se la compilazione Metal fallisce, il render del nodo fallisce (Resolve mostra il nodo in errore) e il messaggio del compilatore viene scritto in `~/Library/Logs/psdv-film-gate.log`. In quel caso, come soluzione temporanea, imposta *Preferences › Memory and GPU › GPU processing mode* su **OpenCL**, se la tua versione lo offre: Resolve userà allora il percorso CPU del plugin.

## 2. Requisiti (Mac)

- macOS 11 o successivo, DaVinci Resolve 18 o successivo (la versione gratuita carica i plugin OFX).
- Command Line Tools: `xcode-select --install`
- CMake 3.20 o successivo: `brew install cmake` (oppure l'installer di cmake.org)
- Connessione a Internet al primo build, per scaricare l'OpenFX SDK (licenza BSD-3). In alternativa puoi passare un checkout locale con `OPENFX_DIR=/percorso/openfx`.

## 3. Build e installer (Terminale, dalla cartella del progetto)

```bash
chmod +x scripts/*.sh packaging/macos/scripts/preinstall

# a) compila il bundle universale (arm64 + x86_64) e lo firma ad-hoc
./scripts/build_macos.sh

# b1) installazione diretta sul tuo Mac
./scripts/install_local_macos.sh

# b2) oppure crea l'installer .pkg da distribuire
./scripts/make_installer_macos.sh        # -> dist/psdv-film-gate-1.0.0.pkg (+ .sha256)
```

`build_macos.sh` controlla da solo che il binario contenga entrambe le architetture e che esporti i due entry point OFX, e verifica la firma.

### Firma e notarizzazione (solo se distribuisci ad altri)

Senza certificati il `.pkg` non è firmato. Sul tuo Mac lo apri con **tasto destro › Apri**. Su altri Mac Gatekeeper lo blocca. Con un account Apple Developer:

```bash
xcrun notarytool store-credentials psdv-notary --apple-id TUO@ID --team-id TEAMID
CODESIGN_IDENTITY="Developer ID Application: Nome (TEAMID)" \
INSTALLER_IDENTITY="Developer ID Installer: Nome (TEAMID)" \
NOTARY_PROFILE=psdv-notary ./scripts/make_installer_macos.sh
```

### Build automatico su GitHub (facoltativo)

`ci/github-actions-build-macos.yml` è un **modello**: copialo in `.github/workflows/` di un **tuo nuovo** repository. Nessun repository esistente è stato toccato. Il workflow compila su un Mac di GitHub e allega il `.pkg` agli artifact. Questo è utile anche per avere un primo build Metal senza installare nulla.

### Disinstallazione

`./scripts/uninstall_macos.sh`, oppure cancella `/Library/OFX/Plugins/psdv-film-gate.ofx.bundle`.

## 4. Uso in DaVinci Resolve

1. Chiudi Resolve, installa, riapri.
2. Pagina **Color** › **Effects** › **OpenFX** › **PSDV** › **psdv-film gate emulation**. Trascinalo su un nodo, oppure in **Edit** su una clip.
3. **Input › Input Encoding** deve corrispondere a ciò che entra nel nodo: *DaVinci Intermediate* (pipeline DWG), *Rec.709 Gamma 2.4* oppure *Linear*. Serve a grana, flicker e leaks per lavorare in esposizione reale.
4. Per i fotogrammi veri sopra e sotto, il nodo deve stare **su una clip**, non su un compound o un generatore. Il plugin chiede a Resolve i frame t−1 e t+1. Agli estremi della clip, o se Resolve non li fornisce, usa il frame corrente.
5. Se il plugin non compare: *Preferences › Video Plugins*, controlla che sia attivo. Poi chiudi Resolve, cancella `~/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCache.xml` (Resolve lo ricrea) e riapri.

Ogni sezione ha **Enable**, un menu **Preset** e i controlli manuali. Scegliere un preset copia i valori negli slider. Se poi modifichi uno slider della stessa sezione, il menu torna su **Custom** e le tue modifiche restano.

### Film Gate & Perforations
| Controllo | Effetto |
|---|---|
| Gate Preset | Super 8 / Regular 8 / 16mm / 35mm scan, *Picture Only* (centrato, senza fori), Clean Gate |
| Film Format | R8, S8, 16, S16, 35 Academy, S35 4p/3p, 2p Techniscope, 65 5p: finestra, passo e perforazioni reali |
| Gate Framing | *Manual Frame Size*, *Fit Gate To Screen* (gate intero centrato con qualunque risoluzione della timeline), *Fill Screen With Picture* |
| Frame Size, Zoom, Position X/Y, Centre On | dimensione e posizione del gate |
| Gate Aspect Ratio, Custom Aspect Ratio | finestra nativa oppure 4:3, 1.37, 1.66, 16:9, 1.85, 2.39, personalizzata |
| Image In Gate, Image Zoom, Image Pan X/Y | posizione e zoom del video dentro la finestra |
| Frames Above / Below, Frame Spacing | fotogrammi vicini ripetuti (default, veloce) / veri (più lento: Resolve decodifica 3 frame) / neri, e distanza tra fotogrammi |
| Gate Edge Softness / Darkness / Darkness Width, Corner Roundness, Optical Softness | bordo della finestra e morbidezza ottica |
| Edge Irregularity (µm), Irregularity Size, Irregularity Motion | rientranze e sporgenze del contorno del gate; Motion 0 = contorno fisso, 1 = cambia a ogni fotogramma |
| Gate Defocus | gate fuori fuoco: bordi sfumati, angoli arrotondati e più scuri |
| Show Perforations, Perforation Style / Side / Size / Opacity | fori scuri, retroilluminati o solo contorno |
| Dust, Fibres & Hairs, Hair In Gate, Scratches, Dirt Polarity | sporco legato alla pellicola, pelo fermo nel gate, righe |

Con **Enable Film Gate** spento resta solo l'immagine a pieno schermo, con weave, grana e leaks se attivi.

### Gate Weave, Jitter & Flicker
Preset: 35mm pin-registered (classe Arricam/435), 16mm pin-registered (classe SR), **16mm Bolex H16** (solo griffa), Regular 8 Bolex H8, **Super 8 cartridge**, Super 8 usurata, telecinema da proiettore, Locked.
Controlli: *Weave Amount* (moltiplicatore generale), *Horizontal / Vertical Weave (µm)* (ampiezza in micrometri sulla pellicola), *Weave Randomness* (deriva lenta ↔ salto a ogni frame), *Drift Cycle* (frequenza, in frame), *Rotational Weave*, *Scanner Jitter*, *Exposure Flicker* (in stop), *Flicker Randomness*, *Weave Seed*.
Le ampiezze sono in µm sulla pellicola, quindi lo stesso valore muove più pixel su Super 8 che su 35mm, come accade nella realtà.

### Film Grain
Preset: Vision3 50D / 250D / 200T / 500T, **Double-X 5222 (RMS 14, dato pubblicato)**, Tri-X 7266, Ektachrome 100D (RMS 8 di E100).
Controlli: *Emulsion Type*, *RMS Granularity* (valore del datasheet Kodak a D 1.0, apertura 48 µm), *Grain Intensity*, *Grain Size*, *Grain Clumping* (fine e uniforme ↔ ammassi organici), *Grain Softness* (morbidezza di scanner/stampa), *Shadow / Midtone / Highlight Response*, *Colour Grain*, *Negative Gamma*, *Grain Seed*.
La grana è legata alla pellicola in mm. Un Super 8 mostra quindi più grana di un 35mm con lo stesso stock, e il fotogramma sopra ha la stessa grana che aveva quando era al centro.

### Perforation Light
È la luce del retroilluminatore dello scanner che passa dal foro della perforazione e si diffonde (halation) nell'emulsione intorno: in una scansione negativa il foro è nero e il bordo brilla, dal bianco-giallo al rosso passando per l'arancio. È **sempre presente** quando ci sono le perforazioni e **non è mai statica**: intensità e lato più caldo cambiano da un fotogramma all'altro.
Preset: **Negative Scan Halation** (default), Soft Warm Glow, Hot White Edge, Deep Red Burn, Subtle Edge, Unstable Backlight, Cool Fringe.
Controlli: *Exposure (EV)* (forza della luce, in stop, come l'Exposure dell'Overscan di Dehancer), *Rim Width*, *Glow Size*, *Core / Glow / Halo Colour*, *Chromatic Fringe* (frangia fredda sul lato in ombra), *Variation* (0 = statica), *Variation Speed* (deriva lenta ↔ ogni frame), *Hot Side* (uniforme ↔ un lato molto più luminoso), *Light Seed*.

## 5. Problemi noti e soluzioni

| Sintomo | Causa / rimedio |
|---|---|
| Il plugin non appare | Controlla che il percorso sia `/Library/OFX/Plugins/psdv-film-gate.ofx.bundle/Contents/MacOS/psdv-film-gate.ofx`, poi svuota la cache OFX (punto 4.5) |
| "is damaged" / bloccato | Build non notarizzata: `sudo xattr -dr com.apple.quarantine /Library/OFX/Plugins/psdv-film-gate.ofx.bundle` |
| Nodo in errore / immagine nera | Il kernel Metal non si è compilato su questo Mac: leggi `~/Library/Logs/psdv-film-gate.log` e inviami il contenuto |
| Fotogrammi vicini uguali al corrente | Resolve non ha fornito t±1 (inizio/fine clip, compound, generatore) |
| Colori o grana strani | *Input Encoding* non corrisponde a ciò che entra nel nodo |

## 6. Limiti dichiarati

- La grana è un modello fisico calibrato sui valori RMS pubblicati. **Non** deriva da scansioni reali. Solo Double-X 5222 (RMS 14) ed E100 (RMS 8) sono dati letti su datasheet. Gli altri valori sono stime: vedi [docs/PRESETS.md](docs/PRESETS.md).
- I costruttori non pubblicano valori numerici di stabilità per le cineprese. I preset di weave sono stime, ordinate secondo il meccanismo di registrazione.
- Supporta solo immagini float RGBA, che è il formato in cui Resolve lavora. Non usa tile.
