# GPUN EQ — Windows 10/11 x64 VST3 for FL Studio

GPUN EQ is a JUCE-based parametric equalizer starter project. It includes a full/compact interface, dynamically addable bands, and VST3 + standalone build targets.

## Included
- `Source/PluginProcessor.*` — audio processing, biquad filters, band state save/restore
- `Source/PluginEditor.*` — full/compact interface and EQ response plot
- `CMakeLists.txt` — JUCE/CMake project
- `.github/workflows/build-windows.yml` — cloud Windows x64 build

## Build without installing developer apps on your PC (GitHub Actions)
This route builds on GitHub's Windows cloud runner. You do not need to install Visual Studio, CMake, or JUCE locally.

1. Create or sign in to a GitHub account.
2. Create a new repository and upload the contents of this ZIP (extract it first; upload the files/folders, including `.github`).
3. Open the repository's **Actions** tab.
4. Select **Build GPUN EQ VST3 (Windows x64)**.
5. Click **Run workflow** and wait for the build to finish.
6. Open the completed workflow run and download the artifact named **GPUN-EQ-Windows-x64**.
7. Extract the artifact. Copy `GPUN EQ.vst3` into:
   `C:\Program Files\Common Files\VST3\`
8. Restart FL Studio. Open **Options > Manage plugins**, run a scan, then find **GPUN EQ** in the plugin list.

GitHub's interface wording can change slightly. The repository must include the `.github/workflows/build-windows.yml` file for the workflow to appear.

## Local build (optional)
Requirements: Windows 10/11 x64, Visual Studio 2022 with Desktop C++, CMake 3.22+, and JUCE 7/8.

```bat
cmake -S . -B build -DJUCE_DIR=C:\JUCE -A x64
cmake --build build --config Release
```

Output:
`build\GPUNEQ_artefacts\Release\VST3\GPUN EQ.vst3`

## Controls
- `+ ADD BAND` creates another band.
- Select a band, choose Bell / Low Shelf / High Shelf / High Pass / Low Pass, and edit Frequency / Gain / Q.
- `COMPACT` switches to the smaller window; `EXPAND` returns to the full window.
- `REMOVE` removes the selected band; at least one remains.

## Honest feature/engineering notes
- "Unlimited" means bands are dynamically added rather than fixed to a set number; practical limits are CPU and memory.
- The displayed curve is a calculated EQ response, not a live FFT spectrum analyzer.
- This is a functional starter implementation, not yet a commercial mastering-grade plugin. The current audio callback uses a mutex while the UI edits the dynamic band/filter vectors; for professional low-latency release, replace that with lock-free parameter snapshots or double-buffered filter state. It also does not yet provide host-automatable parameters, Mid/Side processing, or a live FFT analyzer.
- No precompiled `.vst3` binary is included in this ZIP. GitHub Actions is provided to compile it in the cloud and package the result.
- JUCE is fetched from its public GitHub repository by the workflow; review JUCE's current licensing terms before distributing a commercial product.
