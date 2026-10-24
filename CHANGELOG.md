# Changelog

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
