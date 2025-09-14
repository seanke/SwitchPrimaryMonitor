// SwitchPrimaryMonitor - Windows 10/11: switch primary display to the next attached monitor
// Implementation strategy:
// 1) Enumerate display devices attached to the desktop (skip mirroring drivers)
// 2) Identify the current primary and select the next device as the new primary
// 3) Rebase virtual desktop so the new primary is at (0,0) and preserve relative layout
// 4) Use ChangeDisplaySettingsEx with CDS_SET_PRIMARY and DM_POSITION for all displays

#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>

struct Display {
    DISPLAY_DEVICEW dd{};   // Device information (name, flags)
    DEVMODEW dm{};          // Current settings (including position)
    bool isPrimary = false;
};

static void PrintLastError(const wchar_t* context) {
    DWORD err = GetLastError();
    LPWSTR buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, nullptr);
    std::wcerr << context << L" failed. GetLastError=" << err;
    if (buf) {
        std::wcerr << L" (" << buf << L")";
        LocalFree(buf);
    }
    std::wcerr << std::endl;
}

static bool EnumerateDisplays(std::vector<Display>& out) {
    out.clear();
    for (DWORD i = 0;; ++i) {
        DISPLAY_DEVICEW dd{};
        dd.cb = sizeof(dd);
        if (!EnumDisplayDevicesW(nullptr, i, &dd, 0)) {
            break; // no more devices
        }

        // Consider only displays attached to desktop and not mirroring drivers
        if ((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0) continue;
        if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) != 0) continue;

        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsExW(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0)) {
            PrintLastError(L"EnumDisplaySettingsEx");
            continue;
        }

        Display d;
        d.dd = dd;
        d.dm = dm;
        d.isPrimary = (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) != 0;
        out.push_back(d);
    }
    return !out.empty();
}

static int SwitchPrimaryToNext() {
    std::vector<Display> displays;
    if (!EnumerateDisplays(displays)) {
        std::wcerr << L"No attached displays found." << std::endl;
        return 1;
    }

    // Find current primary index
    int currentPrimary = -1;
    for (size_t i = 0; i < displays.size(); ++i) {
        if (displays[i].isPrimary) { currentPrimary = static_cast<int>(i); break; }
    }
    if (currentPrimary < 0) {
        std::wcerr << L"Could not identify current primary display." << std::endl;
        return 2;
    }

    if (displays.size() == 1) {
        std::wcout << L"Only one display attached. Nothing to switch." << std::endl;
        return 0;
    }

    int targetIndex = (currentPrimary + 1) % static_cast<int>(displays.size());
    Display& target = displays[targetIndex];

    // Compute offset to rebase coordinates so target becomes (0,0)
    LONG offsetX = -target.dm.dmPosition.x;
    LONG offsetY = -target.dm.dmPosition.y;

    // 1) Mark target as primary and move to (0,0)
    {
        DEVMODEW dm = target.dm;
        dm.dmFields |= DM_POSITION; // ensure position field is applied
        dm.dmPosition.x = target.dm.dmPosition.x + offsetX; // should be 0
        dm.dmPosition.y = target.dm.dmPosition.y + offsetY; // should be 0

        LONG res = ChangeDisplaySettingsExW(target.dd.DeviceName, &dm, nullptr,
                                            CDS_SET_PRIMARY | CDS_UPDATEREGISTRY | CDS_NORESET, nullptr);
        if (res != DISP_CHANGE_SUCCESSFUL) {
            std::wcerr << L"ChangeDisplaySettingsEx (set primary) failed for "
                       << target.dd.DeviceName << L" with code " << res << std::endl;
            return 3;
        }
    }

    // 2) Update positions of all other displays preserving relative layout
    for (size_t i = 0; i < displays.size(); ++i) {
        if (static_cast<int>(i) == targetIndex) continue;
        Display& d = displays[i];
        DEVMODEW dm = d.dm;
        dm.dmFields |= DM_POSITION;
        dm.dmPosition.x = d.dm.dmPosition.x + offsetX;
        dm.dmPosition.y = d.dm.dmPosition.y + offsetY;

        LONG res = ChangeDisplaySettingsExW(d.dd.DeviceName, &dm, nullptr,
                                            CDS_UPDATEREGISTRY | CDS_NORESET, nullptr);
        if (res != DISP_CHANGE_SUCCESSFUL) {
            std::wcerr << L"ChangeDisplaySettingsEx (reposition) failed for "
                       << d.dd.DeviceName << L" with code " << res << std::endl;
            return 4;
        }
    }

    // 3) Apply changes
    LONG applyRes = ChangeDisplaySettingsExW(nullptr, nullptr, nullptr, 0, nullptr);
    if (applyRes != DISP_CHANGE_SUCCESSFUL) {
        std::wcerr << L"Final ChangeDisplaySettingsEx apply failed with code " << applyRes << std::endl;
        return 5;
    }

    std::wcout << L"Primary display switched to: " << target.dd.DeviceName << std::endl;
    return 0;
}

// If launched from a terminal, attach to parent console so output is visible.
static void AttachParentConsoleIfPresent() {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
        std::ios_base::sync_with_stdio(true);
    }
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    AttachParentConsoleIfPresent();
    return SwitchPrimaryToNext();
}
