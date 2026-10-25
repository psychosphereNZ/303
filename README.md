# Acid303 – TB-303-Klon mit 16-Step-Sequencer (VST3 / AU / Standalone)

## Bauen (lokal)
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Das Plugin liegt danach unter `build/Acid303_artefacts/Release/VST3/`.
Windows: nach `C:\Program Files\Common Files\VST3\` kopieren. macOS: `~/Library/Audio/Plug-Ins/VST3/`.

## Bauen ohne Entwicklungsumgebung
Projekt auf GitHub hochladen -> Reiter "Actions" -> fertige Builds als Artefakte herunterladen.

## Sequencer
- 16 Steps, je mit Note (0-24 Halbtöne über ROOT), GATE, ACCENT und SLIDE.
- SLIDE auf Step N = Note gleitet in den Step N+1 (Gate bleibt offen, keine neue Hüllkurve).
  Folgt ein Pause-Step, endet die Note an der Step-Grenze.
- ACCENT: lauter, mehr Filter-Modulation, kürzerer Decay (Stärke über den Accent-Regler).
- LENGTH kürzt das Pattern (1-16), GATE LEN bestimmt die Notenlänge.
- RUN startet intern mit TEMPO; HOST SYNC folgt Tempo und Position der DAW (läuft nur bei Play).
- RANDOM würfelt ein Pattern in Moll-Pentatonik, CLEAR leert alles.
- MIDI-Noten funktionieren weiterhin parallel (Velocity >= 100 = Accent).
