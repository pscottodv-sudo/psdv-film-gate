# Changelog

## 1.2.0 - bordo del gate vivo e defocus
- **Edge Irregularity (µm)**, **Irregularity Size**, **Irregularity Motion**: piccole rientranze e sporgenze lungo il contorno del gate. Una parte è fissa (appartiene al gate), una parte cambia da un fotogramma all'altro. Non è weave e non è flicker: si muove solo il contorno. Anche gli aloni dei light leaks seguono il contorno irregolare.
- **Gate Defocus**: gate fuori fuoco, con bordi sfumati e angoli arrotondati e più scuri, come la sfocatura reale di un'apertura rettangolare (profilo separabile gaussiano). La sfumatura è centrata sul contorno anche nella visualizzazione log/gamma.
- Il bordo morbido ora sfuma l'immagine invece di tagliarla dove finisce la clip; la sfumatura può attraversare la linea tra due fotogrammi, quindi vengono calcolati entrambi i gate vicino al confine.
- *Fit Gate To Screen* tiene conto del bordo morbido e irregolare: niente tocca il bordo dello schermo.
- Preset del gate aggiornati con valori di irregolarità e defocus.

## 1.1.0 - velocità e grana
**Velocità**
- I fotogrammi sopra e sotto ora sono di default **Repeat Current Frame (fast)**. *Real Previous / Next* chiede a Resolve due fotogrammi in più per ogni frame, quindi fino a 3 volte la decodifica e il lavoro dei nodi a monte: era la causa principale del playback a scatti. Usalo per il render finale.
- Anche in modalità Real, t−1 / t+1 vengono chiesti **solo se sono visibili** sullo schermo (es. con *Fill Screen* o fotogrammi grandi non viene chiesto nulla).
- Campionamento dell'immagine: un solo decode per pixel invece di uno per texel, 7 campioni invece di 13 per le sfocature piccole.
- Weave e flicker calcolati una volta per frame invece che per pixel; grana: le posizioni delle nuvole sono condivise dai tre strati (metà dei calcoli).
- Metal compilato in fast math.
- Sulla CPU il rendering completo è circa il 30% più veloce. Il guadagno maggiore in Resolve viene dai fotogrammi vicini.

**Grana (v2)**
- Non sembra più rumore digitale. Tre scale (nuvole di colorante, gruppi, ammassi) invece di due; la scala più fine è di sola luminanza, il colore vive nei gruppi e negli ammassi; le nuvole non sono mai più piccole del pixel (prima diventavano puntini sale-e-pepe); gli ammassi sono più radi nelle zone di bassa densità.
- Nuovi controlli **Grain Clumping** (grana fine e uniforme ↔ ammassi organici) e **Grain Softness** (morbidezza di scanner/stampa; 0 = nitida).
- La calibrazione RMS a 48 µm resta: misurato 7.90 contro 7.93 atteso, legge dell'apertura entro lo 0.7%, Double-X 14.33 contro 14.38.

## 1.0.0 - prima versione OFX
- Nuovo plugin OpenFX C++ con rendering Metal (macOS universale) e percorso CPU; sostituisce il DCTL PSDV Filmstrip v2.x.
- Quattro sezioni indipendenti, ognuna con Enable, Preset e controlli manuali; un preset passa a *Custom* quando modifichi uno slider.
  - **Film Gate & Perforations**: 9 formati reali, Fit/Fill/Manual, aspect ratio, zoom/pan dell'immagine dentro il gate, fotogrammi vicini reali (t±1 dall'host) con spaziatura regolabile, bordo del gate (morbidezza, scurimento, angoli), perforazioni scure / retroilluminate / contorno, polvere, fibre, pelo nel gate, righe. Niente striscia di pellicola continua.
  - **Gate Weave, Jitter & Flicker**: ampiezze in µm sulla pellicola, casualità, ciclo, rotazione, jitter dello scanner, flicker in stop; preset da 35mm pin-registered a Super 8 usurata.
  - **Film Grain**: modello a nuvole di colorante calibrato sull'RMS a 48 µm; preset Vision3, Double-X 5222 (RMS 14 pubblicato), Tri-X, Ektachrome; risposta per zone, grana colore, gamma del negativo.
  - **Light Leaks**: alone sulle perforazioni e sul bordo del gate, flare laterali, bruciature organiche; Opacity, Blend Mode, Colour Tint, Frequency, Size; preset Edge Leaks / Warm Flares / Organic Burn / Subtle Glow.
- Installer macOS (.pkg), script di build, installazione e disinstallazione, modello di workflow GitHub Actions.
- **Enable Film Gate** spento disattiva anche polvere, fibre, pelo nel gate, righe e morbidezza ottica: l'immagine esce identica all'ingresso.
- Test GPU contro CPU (`tests/metal/`) e workflow `metal-test` che lo esegue sui Mac di GitHub.
- Correzioni durante lo sviluppo: con il gate spento l'immagine era ritagliata nel formato del gate (ora 1:1 a schermo); i Warm Flares riempivano lo sfondo fuori dal gate e avevano una giuntura al centro (ora sfumano sul bordo e i due lati sono indipendenti).
