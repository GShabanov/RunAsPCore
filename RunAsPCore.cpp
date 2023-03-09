// RunAsPCore.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

static inline const TCHAR* yesno(bool b)
{
    return b ? _T("yes") : _T("no");
}

void print_bitmap(ULONG_PTR mask)
{
    for (int i = sizeof(mask) * 8 - 1; i >= 0; --i)
        _tprintf(_T("%d"), (int)(mask >> i) & 1);
}

void print_group_affinity(const GROUP_AFFINITY& affinity)
{
    _tprintf(_T(" Group #%d = "), affinity.Group);

    print_bitmap(affinity.Mask);

    _tprintf(_T("\n"));
}

int
_tmain(int argc, _TCHAR* argv[])
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* pBuffer;
    DWORD cbBuffer;

    cbBuffer = 0;
    if (GetLogicalProcessorInformationEx(RelationAll, NULL, &cbBuffer))
    {
        _tprintf(_T("GetLogicalProcessorInformationEx failed\n"));
        return 0;
    }

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        _tprintf(_T("GetLogicalProcessorInformationEx returned error (1). GetLastError() = %u\n"), GetLastError());
        return 1;
    }

    pBuffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(cbBuffer);

    if (pBuffer == NULL)
        return 1;

    if (!GetLogicalProcessorInformationEx(RelationAll, pBuffer, &cbBuffer))
    {
        _tprintf(_T("GetLogicalProcessorInformationEx returned error (2). GetLastError() = %u\n"), GetLastError());
        return 2;
    }

    _tprintf(_T("GetLogicalProcessorInformationEx returned %u byte data.\n"), cbBuffer);

    unsigned char* pCur = (unsigned char*)pBuffer;
    unsigned char* pEnd = pCur + cbBuffer;

    DWORD_PTR    AffinityMask = 0;


    for (int idx = 0; pCur < pEnd; pCur += ((SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)pCur)->Size, ++idx)
    {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *pCurrent = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)pCur;

        static const TCHAR* relationship_list[] = {
            _T("ProcessorCore"),
            _T("NumaNode"),
            _T("Cache"),
            _T("ProcessorPackage"),
            _T("Group"),
        };

        const TCHAR* relationship_str;

        if (pCurrent->Relationship >= _countof(relationship_list))
            relationship_str = _T("(reserved)");
        else
            relationship_str = relationship_list[pCurrent->Relationship];


        switch (pCurrent->Relationship)
        {
        case RelationProcessorCore:
        {
            _tprintf(_T(" Relationship = %s\n"), relationship_str);

            PROCESSOR_RELATIONSHIP& info = pCurrent->Processor;

            _tprintf(_T(" SMT Support = %s\n"), yesno(info.Flags == LTP_PC_SMT));
            _tprintf(_T(" Efficiency Class = %d\n"), info.EfficiencyClass);
            _tprintf(_T(" GroupCount = %d\n"), info.GroupCount);

            if (info.GroupCount > 0 &&
                info.EfficiencyClass > 0)
            {
                AffinityMask |= info.GroupMask[0].Mask;

                print_bitmap(info.GroupMask[0].Mask);
            }

        }
        break;

        case RelationGroup:
        {
            _tprintf(_T(" Relationship = %s\n"), relationship_str);

            GROUP_RELATIONSHIP& info = pCurrent->Group;

            _tprintf(_T(" Max Group Count = %d\n"), info.MaximumGroupCount);
            _tprintf(_T(" Active Group Count = %d\n"), info.ActiveGroupCount);

            for (int i = 0; i < info.ActiveGroupCount; ++i)
            {

                PROCESSOR_GROUP_INFO& ginfo = info.GroupInfo[i];

                _tprintf(_T(" Group #%d:\n"), i);
                _tprintf(_T("  Max Processor Count = %d\n"), ginfo.MaximumProcessorCount);
                _tprintf(_T("  Active Processor Count = %d\n"), ginfo.ActiveProcessorCount);
                _tprintf(_T("  Active Processor Mask = "));
                print_bitmap(ginfo.ActiveProcessorMask);
                _tprintf(_T("\n"));
            }
        }
        break;

        default:;
        }
    }

    free(pBuffer);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    _tprintf(_T("\n"));
    _tprintf(_T("[*] Affinity mask\n"));
    print_bitmap(AffinityMask);


    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    if (argc != 2)
    {
        _tprintf(_T("Usage: %s [cmdline]\n"), argv[0]);
        return 1;
    }

    // Start the child process. 
    if (!CreateProcess(NULL,   // No module name (use command line)
        argv[1],         // Command line
        NULL,            // Process handle not inheritable
        NULL,            // Thread handle not inheritable
        FALSE,           // Set handle inheritance to FALSE
        CREATE_SUSPENDED,  // No creation flags
        NULL,            // Use parent's environment block
        NULL,            // Use parent's starting directory 
        &si,             // Pointer to STARTUPINFO structure
        &pi)            // Pointer to PROCESS_INFORMATION structure
        )
    {
        _tprintf(_T("CreateProcess failed (%d).\n"), GetLastError());
        return 1;
    }

    SetProcessAffinityMask(pi.hProcess, AffinityMask);

    ResumeThread(pi.hThread);


    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);


    return 0;
}