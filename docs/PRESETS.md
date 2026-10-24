# Preset: valori e provenienza

Legenda: **[M]** = valore pubblicato, letto sulla fonte indicata · **[D]** = derivato da valori [M] · **[E]** = stima o scelta estetica, non è un dato misurato.

## Film Grain

Il valore *RMS Granularity* segue la definizione Kodak: deviazione standard della densità diffusa ×1000, a densità 1.0, misurata con un microdensitometro con apertura circolare di 48 µm. Il plugin calcola l'ampiezza per pixel con la legge dell'apertura (Selwyn, nella forma esatta per un campo gaussiano). Il risultato dipende quindi dal formato e dalla risoluzione di uscita.

| Preset | Emulsione | RMS | Stato | Note |
|---|---|---|---|---|
| Eastman Double-X 5222 | B/N negativo | **14** | **[M]** | Datasheet Kodak: "Diffuse rms granularity 14", densità netta 1.0, apertura 48 µm, D-96 a 21 °C, gamma 0.65–0.70. Il preset usa Negative Gamma 0.65 |
| Ektachrome 100D 7294 style | colore invertibile | **8** | **[M]** su E100 *still* | Il valore numerico è dell'E100 fotografica (E-4000). Per la 7294 cine il datasheet dà solo il grafico |
| Vision3 50D 5203 style | colore negativo | 2.5 | [E] | Kodak pubblica solo curve per strato, non un numero unico |
| Vision3 250D 5207 style | colore negativo | 4.0 | [E] | come sopra |
| Vision3 200T 5213 style | colore negativo | 4.2 | [E] | come sopra |
| Vision3 500T 5219 style | colore negativo | 6.0 | [E] | come sopra; la definizione della misura (48 µm) è [M] dal datasheet 5219 |
| Tri-X 7266 Reversal style | B/N invertibile | 16 | [E] | La definizione è [M] dal datasheet 7266; il valore no |

Il modello fisico e i suoi elementi:

- Nuvole di colorante gaussiane da 6.5 µm e 15 µm, granularità proporzionale a √D: modello di letteratura [M] (Stephenson & Saunders 2007).
- Legge dell'apertura σ·√A costante per aperture grandi: [M] (Higgins & Stultz 1959).
- Curve caratteristiche, granularità per strato, matrice cromatica e fattori di dimensione: [E].

Per calibrare su uno stock reale: leggi il valore RMS sul datasheet dello stock e inseriscilo in *RMS Granularity*. Per una verifica oggettiva, confronta la deviazione standard misurata su blocchi di 42.5 µm di una scansione uniforme con quella della simulazione.

## Gate Weave, Jitter & Flicker

I costruttori non pubblicano valori numerici di stabilità d'immagine per le cineprese. Tutti i numeri sono **[E]**. È documentato solo l'**ordine** tra i meccanismi: doppia contro-griffa / pin di registrazione < solo griffa < cartuccia Super 8, che ha il pressore di plastica nella cartuccia e non nella camera. Le ampiezze sono in µm rms sulla pellicola.

| Preset | Orizz. µm | Vert. µm | Casualità | Ciclo (frame) | Rot. ° | Jitter scanner µm | Flicker stop |
|---|---|---|---|---|---|---|---|
| 35mm Pin-Registered (Arricam / 435 class) | 1.0 | 1.5 | 0.45 | 12 | 0.00 | 0.6 | 0.015 |
| 16mm Pin-Registered (SR class) | 1.5 | 2.5 | 0.50 | 10 | 0.00 | 1.0 | 0.025 |
| 16mm Claw-Only (Bolex H16 class) | 4 | 8 | 0.60 | 8 | 0.02 | 1.5 | 0.06 |
| Regular 8 Spring Camera (Bolex H8 class) | 6 | 12 | 0.60 | 7 | 0.03 | 2.0 | 0.12 |
| Super 8 Cartridge Camera | 8 | 18 | 0.65 | 6 | 0.05 | 2.0 | 0.10 |
| Worn Super 8 Camera | 16 | 35 | 0.70 | 5 | 0.10 | 3.0 | 0.18 |
| Projector Telecine | 10 | 14 | 0.30 | 24 | 0.02 | 4.0 | 0.08 |
| Locked | 0 | 0 | – | – | 0 | 0 | 0 |

## Film Gate & Perforations

La geometria viene dalla tabella dei formati in `psdv_core.h` (SMPTE/ISO; valori [M]/[D] come nella documentazione della v2):

- Super 8: gate 5.69×4.22 mm, passo 4.234, foro 0.914×1.143.
- Regular 8: 4.90×3.60, passo 3.810, foro 1.83×1.27 sulla linea tra i fotogrammi.
- 16mm: 10.26×7.49, passo 7.605.
- Super 16: 12.52×7.41.
- 35mm: perforazioni BH 2.794×1.854, passo 4.74 mm per foro.
- 65mm 5 perforazioni: 52.48×23.01, perforazioni KS.

I preset di gate (dimensione a schermo, spaziatura, morbidezza del bordo, sporco) sono scelte estetiche **[E]**.

## Light Leaks

Sono un look, non una misura. Tutti i valori sono **[E]**. *Edge Leaks* riproduce il riferimento fornito: foro scuro con bordo arancio luminoso e bordo caldo intorno alla finestra.

| Preset | Opacità | Blend | Tinta RGB | Frequenza | Size | Perf glow | Edge glow | Flares | Burn |
|---|---|---|---|---|---|---|---|---|---|
| Edge Leaks | 1.0 | Screen | 1.00 / 0.28 / 0.04 | 0.3 | 1.0 | 0.9 | 0.7 | 0 | 0 |
| Warm Flares | 1.0 | Screen | 1.00 / 0.38 / 0.08 | 1.0 | 2.0 | 0.3 | 0.3 | 0.8 | 0 |
| Organic Burn | 1.0 | Add | 1.00 / 0.22 / 0.02 | 0.5 | 1.5 | 0.4 | 0.4 | 0.2 | 0.9 |
| Subtle Glow | 0.6 | Screen | 1.00 / 0.40 / 0.12 | 0.2 | 0.8 | 0.4 | 0.35 | 0 | 0 |

## Fonti

- Kodak, EASTMAN DOUBLE-X 5222/7222 Technical Data: https://www.kodak.com/content/products-brochures/Film/DOUBLEX-Technical-Data-EN.pdf
- Kodak, EKTACHROME E100 (E-4000): https://business.kodakmoments.com/sites/default/files/files/products/e4000_ektachrome_100.pdf
- Kodak, EKTACHROME 100D 7294: https://www.freestylephoto.com/static/pdf/product_pdf/kodak/Technical-data-7294-EKTACHROME-100D.pdf
- Kodak, VISION3 500T 5219/7219: https://www.kodak.com/content/products-brochures/Film/VISION3_5219_7219_Technical-data.pdf
- Kodak, TRI-X Reversal 7266: https://www.kodak.com/content/products-brochures/Film/TRI-X-7266-Technical-Data-EN.pdf
- I. Stephenson, A. Saunders, *Simulating Film Grain using the Noise-Power Spectrum*, TPCG 2007: https://eprints.bournemouth.ac.uk/10547/1/grain.pdf
- G. C. Higgins, K. F. Stultz, *Experimental Study of rms Granularity as a Function of Scanning-Spot Size*, JOSA 49 (1959) 925, doi:10.1364/JOSA.49.000925
- Formati: https://www.filmkorn.org/super8data/database/articles_list/super8_fotmat_standards.htm · https://en.wikipedia.org/wiki/Standard_8_mm_film · https://en.wikipedia.org/wiki/16_mm_film · https://en.wikipedia.org/wiki/Film_perforations
