# Ficu Stutter (VST3, Windows)

Prosty stutter / beat repeater. Pokrętła: STUTTER, REFRESH, FADE, MIX + przyciski SYNC i ON.

## Jak zbudować (najłatwiej: GitHub, bez instalowania niczego)
1. Załóż repo na GitHub i wrzuć cały ten folder (z ukrytym folderem .github).
2. Wejdź w zakładkę Actions, poczekaj na zielony build (ok. 10 min).
3. Pobierz artefakt "FicuStutter-VST3" i skopiuj folder "Ficu Stutter.vst3"
   do C:\Program Files\Common Files\VST3\
4. Zrescanuj wtyczki w DAW.

## Jak zbudować lokalnie
Wymagane: Visual Studio 2022 (Desktop C++) i CMake.
    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release
Wynik: build\FicuStutter_artefacts\Release\VST3\Ficu Stutter.vst3
