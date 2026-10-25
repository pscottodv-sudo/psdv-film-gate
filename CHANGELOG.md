# Changelog

## 1.8.0 - perforazioni fedeli (overscan)
- **Dimensione fissa e reale**: tolto lo slider *Perforation Size*. La perforazione ha sempre la misura dello standard del formato scelto (R8, S8, 16, S16, 35, 65) e scala solo insieme a tutta la pellicola.
- **Perforation Position**: *Scan (faithful overscan)* mette i fori esattamente dove li prevede lo standard; *Custom* sblocca **Perforation Shift (mm)** per spostarli verso il bordo della pellicola (+) o verso l'immagine (−), senza cambiarne la misura.
- Nuova inquadratura **Overscan (Full Film Width)**: tutta la larghezza della pellicola, centrata sulla mezzeria del film come una vera scansione overscan.
- Misure ricontrollate sugli standard SMPTE: 16mm larghezza 15.95 mm, bordo→foro 0.90 mm (ST 109); 65mm larghezza 64.97 mm, bordo→foro 2.97 mm, raggio 0.51 mm, passo negativo 4.740 mm → 23.70 mm per 5 perforazioni (ST 145).

## 1.7.0 - anello "bruciato" come Dehancer
- Profilo della luce delle perforazioni rifatto misurando un fotogramma Dehancer (valori Rec.709 riportati in luce lineare): anello crema quasi bruciato sul bordo, banda arancio piatta e satura che finisce di colpo (~6% dell'altezza del foro), banda blu più lontana sotto il foro.
- Arancio più deciso sul lato verso cui pende la luce, quasi assente sul lato opposto.
- **Input Encoding** ora parte da **Rec.709 (Gamma 2.4)**, il default dei progetti Resolve. Con la timeline in Rec.709 e il plugin su DaVinci Intermediate, luce e grana venivano codificate in log e apparivano spente e senza contrasto. Con una timeline DaVinci Wide Gamut / Intermediate scegli DaVinci Intermediate.

## 1.6.0 - luce delle perforazioni con grana e colore più vivo
- La luce delle perforazioni passa per **la stessa grana** del film (stock, dimensione, clumping, softness): il colore si rompe come nel frame Dehancer invece di essere un bagliore digitale liscio.
- Nuovi controlli **Light Saturation** e **Hot Core** (la parte più luminosa del bordo brucia verso bianco-giallo, come un retroilluminatore sovraesposto).
- Arancio più caldo e più stretto; la banda blu sta fuori dall'arancio (sotto e, debole, più lontano sul lato del bagliore), senza mescolarsi in viola.
- Gli identificativi dei parametri della luce sono cambiati: sui nodi già esistenti la sezione riparte dai default (prima restavano valori salvati con significati vecchi).

## 1.5.0 - defocus delle perforazioni
- **Perforation Defocus**: foro fuori fuoco, bordo morbido; la luce si allarga e si ammorbidisce mantenendo i colori.
- Dove c'è la luce delle perforazioni l'immagine sottostante sparisce (la luce sta sulla pellicola non impressionata, non sopra l'immagine); dentro il foro l'immagine era già nera.

## 1.4.0 - luce delle perforazioni rifatta sul riferimento Dehancer
- Nuovo profilo a tre strati: linea sottile caldo-bianca sul bordo del foro, bagliore arancio che **pende da un lato**, banda **blu** poco distante dal bordo ruotata di 80° rispetto all'arancio (di default: arancio a sinistra, blu sotto, bordo opposto pulito).
- Controlli: **Opacity**, Exposure (EV), Rim Width, Glow Size, Rim / Glow / Cool Band Colour, **Cool Band**, **Lean** (uniforme ↔ da un lato), **Light Direction (°)**, Variation, Variation Speed, Light Seed. 8 preset.
- **Perforation Irregularity (µm)**: contorno del foro irregolare, con parte fissa e parte che cambia da un fotogramma all'altro (usa *Irregularity Motion*); la luce segue il contorno.
- **Perforation Opacity** ora agisce anche sulla luce: a 0 spariscono foro e luce (prima, con il foro nero sul bordo nero, sembrava non fare nulla).

## 1.3.0 - luce delle perforazioni
- La sezione *Light Leaks* è sostituita da **Perforation Light**: solo la luce intorno ai fori delle perforazioni, sempre presente quando le perforazioni sono visibili (non c'è più un interruttore), mai statica.
- Rimossi bagliore del bordo del gate, Warm Flares e Organic Burn.
- Profilo del bordo: nucleo bianco-giallo sottilissimo, alone arancio, alone rosso debole; il foro resta nero. Il lato più luminoso gira intorno al foro e l'intensità cambia da un fotogramma all'altro (ogni foro ha la sua variazione); frangia fredda opzionale sul lato in ombra.
- Controlli: Exposure (EV), Rim Width, Glow Size, Core/Glow/Halo Colour, Chromatic Fringe, Variation, Variation Speed, Hot Side, Light Seed; 7 preset.

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
