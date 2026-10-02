// getdata.cpp
// Gather system performance metrics (CPU, GPU, RAM) using PDH and GlobalMemoryStatusEx.

#include "MiniMonitor.h"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <vector>
#include <algorithm>
#include <dxgi1_4.h>
#include <cstdint>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "dxgi.lib")

// Externs defined in MiniMonitor.cpp
extern PDH_HQUERY cpuQuery;
extern PDH_HCOUNTER cpuCounter;
extern PDH_HQUERY gpuQuery;
extern PDH_HCOUNTER gpuCounter;
extern float cpuLoad;
extern float gpuLoad;
extern float ramLoad;
extern bool firstSampleTaken;
extern float gpuRamLoad;

void UpdatePerformanceData()
{
    static PDH_HQUERY gpuMemoryQuery = nullptr;
    static PDH_HCOUNTER gpuMemoryCounter = nullptr;
    static bool gpuMemoryCounterInitialized = false;

    if (!gpuMemoryCounterInitialized) {
        gpuMemoryCounterInitialized = true;
        if (PdhOpenQueryW(nullptr, 0, &gpuMemoryQuery) != ERROR_SUCCESS ||
            PdhAddEnglishCounterW(gpuMemoryQuery, L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &gpuMemoryCounter) != ERROR_SUCCESS) {
            if (gpuMemoryQuery) {
                PdhCloseQuery(gpuMemoryQuery);
                gpuMemoryQuery = nullptr;
            }
            gpuMemoryCounter = nullptr;
        }
    }

    // CPU: collect and format the aggregated processor utility value.
    if (cpuQuery && cpuCounter) {
        PDH_STATUS status = PdhCollectQueryData(cpuQuery);
        if (status == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE counterVal;
            PDH_STATUS s2 = PdhGetFormattedCounterValue(cpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
            if (s2 == ERROR_SUCCESS && counterVal.CStatus == ERROR_SUCCESS) {
                cpuLoad = static_cast<float>(counterVal.doubleValue);
                cpuLoad = std::clamp(cpuLoad, 0.0f, 100.0f);
            }
        }
    }

    // GPU: handle wildcard counters which may return multiple instances (array) or a single instance.
    if (gpuQuery && gpuCounter) {
        PDH_STATUS status = PdhCollectQueryData(gpuQuery);
        if (status == ERROR_SUCCESS) {
            DWORD bufferSize = 0;
            DWORD itemCount = 0;

            // First call to discover required buffer size for array result.
            PDH_STATUS s = PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);

            if (s == PDH_MORE_DATA && bufferSize > 0 && itemCount > 0) {
                // Allocate buffer and retrieve all instances.
                std::vector<BYTE> buffer(bufferSize);
                PDH_FMT_COUNTERVALUE_ITEM_W* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
                PDH_STATUS s2 = PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items);
                if (s2 == ERROR_SUCCESS && itemCount > 0) {
                    double maxVal = 0.0;
                    for (DWORD i = 0; i < itemCount; ++i) {
                        if (items[i].FmtValue.CStatus == ERROR_SUCCESS) {
                            double val = items[i].FmtValue.doubleValue;
                            if (val > maxVal) {
                                maxVal = val;
                            }
                        }
                    }
                    // Pick highest engine utilization; clamp to 0-100.
                    gpuLoad = static_cast<float>(std::clamp(maxVal, 0.0, 100.0));
                }
            }
            else if (s == ERROR_SUCCESS) {
                // Single instance returned directly.
                PDH_FMT_COUNTERVALUE counterVal;
                PDH_STATUS s3 = PdhGetFormattedCounterValue(gpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
                if (s3 == ERROR_SUCCESS && counterVal.CStatus == ERROR_SUCCESS) {
                    double val = std::clamp(counterVal.doubleValue, 0.0, 100.0);
                    gpuLoad = static_cast<float>(val);
                }
            }
        }
    }

    // RAM: calculate used physical memory percentage.
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&mem) && mem.ullTotalPhys > 0) {
        double used = static_cast<double>(mem.ullTotalPhys - mem.ullAvailPhys);
        double pct = (used / static_cast<double>(mem.ullTotalPhys)) * 100.0;
        pct = std::clamp(pct, 0.0, 100.0);
        ramLoad = static_cast<float>(pct);
    }

    if (gpuMemoryQuery && gpuMemoryCounter && PdhCollectQueryData(gpuMemoryQuery) == ERROR_SUCCESS) {
        DWORD bufferSize = 0;
        DWORD itemCount = 0;
        PDH_STATUS status = PdhGetFormattedCounterArrayW(
            gpuMemoryCounter, PDH_FMT_LARGE, &bufferSize, &itemCount, nullptr);

        if (status == PDH_MORE_DATA && bufferSize > 0 && itemCount > 0) {
            std::vector<BYTE> buffer(bufferSize);
            auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
            status = PdhGetFormattedCounterArrayW(
                gpuMemoryCounter, PDH_FMT_LARGE, &bufferSize, &itemCount, items);

            if (status == ERROR_SUCCESS) {
                uint64_t totalUsage = 0;
                for (DWORD i = 0; i < itemCount; ++i) {
                    if (items[i].FmtValue.CStatus == ERROR_SUCCESS && items[i].FmtValue.largeValue > 0) {
                        totalUsage += static_cast<uint64_t>(items[i].FmtValue.largeValue);
                    }
                }

                uint64_t totalDedicatedMemory = 0;
                IDXGIFactory4* factory = nullptr;
                if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
                    for (UINT index = 0;; ++index) {
                        IDXGIAdapter1* adapter = nullptr;
                        if (factory->EnumAdapters1(index, &adapter) == DXGI_ERROR_NOT_FOUND) {
                            break;
                        }
                        if (!adapter) {
                            continue;
                        }

                        DXGI_ADAPTER_DESC1 description{};
                        if (SUCCEEDED(adapter->GetDesc1(&description)) &&
                            !(description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                            totalDedicatedMemory += description.DedicatedVideoMemory;
                        }

                        adapter->Release();
                    }
                    factory->Release();
                }

                if (totalDedicatedMemory > 0) {
                    const double percentage =
                        (static_cast<double>(totalUsage) / static_cast<double>(totalDedicatedMemory)) * 100.0;
                    gpuRamLoad = static_cast<float>(std::clamp(percentage, 0.0, 100.0));
                }
            }
        }
    }

    // Indicate that at least one sample has been captured.
    firstSampleTaken = true;
}