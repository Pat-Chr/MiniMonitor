MiniMonitor
MiniMonitor is a lightweight, unobtrusive Windows desktop widget designed to provide real-time monitoring of your system’s CPU and GPU utilization. It presents the data in a tiny, borderless window that stays out of your way, making it perfect for gamers or power users who want to keep an eye on performance without a bulky overlay.

✨ Features
Minimalist Design: A tiny, borderless window with a dark background and neon green text.
Real-time Updates: Refreshes system statistics every 500ms.
Low Resource Footprint: Uses the Windows Performance Data Helper (PDH) API for highly efficient data collection.
Unobtrusive: Designed as a “Tool Window,” meaning it will not clutter your Taskbar.
Aggregate GPU Tracking: Automatically detects and sums up the utilization of all GPU engines.
🖥️ Display Format
The monitor displays information in the following format: C:XX% G:XX%

C: Total CPU Processor Time percentage.
G: Total GPU Engine Utilization percentage.
🖱️ Controls
Since the window has no title bar or borders, use the following mouse controls to interact with it:

Move Window: Click and hold the Left Mouse Button anywhere on the widget to drag it to a new position on your screen.
Close Program: Right-click anywhere on the widget to instantly exit the application.
🛠️ Requirements & Compilation
Prerequisites
Operating System: Windows (required for WinAPI and PDH).
Compiler: A C++ compiler supporting Windows development (e.g., Microsoft Visual Studio with the “Desktop development with C++” workload).
Compilation Instructions
The project is written in C++ and utilizes the pdh.lib library.

If you are using Visual Studio:

Create a new “Windows Desktop Application” project.
Add MiniMonitor.cpp (and its corresponding .h files) to your project.
The code includes #pragma comment(lib, "pdh.lib"), so the linker will automatically include the necessary library.
Build the solution in Release mode for the best performance.