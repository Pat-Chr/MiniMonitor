# Version History

## v1.0.5.1 - Work in progress
- Button to close MiniMonitor in the info/settings menu.

## v1.0.5.0 - 2026-10-20
- Code cleanup and optimization.
- Both settings windows are now dragable.
- New option: Turn red if the load is above 90%.
- Bugfix: If the graphs are all turned off, the window still shows.
- System trayicon added.

## v1.0.4.2 - 2026-10-12
- Bugfix: The GPU Memory of another iGPU doesn't get added to the overall value anymore.
- Background can now be turned to transparent.

## v1.0.4.1 - 2026-10-08
- Single Meters can now be turned on and off in the settings.
- Escape key closes info and settings window now.
- Bugfix: The color pickers in the settings now show the correct current colors.
- Support for multi Monitor systems.

## v1.0.4.0 - 2026-10-02
- Switching the Border on and off no longer needs a restart.
- Added new VideoRam usage Information.
- Added double Buffering to the Main window. Stops flickering.

## v1.0.3.0 - 2026-09-28
- Border can now be switched on/off in settings window. (needs a restart)
- New function for global saving of settings. We don't have to create a new function every time. (works)
- All the current settings use the new unified save function now.
- Code improvement: Universal read-config function instead of individual ones for every task.
- Code improvement: Reading the text color is now all done in the ReadFromSettings function. Not needing the GetTextColor function anymore.
- reading background also now handled in the ReadFromSettings function. Not needing the GetBackgroundColor function anymore.
- New reset Button to reset the settings to default.
- Color of the edit settings window is now same as info window.

## v1.0.2.6 - 2026-09-21
- Change settings button added for easier access to settings. Opens a separate dialog window.
- Dragging now requires SHIFT key to be held down to prevent accidental moves.
- If the loaded window position is outside of the screen area. (example: resolution has changed) fall back to default position.
- If the window is being moved, ensure it stays fully within the monitor's work area.
- Bugfix: if video is being decoded while GPU compute is used at same time the value does not get added together anymore. instead it now picks the larger value of the two.

## v1.0.2.5 - 2026-09-20
- Now saves the last window position in the config file.
- The Config File now has a line for window position.
- When the main window opens it reads the last window position from config file and opens the window at this position.

## v1.0.2.4 - 2026-09-19
- Settings Window is now always on top
- Background color can now be changed in the config file. RGB Values.
- moved creating the config file to readconfig.cpp so it doesn't clutter the main file.
- you can now change the text color in the config file. RGB Values.
- better icon
- settings window is now created in its own file. much easier to read and work on.
- the settings window now shows the program version and the current text color.

## v1.0.2.3 - 2026-09-18
- One pixel border for the main window. looks better now.
- now creates a config file if it doesn't already exist. the config file does nothing so far.

## v1.0.2.2 - 2026-09-18
- getdata functions are now in their own file "getdata.cpp". The main program is more compact now.

## v1.0.2.1 - 2026-09-18
- New Icon: Chip with graph.
- Added list of future plans and ideas to the readme.
- Added RAM load display. Now shows CPU, GPU, and RAM utilization.

## v1.0.2.0 — 2026-09-17
- Changelog section in readme was gone. fixed.
- RAM load added. Is already working, the release exe doesn't have it yet.

## v1.0.1.0 — 2026-09-17
- Added GPU utilization display.
- Implemented 500ms refresh cadence.
- Control info Window added with basic instructions. Also used for settings later.

## v1.0.0.0 — 2026-09-17
- Minimal CPU monitor UI implemented.
- Borderless toolwindow and basic mouse controls.
