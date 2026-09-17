# README.md

MiniMonitor is a minimalist Windows desktop widget for real‑time CPU and GPU utilization monitoring. It displays values in a small borderless window with a dark background and neon green text.

## Features
- Minimal, borderless widget (Tool Window — does not appear in the taskbar)  
- Real‑time updates every 500 ms  
- Low resource usage via the PDH API (Performance Data Helper)  
- Simple controls: drag to move, double‑click to exit, right click for info and settings (future)

## Display format
C:XX% G:XX%

- C: Total CPU processor time (%)  
- G: Total GPU engine utilization (%)

## Requirements
- OS: Windows (required for WinAPI and PDH)  
- IDE: Microsoft Visual Studio (Desktop Development with C++ recommended)  
- Linker: pdh.lib (included via `#pragma comment(lib, "pdh.lib")`)

## Build (Visual Studio)
1. Create a new "Windows Desktop Application" project (or open an existing one).  
2. Add source file(s) (e.g. `MiniMonitor.cpp`).  
3. Ensure pdh.lib is linked (already referenced via `#pragma` in example).  
4. Use Release configuration for best performance.  
5. Run from Visual Studio or launch the `.exe` from the output folder.

## Version Changelog (English)
- v1.0.2 — 2026-09-17
  - Right click now opens an info window which will also be used for settings later. still shows the controls.
- v1.0.1 — 2026-09-17
  - Better controls and right click explains them.
- v1.0.0 — 2026-09-17
  - Initial release: borderless widget with CPU and GPU monitoring via PDH.