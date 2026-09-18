// getdata.cpp

#include "MiniMonitor.h"
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "pdh.lib")

// externs from MiniMonitor.cpp
extern PDH_HQUERY cpuQuery;
extern PDH_HCOUNTER cpuCounter;
extern PDH_HQUERY gpuQuery;
extern PDH_HCOUNTER gpuCounter;
extern float cpuLoad;
extern float gpuLoad;
extern float ramLoad;
extern bool firstSampleTaken;

void UpdatePerformanceData()
{
    // CPU
    if (cpuQuery && cpuCounter) {
        PDH_STATUS status = PdhCollectQueryData(cpuQuery);
        if (status == ERROR_SUCCESS) {
            PDH_FMT_COUNTERVALUE counterVal;
            PDH_STATUS s2 = PdhGetFormattedCounterValue(cpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
            if (s2 == ERROR_SUCCESS && counterVal.CStatus == ERROR_SUCCESS) {
                cpuLoad = static_cast<float>(counterVal.doubleValue);
                if (cpuLoad < 0.0f) cpuLoad = 0.0f;
                if (cpuLoad > 100.0f) cpuLoad = 100.0f;
            }
        }
    }

    // GPU (aggregate instances returned by wildcard counter)
    if (gpuQuery && gpuCounter) {
        PDH_STATUS status = PdhCollectQueryData(gpuQuery);
        if (status == ERROR_SUCCESS) {
            DWORD bufferSize = 0;
            DWORD itemCount = 0;
            // First call to get required buffer size
            PDH_STATUS s = PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);
            if (s == PDH_MORE_DATA && bufferSize > 0 && itemCount > 0) {
                // Allocate buffer
                std::vector<BYTE> buffer(bufferSize);
                PDH_FMT_COUNTERVALUE_ITEM_W* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
                PDH_STATUS s2 = PdhGetFormattedCounterArrayW(gpuCounter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items);
                if (s2 == ERROR_SUCCESS && itemCount > 0) {
                    double sum = 0.0;
                    DWORD validCount = 0;
                    for (DWORD i = 0; i < itemCount; ++i) {
                        if (items[i].FmtValue.CStatus == ERROR_SUCCESS) {
                            sum += items[i].FmtValue.doubleValue;
                            ++validCount;
                        }
                    }
                    if (validCount > 0) {
                        // Sum of engine utilizations; clamp to 0-100 (typical desired behavior)
                        double val = sum;
                        if (val < 0.0) val = 0.0;
                        if (val > 100.0) val = 100.0;
                        gpuLoad = static_cast<float>(val);
                    }
                }
            }
            else if (s == ERROR_SUCCESS) {
                // Single instance returned directly
                PDH_FMT_COUNTERVALUE counterVal;
                PDH_STATUS s3 = PdhGetFormattedCounterValue(gpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
                if (s3 == ERROR_SUCCESS && counterVal.CStatus == ERROR_SUCCESS) {
                    double val = counterVal.doubleValue;
                    if (val < 0.0) val = 0.0;
                    if (val > 100.0) val = 100.0;
                    gpuLoad = static_cast<float>(val);
                }
            }
        }
    }

    // RAM
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&mem)) {
        if (mem.ullTotalPhys > 0) {
            double used = static_cast<double>(mem.ullTotalPhys - mem.ullAvailPhys);
            double pct = (used / static_cast<double>(mem.ullTotalPhys)) * 100.0;
            if (pct < 0.0) pct = 0.0;
            if (pct > 100.0) pct = 100.0;
            ramLoad = static_cast<float>(pct);
        }
    }

    // Mark that at least one sample has been taken (if not already)
    firstSampleTaken = true;
}