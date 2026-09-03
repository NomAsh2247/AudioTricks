# AudioTricks

Practice with sound programming in C++.

AudioTricks is a small real-time audio project that captures audio through PortAudio and visualizes the incoming signal as a spectrogram. It is primarily intended as a space for experimenting with audio processing, digital signal processing, visualization, and related concepts.

## Features

* Real-time audio input using **PortAudio**
* Audio buffering with a ring buffer
* Real-time spectrogram visualization
* OpenGL-based rendering
* Dear ImGui and ImPlot integration
* FFT-based audio analysis using FFTW
* CMake-based build system

## Project Structure

```text
AudioTricks/
├── AudioTricks.cpp          # Application entry point
├── AudioPassthrough.h       # Audio passthrough functionality
├── BasePortAudio.h          # PortAudio helpers
├── Spectrogram.cpp          # Spectrogram implementation
├── Spectrogram.h            # Spectrogram interface
├── CMakeLists.txt            # Build configuration
├── CMakePresets.json         # CMake presets
└── deps/                    # Third-party dependencies
```

The application:

1. Initializes PortAudio.
2. Opens the system's default audio stream.
3. Places incoming audio into a ring buffer.
4. Processes audio blocks for spectral analysis.
5. Renders the resulting spectrogram in a graphical window.
6. Continues processing until the visualization window is closed.

## Status

This is a personal practice/experimental project for learning and exploring sound programming. Additional features may be added in the future.
