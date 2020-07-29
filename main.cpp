#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <Windows.h>
#include <cstdio>
#include <tlhelp32.h>
#include <shellapi.h>

QStringList Apps;
bool isBlacklist = false;

void RaiseToDebugPermission()
{
    HANDLE hToken;
    HANDLE hProcess = GetCurrentProcess();
    if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        TOKEN_PRIVILEGES tkp;
        if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid))
        {
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
        }
        CloseHandle(hToken);
    }
}

int GetProcessID(wchar_t *process_name)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (wcscmp(entry.szExeFile, process_name) == 0)
            {
                return entry.th32ProcessID;
            }
        }
    }
    CloseHandle(snapshot);
    return 4;
}

BOOL OccupyFile(LPCTSTR lpFileName)
{
    BOOL bRet;
    RaiseToDebugPermission();
    QString CHRName = "explorer.exe";
    wchar_t *WCTName = reinterpret_cast<wchar_t *>(CHRName.data());
    HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, GetProcessID(WCTName));
    if (hProcess == NULL)
    {
        qDebug() << "Failed";
        return FALSE;
    }
    HANDLE hFile;
    HANDLE hTargetHandle;
    //以独占模式打开目标文件
    hFile = CreateFile(lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hProcess);
        return FALSE;
    }
    bRet = DuplicateHandle(GetCurrentProcess(), hFile, hProcess, &hTargetHandle,
                           0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    CloseHandle(hProcess);
    return bRet;
}

void ParseArguments()
{
    QStringList arguments = QCoreApplication::arguments();
    isBlacklist = arguments.at(1) == "black";
    for (int i = 2; i < arguments.count(); i++)
    {
        Apps.append(arguments.at(i));
        const wchar_t *WCTArg = reinterpret_cast<const wchar_t *>(arguments.at(i).data());
        OccupyFile(WCTArg);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ParseArguments();
    return a.exec();
}
