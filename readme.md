# BORIS Granular Station

<img src='./images/screenshot0.png' width="70%" height="70%">

**BORIS Granular Station** is a lightweight real-time granulator. It lets you zoom in on the details of sound and explore its temporal dimension while it’s flowing. But honestly, do whatever you want with it.

With 0 samples of latency introduced, BORIS is real time for real.

### Features

- Use the thumb on the waveform to choose the starting point of the grains (x-axis) and their random drift (y-axis)
- Pitch shifting works in 1/8 tone steps
- Envelope selector uses a custom interpolation for smooth transitions
- Freeze the incoming audio at any time
- Play with feedback — but be warned: the output can vary drastically, so feedback limits aren’t enforced

### Demo Video

https://youtu.be/WWlFgpCR7Ug

---

## Installation

**Download prebuilt binaries from the `install/` folder**:

- **VST3 Plugin**  
  Copy the file `BORIS_Granular_Station.vst3` into your system’s VST3 plugin directory:
  - **Windows:** `C:\Program Files\Common Files\VST3\`
  
- **Standalone Application**  
  Simply run the `BorisGranularStation.exe` from the same folder — no installation needed.

---

## Building from Source

BORIS Granular Station is a C++ project built with CMake. It uses [JUCE](https://juce.com/) as a submodule and includes the RNBO API directly.

### Prerequisites

- CMake (version 3.15 or newer)
- A C++ compiler (e.g., MSVC, Clang, or GCC)
- JUCE (added as a Git submodule)

### Clone the Repository

```bash
git clone --recurse-submodules https://github.com/glesdora/boris-granular-station
cd boris-granular-station
```
