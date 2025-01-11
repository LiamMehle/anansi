#include <stdbool.h>
#include <windows.h>
#include <stdio.h>

BOOL EnablePrivilege(HANDLE hToken, LPCTSTR lpszPrivilege) {
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
        printf("LookupPrivilegeValue error: %lu\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        printf("AdjustTokenPrivileges error: %lu\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        printf("The token does not have the specified privilege.\n");
        return FALSE;
    }

    return TRUE;
}

bool EnableLargePagePrivilege() {
    HANDLE hToken;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        printf("OpenProcessToken failed: %lu\n", GetLastError());
        return FALSE;
    } else
        puts("OpenProcessToken succeeded");

    BOOL result = EnablePrivilege(hToken, SE_LOCK_MEMORY_NAME);
    CloseHandle(hToken);
    return result;
}
