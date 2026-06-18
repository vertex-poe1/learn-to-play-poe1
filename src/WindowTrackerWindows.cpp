#include "WindowTracker.h"

#include <QFileInfo>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>

namespace {

struct EnumData {
    const QString &target;
    WindowState    result;
};

BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd))
        return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid)
        return TRUE;

    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc)
        return TRUE;

    wchar_t buf[MAX_PATH] = {};
    DWORD   size          = MAX_PATH;
    if (QueryFullProcessImageNameW(proc, 0, buf, &size)) {
        const QString fullPath = QString::fromWCharArray(buf, static_cast<int>(size));
        const QString baseName = QFileInfo(fullPath).fileName();
        if (baseName.compare(reinterpret_cast<EnumData *>(lParam)->target,
                             Qt::CaseInsensitive) == 0) {
            RECT r = {};
            GetWindowRect(hwnd, &r);
            auto *data         = reinterpret_cast<EnumData *>(lParam);
            data->result.found      = true;
            data->result.rect       = QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
            data->result.installDir = QFileInfo(fullPath).absolutePath();
            CloseHandle(proc);
            return FALSE; // stop enumeration
        }
    }

    CloseHandle(proc);
    return TRUE;
}

} // namespace

class WindowTrackerWindows : public WindowTracker {
public:
    WindowState poll(const QString &executableName) override
    {
        EnumData data{executableName, {}};
        EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));
        return data.result;
    }
};

WindowTracker *WindowTracker::create()
{
    return new WindowTrackerWindows();
}
