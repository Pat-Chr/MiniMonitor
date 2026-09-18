# MiniMonitor

Minimalist Windows desktop widget for real-time CPU and GPU utilization monitoring.

![preview](docs/images/preview_1.0.1.0.png)

## 🚀 Features
- **Minimalist Design**: Small, borderless window that stays out of your way.
- **Low Overhead**: Uses Windows Performance Data Helper (PDH) API for efficient, low-resource monitoring.
- **Real-time Updates**: Refreshes every 500ms.
- **Non-Intrusive**: Uses `WS_EX_TOOLWINDOW` so it does not appear in the taskbar.
- **Lightweight**: Minimal dependencies; single executable expected for typical builds.

## 🖥️ Display Format
The widget displays current utilization in a stacked format:
```text
CPU:XX%
GPU:XX%
RAM:XX%
```
*(Note: RAM is already working, but didn't make it into the binary.)*

## 🖱️ Controls
| Action | Result |
| :--- | :--- |
| **Left Click & Drag** | Move the widget around your screen |
| **Double Click** | Close the widget |
| **Right Click** | Open Info/Settings window |

## 🛠️ Technical Details
- **Language**: C++
- **API**: Win32 API, PDH (Performance Data Helper)
- **Dependencies**: `pdh.lib` (linked via `#pragma comment`)
- **Build type**: Recommended Release x64 for lowest overhead
- **Window style**: borderless / toolwindow; designed for minimal screen real estate

## 🔨 Build Instructions
### Prerequisites
- **OS**: Windows 10/11
- **IDE**: Microsoft Visual Studio (with "Desktop development with C++" workload installed).

### Steps
1. Open the solution (`MiniMonitor.slnx` or `.vcxproj`) in Visual Studio.
2. Set the build configuration to **Release**.
3. Set the platform to **x64**.
4. Build the solution (**Ctrl+Shift+B**).
5. Run the generated `.exe` from the output folder (e.g., `x64/Release/`).

## 🗒️ Changelog
- v1.0.2.1 - unreleased WIP
	- New Icon. Chip with graph.
 	- Added list of future plans and ideas to readme.
- v1.0.2.0 — 2026-09-17
	- Changelog section in readme was gone. fixed.
	- RAM load added. Is already working, the release exe doesn't have it yet.
- v1.0.1.0 — 2026-09-17
	- Added GPU utilization display.
	- Implemented 500ms refresh cadence.
	- Control info Window added with basic instructions. Also used for settings later.
- v1.0.0.0 — 2026-09-17
	- Minimal CPU monitor UI implemented.
	- Borderless toolwindow and basic mouse controls.

## 🗒️ Planned features and improvements.
For next release
- The Functions which are getting all the information should be in their own file or class. This would make the code of the main program more compact.
- Adding a tooltip that says "Right click for settings" when the user hovers the mouse over the main window. Note: there was an error last time i tried.

For later
- the program should create a config file at start. only if it doesnt already exist. Just an empty file for now.
- The config file should have some lines with settings in it and be able to read and write them.
- If the config file works add the first settings option. (maybe "text color")
- The Window could be transparent or maybe have a little border. Some design improvements.
- The Design could be reworked to look like:
```text
	-------------
	| CPU% CRAM% |
	| GPU% GRAM% |
	-------------
 ```
- 

## 📜 License
All rights reserved. This project is for personal use only and may not be redistributed or used commercially without permission.
