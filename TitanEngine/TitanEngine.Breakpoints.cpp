#include "stdafx.h"
#include "definitions.h"
#include "Global.Breakpoints.h"
#include "Global.Debugger.h"
#include "Global.Engine.h"
#include "Global.Engine.Threading.h"

static long engineDefaultBreakPointType = UE_BREAKPOINT_INT3;
static BYTE UD2BreakPoint[2] = {0x0F, 0x0B};
static BYTE INT3BreakPoint = 0xCC;
static BYTE INT3LongBreakPoint[2] = {0xCD, 0x03};

__declspec(dllexport) void TITCALL SetBPXOptions(long DefaultBreakPointType)
{
    engineDefaultBreakPointType = DefaultBreakPointType;
}

__declspec(dllexport) bool TITCALL IsBPXEnabled(ULONG_PTR bpxAddress)
{
    MutexLocker lock("BreakPointBuffer");
    ULONG_PTR NumberOfBytesReadWritten = 0;
    DWORD MaximumBreakPoints = 0;
    BYTE ReadData[10] = {};
    int bpcount=BreakPointBuffer.size();
    for(int i=0; i<bpcount; i++)
    {
        if(BreakPointBuffer.at(i).BreakPointAddress == bpxAddress)
        {
            if(BreakPointBuffer.at(i).BreakPointActive != UE_BPXINACTIVE)
            {
                if(ReadProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &ReadData[0], UE_MAX_BREAKPOINT_SIZE, &NumberOfBytesReadWritten))
                {
                    if(BreakPointBuffer.at(i).AdvancedBreakPointType == UE_BREAKPOINT_INT3 && ReadData[0] == INT3BreakPoint)
                        return true;
                    else if(BreakPointBuffer.at(i).AdvancedBreakPointType == UE_BREAKPOINT_LONG_INT3 && ReadData[0] == INT3LongBreakPoint[0] && ReadData[1] == INT3LongBreakPoint[1])
                        return true;
                    else if(BreakPointBuffer.at(i).AdvancedBreakPointType == UE_BREAKPOINT_UD2 && ReadData[0] == UD2BreakPoint[0] && ReadData[1] == UD2BreakPoint[1])
                        return true;
                    else //TODO: delete breakpoint from list?
                        return false;
                }
                else
                    return false;
            }
            else
                return false;
        }
    }
    return false;
}

__declspec(dllexport) bool TITCALL EnableBPX(ULONG_PTR bpxAddress)
{
    MutexLocker lock("BreakPointBuffer");
    MEMORY_BASIC_INFORMATION MemInfo;
    ULONG_PTR NumberOfBytesReadWritten = 0;
    DWORD MaximumBreakPoints = 0;
    bool testWrite = false;
    DWORD OldProtect;
    int bpcount=BreakPointBuffer.size();
    for(int i=0; i<bpcount; i++)
    {
        if(BreakPointBuffer.at(i).BreakPointAddress == bpxAddress)
        {
            VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
            OldProtect = MemInfo.Protect;
            VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
            if(BreakPointBuffer.at(i).BreakPointActive == UE_BPXINACTIVE && (BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT || BreakPointBuffer.at(i).BreakPointType == UE_SINGLESHOOT))
            {
                if(BreakPointBuffer.at(i).AdvancedBreakPointType == UE_BREAKPOINT_INT3)
                {
                    if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &INT3BreakPoint, 1, &NumberOfBytesReadWritten))
                    {
                        testWrite = true;
                    }
                }
                else if(BreakPointBuffer.at(i).AdvancedBreakPointType == UE_BREAKPOINT_LONG_INT3)
                {
                    if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &INT3LongBreakPoint, 2, &NumberOfBytesReadWritten))
                    {
                        testWrite = true;
                    }
                }
                else if(BreakPointBuffer.at(i).AdvancedBreakPointType == UE_BREAKPOINT_UD2)
                {
                    if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &UD2BreakPoint, 2, &NumberOfBytesReadWritten))
                    {
                        testWrite = true;
                    }
                }
                if(testWrite)
                {
                    BreakPointBuffer.at(i).BreakPointActive = UE_BPXACTIVE;
                    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, OldProtect, &OldProtect);
                    return true;
                }
                else
                {
                    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, OldProtect, &OldProtect);
                    return false;
                }
            }
            else
            {
                VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, OldProtect, &OldProtect);
                return false;
            }
        }
    }
    return false;
}

__declspec(dllexport) bool TITCALL DisableBPX(ULONG_PTR bpxAddress)
{
    MutexLocker lock("BreakPointBuffer");
    MEMORY_BASIC_INFORMATION MemInfo;
    ULONG_PTR NumberOfBytesReadWritten = 0;
    DWORD MaximumBreakPoints = 0;
    DWORD OldProtect;
    int bpcount=BreakPointBuffer.size();
    for(int i=0; i<bpcount; i++)
    {
        if(BreakPointBuffer.at(i).BreakPointAddress == bpxAddress)
        {
            VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
            OldProtect = MemInfo.Protect;
            VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
            if(BreakPointBuffer.at(i).BreakPointActive == UE_BPXACTIVE && (BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT || BreakPointBuffer.at(i).BreakPointType == UE_SINGLESHOOT))
            {
                if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &BreakPointBuffer.at(i).OriginalByte[0], BreakPointBuffer.at(i).BreakPointSize, &NumberOfBytesReadWritten))
                {
                    BreakPointBuffer.at(i).BreakPointActive = UE_BPXINACTIVE;
                    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, OldProtect, &OldProtect);
                    return true;
                }
                else
                {
                    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, OldProtect, &OldProtect);
                    return false;
                }
            }
            else
            {
                VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(i).BreakPointSize, OldProtect, &OldProtect);
                return false;
            }
        }
    }
    return false;
}

__declspec(dllexport) bool TITCALL SetBPX(ULONG_PTR bpxAddress, DWORD bpxType, LPVOID bpxCallBack)
{
    MutexLocker lock("BreakPointBuffer");
    void* bpxDataPrt;
    PMEMORY_COMPARE_HANDLER bpxDataCmpPtr;
    MEMORY_BASIC_INFORMATION MemInfo;
    ULONG_PTR NumberOfBytesReadWritten = 0;
    BYTE SelectedBreakPointType;
    DWORD checkBpxType;
    DWORD OldProtect;

    if(bpxCallBack == NULL)
    {
        return false;
    }
    int bpcount=BreakPointBuffer.size();
    //search for breakpoint
    for(int i=0; i<bpcount; i++)
    {
        if(BreakPointBuffer.at(i).BreakPointAddress == bpxAddress && BreakPointBuffer.at(i).BreakPointActive == UE_BPXACTIVE && (BreakPointBuffer.at(i).BreakPointType == UE_SINGLESHOOT || BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT))
            return false;
        else if(BreakPointBuffer.at(i).BreakPointAddress == bpxAddress && BreakPointBuffer.at(i).BreakPointActive == UE_BPXINACTIVE && (BreakPointBuffer.at(i).BreakPointType == UE_SINGLESHOOT || BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT))
        {
            lock.unlock();
            return EnableBPX(bpxAddress);
        }
    }
    //setup new breakpoint structure
    BreakPointDetail NewBreakPoint;
    memset(&NewBreakPoint, 0, sizeof(BreakPointDetail));
    if(bpxType < UE_BREAKPOINT_TYPE_INT3)
    {
        if(engineDefaultBreakPointType == UE_BREAKPOINT_INT3)
        {
            SelectedBreakPointType = UE_BREAKPOINT_INT3;
            NewBreakPoint.BreakPointSize = 1;
            bpxDataPrt = &INT3BreakPoint;
        }
        else if(engineDefaultBreakPointType == UE_BREAKPOINT_LONG_INT3)
        {
            SelectedBreakPointType = UE_BREAKPOINT_LONG_INT3;
            NewBreakPoint.BreakPointSize = 2;
            bpxDataPrt = &INT3LongBreakPoint;
        }
        else if(engineDefaultBreakPointType == UE_BREAKPOINT_UD2)
        {
            SelectedBreakPointType = UE_BREAKPOINT_UD2;
            NewBreakPoint.BreakPointSize = 2;
            bpxDataPrt = &UD2BreakPoint;
        }
    }
    else
    {
        checkBpxType = bpxType >> 24;
        checkBpxType = checkBpxType << 24;
        if(checkBpxType == UE_BREAKPOINT_TYPE_INT3)
        {
            SelectedBreakPointType = UE_BREAKPOINT_INT3;
            NewBreakPoint.BreakPointSize = 1;
            bpxDataPrt = &INT3BreakPoint;
        }
        else if(checkBpxType == UE_BREAKPOINT_TYPE_LONG_INT3)
        {
            SelectedBreakPointType = UE_BREAKPOINT_LONG_INT3;
            NewBreakPoint.BreakPointSize = 2;
            bpxDataPrt = &INT3LongBreakPoint;
        }
        else if(checkBpxType == UE_BREAKPOINT_TYPE_UD2)
        {
            SelectedBreakPointType = UE_BREAKPOINT_UD2;
            NewBreakPoint.BreakPointSize = 2;
            bpxDataPrt = &UD2BreakPoint;
        }
    }
    //set breakpoint in process
    bpxDataCmpPtr = (PMEMORY_COMPARE_HANDLER)bpxDataPrt;
    VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
    OldProtect = MemInfo.Protect;
    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, NewBreakPoint.BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
    if(ReadProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &NewBreakPoint.OriginalByte[0], NewBreakPoint.BreakPointSize, &NumberOfBytesReadWritten))
    {
        if(WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, bpxDataPrt, NewBreakPoint.BreakPointSize, &NumberOfBytesReadWritten))
        {
            //add new breakpoint to the list
            NewBreakPoint.AdvancedBreakPointType = SelectedBreakPointType&0xFF;
            NewBreakPoint.BreakPointActive = UE_BPXACTIVE;
            NewBreakPoint.BreakPointAddress = bpxAddress;
            NewBreakPoint.BreakPointType = bpxType&0xFF;
            NewBreakPoint.ExecuteCallBack = (ULONG_PTR)bpxCallBack;
            BreakPointBuffer.push_back(NewBreakPoint);
            VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, NewBreakPoint.BreakPointSize, OldProtect, &OldProtect);
            return true;
        }
        else
        {
            VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, NewBreakPoint.BreakPointSize, OldProtect, &OldProtect);
            return false;
        }
    }
    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, NewBreakPoint.BreakPointSize, OldProtect, &OldProtect);
    return false;
}

__declspec(dllexport) bool TITCALL DeleteBPX(ULONG_PTR bpxAddress)
{
    MutexLocker lock("BreakPointBuffer");
    ULONG_PTR NumberOfBytesReadWritten = 0;
    DWORD OldProtect;
    int bpcount=BreakPointBuffer.size();
    int found=-1;
    for(int i=0; i<bpcount; i++)
    {
        if(BreakPointBuffer.at(i).BreakPointAddress == bpxAddress && (BreakPointBuffer.at(i).BreakPointType == UE_SINGLESHOOT || BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT))
        {
            found=i;
            break;
        }
    }
    if(found == -1) //not found
        return false;
    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(found).BreakPointSize, PAGE_EXECUTE_READWRITE, &OldProtect);
    lock.unlock();
    if(IsBPXEnabled(bpxAddress))
    {
        if(!WriteProcessMemory(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, &BreakPointBuffer.at(found).OriginalByte[0], BreakPointBuffer.at(found).BreakPointSize, &NumberOfBytesReadWritten))
        {
            VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(found).BreakPointSize, OldProtect, &OldProtect);
            return false;
        }
    }
    lock.relock();
    VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)bpxAddress, BreakPointBuffer.at(found).BreakPointSize, OldProtect, &OldProtect);
    BreakPointBuffer.erase(BreakPointBuffer.begin()+found);
    return true;
}

__declspec(dllexport) bool TITCALL SafeDeleteBPX(ULONG_PTR bpxAddress)
{
    //TODO: remove?
    return DeleteBPX(bpxAddress);
}

__declspec(dllexport) bool TITCALL SetAPIBreakPoint(const char* szDLLName, const char* szAPIName, DWORD bpxType, DWORD bpxPlace, LPVOID bpxCallBack)
{
    BYTE ReadByte = NULL;
    HMODULE hModule = NULL;
    DWORD ReadMemSize = NULL;
    ULONG_PTR APIAddress = NULL;
    ULONG_PTR tryAPIAddress = NULL;
    ULONG_PTR QueryAPIAddress = NULL;
    int i = MAX_RET_SEARCH_INSTRUCTIONS;
    ULONG_PTR ueNumberOfReadWrite = NULL;
    int currentInstructionLen = NULL;
    bool ModuleLoaded = false;
    void* CmdBuffer = NULL;
    bool RemovedBpx = false;

    if(szDLLName != NULL && szAPIName != NULL)
    {
        hModule = GetModuleHandleA(szDLLName);
        if(hModule == NULL)
        {
            if(engineAlowModuleLoading)
            {
                hModule = LoadLibraryA(szDLLName);
                ModuleLoaded = true;
            }
            else
            {
                ReadMemSize = MAX_RET_SEARCH_INSTRUCTIONS * MAXIMUM_INSTRUCTION_SIZE;
                APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, NULL, szAPIName, UE_OPTION_IMPORTER_RETURN_APIADDRESS);
                if(APIAddress != NULL)
                {
                    CmdBuffer = VirtualAlloc(NULL, ReadMemSize, MEM_COMMIT, PAGE_READWRITE);
                    while(ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite) == false && ReadMemSize > NULL)
                    {
                        ReadMemSize = ReadMemSize - (MAXIMUM_INSTRUCTION_SIZE * 10);
                    }
                    if(ReadMemSize == NULL)
                    {
                        VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                        APIAddress = NULL;
                    }
                    else
                    {
                        tryAPIAddress = (ULONG_PTR)CmdBuffer;
                    }
                }
            }
        }
        if(hModule != NULL || APIAddress != NULL)
        {
            if(hModule != NULL)
            {
                APIAddress = (ULONG_PTR)GetProcAddress(hModule, szAPIName);
            }
            if(bpxPlace == UE_APIEND)
            {
                if(tryAPIAddress == NULL)
                {
                    tryAPIAddress = APIAddress;
                }
                QueryAPIAddress = APIAddress;
                RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
                while(i > 0 && ReadByte != 0xC3 && ReadByte != 0xC2)
                {
                    if(engineAlowModuleLoading == false && CmdBuffer != NULL)
                    {
                        if(IsBPXEnabled(QueryAPIAddress))
                        {
                            DisableBPX(QueryAPIAddress);
                            ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite);
                            RemovedBpx = true;
                        }
                    }
                    currentInstructionLen = StaticLengthDisassemble((LPVOID)tryAPIAddress);
                    tryAPIAddress = tryAPIAddress + currentInstructionLen;
                    RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
                    QueryAPIAddress = QueryAPIAddress + currentInstructionLen;
                    if(!engineAlowModuleLoading)
                    {
                        if(RemovedBpx)
                        {
                            EnableBPX(QueryAPIAddress - currentInstructionLen);
                        }
                    }
                    RemovedBpx = false;
                    i--;
                }
                if(i != NULL)
                {
                    if((engineAlowModuleLoading == true && ModuleLoaded == true) || (engineAlowModuleLoading == true && ModuleLoaded == false))
                    {
                        APIAddress = tryAPIAddress;
                    }
                    else if(!engineAlowModuleLoading)
                    {
                        if(CmdBuffer != NULL)
                        {
                            APIAddress = tryAPIAddress - (ULONG_PTR)CmdBuffer + APIAddress;
                        }
                        else
                        {
                            APIAddress = tryAPIAddress;
                        }
                    }
                }
                else
                {
                    if(ModuleLoaded)
                    {
                        FreeLibrary(hModule);
                    }
                    if(CmdBuffer != NULL)
                    {
                        VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                    }
                    return false;
                }
            }
            if(engineAlowModuleLoading)
            {
                APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS);
                if(ModuleLoaded)
                {
                    FreeLibrary(hModule);
                }
            }
            else
            {
                if(CmdBuffer != NULL)
                {
                    VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                }
            }
            return SetBPX(APIAddress, bpxType, bpxCallBack);
        }
        else
        {
            if(engineAlowModuleLoading)
            {
                if(ModuleLoaded)
                {
                    FreeLibrary(hModule);
                }
            }
            else
            {
                if(CmdBuffer != NULL)
                {
                    VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                }
            }
            return false;
        }
    }
    else
    {
        return false;
    }
    return false;
}

__declspec(dllexport) bool TITCALL DeleteAPIBreakPoint(const char* szDLLName, const char* szAPIName, DWORD bpxPlace)
{
    BYTE ReadByte = NULL;
    HMODULE hModule = NULL;
    DWORD ReadMemSize = NULL;
    ULONG_PTR APIAddress = NULL;
    ULONG_PTR tryAPIAddress = NULL;
    ULONG_PTR QueryAPIAddress = NULL;
    int i = MAX_RET_SEARCH_INSTRUCTIONS;
    ULONG_PTR ueNumberOfReadWrite = NULL;
    int currentInstructionLen = NULL;
    bool ModuleLoaded = false;
    void* CmdBuffer = NULL;
    bool RemovedBpx = false;

    if(szDLLName != NULL && szAPIName != NULL)
    {
        hModule = GetModuleHandleA(szDLLName);
        if(hModule == NULL)
        {
            if(engineAlowModuleLoading)
            {
                hModule = LoadLibraryA(szDLLName);
                ModuleLoaded = true;
            }
            else
            {
                ReadMemSize = MAX_RET_SEARCH_INSTRUCTIONS * MAXIMUM_INSTRUCTION_SIZE;
                APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, NULL, szAPIName, UE_OPTION_IMPORTER_RETURN_APIADDRESS);
                if(APIAddress != NULL)
                {
                    CmdBuffer = VirtualAlloc(NULL, ReadMemSize, MEM_COMMIT, PAGE_READWRITE);
                    while(ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite) == false && ReadMemSize > NULL)
                    {
                        ReadMemSize = ReadMemSize - (MAXIMUM_INSTRUCTION_SIZE * 10);
                    }
                    if(ReadMemSize == NULL)
                    {
                        VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                        APIAddress = NULL;
                    }
                    else
                    {
                        tryAPIAddress = (ULONG_PTR)CmdBuffer;
                    }
                }
            }
        }
        if(hModule != NULL || APIAddress != NULL)
        {
            if(hModule != NULL)
            {
                APIAddress = (ULONG_PTR)GetProcAddress(hModule, szAPIName);
            }
            if(bpxPlace == UE_APIEND)
            {
                if(tryAPIAddress == NULL)
                {
                    tryAPIAddress = APIAddress;
                }
                QueryAPIAddress = APIAddress;
                RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
                while(i > 0 && ReadByte != 0xC3 && ReadByte != 0xC2)
                {
                    if(engineAlowModuleLoading == false && CmdBuffer != NULL)
                    {
                        if(IsBPXEnabled(QueryAPIAddress))
                        {
                            DisableBPX(QueryAPIAddress);
                            ReadProcessMemory(dbgProcessInformation.hProcess, (void*)APIAddress, CmdBuffer, ReadMemSize, &ueNumberOfReadWrite);
                            RemovedBpx = true;
                        }
                    }
                    currentInstructionLen = StaticLengthDisassemble((LPVOID)tryAPIAddress);
                    tryAPIAddress = tryAPIAddress + currentInstructionLen;
                    RtlMoveMemory(&ReadByte, (LPVOID)tryAPIAddress, 1);
                    QueryAPIAddress = QueryAPIAddress + currentInstructionLen;
                    if(!engineAlowModuleLoading)
                    {
                        if(RemovedBpx)
                        {
                            EnableBPX(QueryAPIAddress - currentInstructionLen);
                        }
                    }
                    RemovedBpx = false;
                    i--;
                }
                if(i != NULL)
                {
                    if((engineAlowModuleLoading == true && ModuleLoaded == true) || (engineAlowModuleLoading == true && ModuleLoaded == false))
                    {
                        APIAddress = tryAPIAddress;
                    }
                    else if(!engineAlowModuleLoading)
                    {
                        if(CmdBuffer != NULL)
                        {
                            APIAddress = tryAPIAddress - (ULONG_PTR)CmdBuffer + APIAddress;
                        }
                        else
                        {
                            APIAddress = tryAPIAddress;
                        }
                    }
                }
                else
                {
                    if(ModuleLoaded)
                    {
                        FreeLibrary(hModule);
                    }
                    if(CmdBuffer != NULL)
                    {
                        VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                    }
                    return false;
                }
            }
            if(engineAlowModuleLoading)
            {
                APIAddress = (ULONG_PTR)EngineGlobalAPIHandler(dbgProcessInformation.hProcess, NULL, APIAddress, NULL, UE_OPTION_IMPORTER_REALIGN_APIADDRESS);
                if(ModuleLoaded)
                {
                    FreeLibrary(hModule);
                }
            }
            else
            {
                if(CmdBuffer != NULL)
                {
                    VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                }
            }
            return(DeleteBPX(APIAddress));
        }
        else
        {
            if(engineAlowModuleLoading)
            {
                if(ModuleLoaded)
                {
                    FreeLibrary(hModule);
                }
            }
            else
            {
                if(CmdBuffer != NULL)
                {
                    VirtualFree(CmdBuffer, NULL, MEM_RELEASE);
                }
            }
            return false;
        }
    }
    else
    {
        return false;
    }
    return false;
}

__declspec(dllexport) bool TITCALL SafeDeleteAPIBreakPoint(const char* szDLLName, const char* szAPIName, DWORD bpxPlace)
{
    //TODO: remove?
    return DeleteAPIBreakPoint(szDLLName, szAPIName, bpxPlace);
}

__declspec(dllexport) bool TITCALL SetMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, LPVOID bpxCallBack)
{
    return SetMemoryBPXEx(MemoryStart, SizeOfMemory, UE_MEMORY, false, bpxCallBack);
}

__declspec(dllexport) bool TITCALL SetMemoryBPXEx(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory, DWORD BreakPointType, bool RestoreOnHit, LPVOID bpxCallBack)
{
    MutexLocker lock("BreakPointBuffer");
    MEMORY_BASIC_INFORMATION MemInfo;
    ULONG_PTR NumberOfBytesReadWritten = 0;
    DWORD NewProtect = 0;
    DWORD OldProtect = 0;
    int bpcount=BreakPointBuffer.size();
    //search for breakpoint
    for(int i=0; i<bpcount; i++)
    {
        if(BreakPointBuffer.at(i).BreakPointAddress == MemoryStart &&
                (BreakPointBuffer.at(i).BreakPointType == UE_MEMORY ||
                 BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_READ ||
                 BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_WRITE ||
                 BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_EXECUTE)
          )
        {
            return false;
        }
    }
    //add new breakpoint
    BreakPointDetail NewBreakPoint;
    memset(&NewBreakPoint, 0, sizeof(BreakPointDetail));
    VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
    OldProtect = MemInfo.Protect;
    if(!(OldProtect & PAGE_GUARD))
    {
        NewProtect = OldProtect ^ PAGE_GUARD;
        VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, SizeOfMemory, NewProtect, &OldProtect);
    }
    NewBreakPoint.BreakPointActive = UE_BPXACTIVE;
    NewBreakPoint.BreakPointAddress = MemoryStart;
    NewBreakPoint.BreakPointType = BreakPointType;
    NewBreakPoint.BreakPointSize = SizeOfMemory;
    NewBreakPoint.MemoryBpxRestoreOnHit = (BYTE)RestoreOnHit;
    NewBreakPoint.ExecuteCallBack = (ULONG_PTR)bpxCallBack;
    BreakPointBuffer.push_back(NewBreakPoint);
    return true;
}

__declspec(dllexport) bool TITCALL RemoveMemoryBPX(ULONG_PTR MemoryStart, SIZE_T SizeOfMemory)
{
    MutexLocker lock("BreakPointBuffer");
    MEMORY_BASIC_INFORMATION MemInfo;
    ULONG_PTR NumberOfBytesReadWritten = 0;
    DWORD NewProtect = 0;
    DWORD OldProtect = 0;
    int bpcount=BreakPointBuffer.size();
    int found=-1;
    //search for breakpoint
    for(int i=0; i<bpcount; i++)
    {
        if(BreakPointBuffer.at(i).BreakPointAddress == MemoryStart &&
                (BreakPointBuffer.at(i).BreakPointType == UE_MEMORY ||
                 BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_READ ||
                 BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_WRITE ||
                 BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_EXECUTE)
          )
        {
            found=i;
            break;
        }
    }
    if(found==-1) //not found
        return false;
    VirtualQueryEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, &MemInfo, sizeof MEMORY_BASIC_INFORMATION);
    OldProtect = MemInfo.Protect;
    NewProtect = OldProtect & ~PAGE_GUARD;
    if(SizeOfMemory)
        VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, SizeOfMemory, NewProtect, &OldProtect);
    else
        VirtualProtectEx(dbgProcessInformation.hProcess, (LPVOID)MemoryStart, BreakPointBuffer.at(found).BreakPointSize, NewProtect, &OldProtect);
    BreakPointBuffer.erase(BreakPointBuffer.begin()+found);
    return true;
}

__declspec(dllexport) bool TITCALL GetUnusedHardwareBreakPointRegister(LPDWORD RegisterIndex)
{
    return EngineIsThereFreeHardwareBreakSlot(RegisterIndex);
}

__declspec(dllexport) bool TITCALL SetHardwareBreakPoint(ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack)
{
    HWBP_SIZE hwbpSize;
    HWBP_MODE hwbpMode;
    HWBP_TYPE hwbpType;
    int hwbpIndex=-1;
    DR7 dr7;

    switch(bpxSize)
    {
    case UE_HARDWARE_SIZE_1:
        hwbpSize=SIZE_1;
        break;
    case UE_HARDWARE_SIZE_2:
        hwbpSize=SIZE_2;
        if((bpxAddress%2)!=0)
            return false;
        break;
    case UE_HARDWARE_SIZE_4:
        hwbpSize=SIZE_4;
        if((bpxAddress%4)!=0)
            return false;
        break;
    case UE_HARDWARE_SIZE_8:
        hwbpSize=SIZE_8;
        if((bpxAddress%8)!=0)
            return false;
        break;
    default:
        return false;
    }

    if(!IndexOfRegister)
    {
        if(!DebugRegister[0].DrxEnabled)
            IndexOfRegister = UE_DR0;
        else if(!DebugRegister[1].DrxEnabled)
            IndexOfRegister = UE_DR1;
        else if(!DebugRegister[2].DrxEnabled)
            IndexOfRegister = UE_DR2;
        else if(!DebugRegister[3].DrxEnabled)
            IndexOfRegister = UE_DR3;
        else
            return false;
    }

    switch(IndexOfRegister)
    {
    case UE_DR0:
        hwbpIndex=0;
        break;
    case UE_DR1:
        hwbpIndex=1;
        break;
    case UE_DR2:
        hwbpIndex=2;
        break;
    case UE_DR3:
        hwbpIndex=3;
        break;
    default:
        return false;
    }

    uintdr7((ULONG_PTR)GetContextData(UE_DR7), &dr7);

    DebugRegister[hwbpIndex].DrxExecution=false;

    switch(bpxType)
    {
    case UE_HARDWARE_EXECUTE:
        hwbpSize=SIZE_1;
        hwbpType=TYPE_EXECUTE;
        DebugRegister[hwbpIndex].DrxExecution=true;
        break;
    case UE_HARDWARE_WRITE:
        hwbpType=TYPE_WRITE;
        break;
    case UE_HARDWARE_READWRITE:
        hwbpType=TYPE_READWRITE;
        break;
    default:
        return false;
    }

    hwbpMode=MODE_LOCAL;

    dr7.HWBP_MODE[hwbpIndex]=hwbpMode;
    dr7.HWBP_SIZE[hwbpIndex]=hwbpSize;
    dr7.HWBP_TYPE[hwbpIndex]=hwbpType;

    SetContextData(UE_DR7, dr7uint(&dr7)); //NOTE: MUST SET THIS FIRST FOR X64!
    SetContextData(IndexOfRegister, (ULONG_PTR)bpxAddress);

    DebugRegister[hwbpIndex].DrxBreakPointType=bpxType;
    DebugRegister[hwbpIndex].DrxBreakPointSize=bpxSize;
    DebugRegister[hwbpIndex].DrxEnabled=true;
    DebugRegister[hwbpIndex].DrxBreakAddress=(ULONG_PTR)bpxAddress;
    DebugRegister[hwbpIndex].DrxCallBack=(ULONG_PTR)bpxCallBack;

    return true;
}

__declspec(dllexport) bool TITCALL SetHardwareBreakPointEx(HANDLE hActiveThread, ULONG_PTR bpxAddress, DWORD IndexOfRegister, DWORD bpxType, DWORD bpxSize, LPVOID bpxCallBack, LPDWORD IndexOfSelectedRegister)
{
    HWBP_SIZE hwbpSize;
    HWBP_MODE hwbpMode;
    HWBP_TYPE hwbpType;
    int hwbpIndex=-1;
    DR7 dr7;

    switch(bpxSize)
    {
    case UE_HARDWARE_SIZE_1:
        hwbpSize=SIZE_1;
        break;
    case UE_HARDWARE_SIZE_2:
        hwbpSize=SIZE_2;
        if((bpxAddress%2)!=0)
            return false;
        break;
    case UE_HARDWARE_SIZE_4:
        hwbpSize=SIZE_4;
        if((bpxAddress%4)!=0)
            return false;
        break;
    case UE_HARDWARE_SIZE_8:
        hwbpSize=SIZE_8;
        if((bpxAddress%8)!=0)
            return false;
        break;
    default:
        return false;
    }

    if(!IndexOfRegister)
    {
        if(!DebugRegister[0].DrxEnabled)
            IndexOfRegister = UE_DR0;
        else if(!DebugRegister[1].DrxEnabled)
            IndexOfRegister = UE_DR1;
        else if(!DebugRegister[2].DrxEnabled)
            IndexOfRegister = UE_DR2;
        else if(!DebugRegister[3].DrxEnabled)
            IndexOfRegister = UE_DR3;
        else
            return false;
    }

    if(IndexOfSelectedRegister)
        *IndexOfSelectedRegister=IndexOfRegister;

    switch(IndexOfRegister)
    {
    case UE_DR0:
        hwbpIndex=0;
        break;
    case UE_DR1:
        hwbpIndex=1;
        break;
    case UE_DR2:
        hwbpIndex=2;
        break;
    case UE_DR3:
        hwbpIndex=3;
        break;
    default:
        return false;
    }

    uintdr7((ULONG_PTR)GetContextDataEx(hActiveThread, UE_DR7), &dr7);

    DebugRegister[hwbpIndex].DrxExecution=false;

    switch(bpxType)
    {
    case UE_HARDWARE_EXECUTE:
        hwbpSize=SIZE_1;
        hwbpType=TYPE_EXECUTE;
        DebugRegister[hwbpIndex].DrxExecution=true;
        break;
    case UE_HARDWARE_WRITE:
        hwbpType=TYPE_WRITE;
        break;
    case UE_HARDWARE_READWRITE:
        hwbpType=TYPE_READWRITE;
        break;
    default:
        return false;
    }

    hwbpMode=MODE_LOCAL;

    dr7.HWBP_MODE[hwbpIndex]=hwbpMode;
    dr7.HWBP_SIZE[hwbpIndex]=hwbpSize;
    dr7.HWBP_TYPE[hwbpIndex]=hwbpType;

    SetContextDataEx(hActiveThread, UE_DR7, dr7uint(&dr7));
    SetContextDataEx(hActiveThread, IndexOfRegister, (ULONG_PTR)bpxAddress);

    DebugRegister[hwbpIndex].DrxBreakPointType=bpxType;
    DebugRegister[hwbpIndex].DrxBreakPointSize=bpxSize;
    DebugRegister[hwbpIndex].DrxEnabled=true;
    DebugRegister[hwbpIndex].DrxBreakAddress=(ULONG_PTR)bpxAddress;
    DebugRegister[hwbpIndex].DrxCallBack=(ULONG_PTR)bpxCallBack;

    return true;
}

__declspec(dllexport) bool TITCALL DeleteHardwareBreakPoint(DWORD IndexOfRegister)
{
    ULONG_PTR HardwareBPX = NULL;
    ULONG_PTR bpxAddress = NULL;

    if(IndexOfRegister == UE_DR0)
    {
        HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
        HardwareBPX = HardwareBPX &~ (1 << 0);
        HardwareBPX = HardwareBPX &~ (1 << 1);
        SetContextData(UE_DR0, (ULONG_PTR)bpxAddress);
        SetContextData(UE_DR7, HardwareBPX);
        DebugRegister[0].DrxEnabled = false;
        DebugRegister[0].DrxBreakAddress = NULL;
        DebugRegister[0].DrxCallBack = NULL;
        return true;
    }
    else if(IndexOfRegister == UE_DR1)
    {
        HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
        HardwareBPX = HardwareBPX &~ (1 << 2);
        HardwareBPX = HardwareBPX &~ (1 << 3);
        SetContextData(UE_DR1, (ULONG_PTR)bpxAddress);
        SetContextData(UE_DR7, HardwareBPX);
        DebugRegister[1].DrxEnabled = false;
        DebugRegister[1].DrxBreakAddress = NULL;
        DebugRegister[1].DrxCallBack = NULL;
        return true;
    }
    else if(IndexOfRegister == UE_DR2)
    {
        HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
        HardwareBPX = HardwareBPX &~ (1 << 4);
        HardwareBPX = HardwareBPX &~ (1 << 5);
        SetContextData(UE_DR2, (ULONG_PTR)bpxAddress);
        SetContextData(UE_DR7, HardwareBPX);
        DebugRegister[2].DrxEnabled = false;
        DebugRegister[2].DrxBreakAddress = NULL;
        DebugRegister[2].DrxCallBack = NULL;
        return true;
    }
    else if(IndexOfRegister == UE_DR3)
    {
        HardwareBPX = (ULONG_PTR)GetContextData(UE_DR7);
        HardwareBPX = HardwareBPX &~ (1 << 6);
        HardwareBPX = HardwareBPX &~ (1 << 7);
        SetContextData(UE_DR3, (ULONG_PTR)bpxAddress);
        SetContextData(UE_DR7, HardwareBPX);
        DebugRegister[3].DrxEnabled = false;
        DebugRegister[3].DrxBreakAddress = NULL;
        DebugRegister[3].DrxCallBack = NULL;
        return true;
    }
    else
    {
        return false;
    }
    return false;
}

__declspec(dllexport) bool TITCALL RemoveAllBreakPoints(DWORD RemoveOption)
{
    MutexLocker lock("BreakPointBuffer");
    int bpcount=BreakPointBuffer.size();
    if(RemoveOption == UE_OPTION_REMOVEALL)
    {
        for(int i=bpcount-1; i>-1; i--)
        {
            if(BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT)
            {
                lock.unlock();
                DeleteBPX((ULONG_PTR)BreakPointBuffer.at(i).BreakPointAddress);
                lock.relock();
            }
            else if(BreakPointBuffer.at(i).BreakPointType == UE_MEMORY ||
                    BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_READ ||
                    BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_WRITE ||
                    BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_EXECUTE)
            {
                lock.unlock();
                RemoveMemoryBPX((ULONG_PTR)BreakPointBuffer.at(i).BreakPointAddress, BreakPointBuffer.at(i).BreakPointSize);
                lock.relock();
            }
        }
        DeleteHardwareBreakPoint(UE_DR0);
        DeleteHardwareBreakPoint(UE_DR1);
        DeleteHardwareBreakPoint(UE_DR2);
        DeleteHardwareBreakPoint(UE_DR3);
        return true;
    }
    else if(RemoveOption == UE_OPTION_DISABLEALL)
    {
        for(int i=bpcount-1; i>-1; i--)
        {
            if(BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT && BreakPointBuffer.at(i).BreakPointActive == UE_BPXACTIVE)
            {
                lock.unlock();
                DisableBPX((ULONG_PTR)BreakPointBuffer.at(i).BreakPointAddress);
                lock.relock();
            }
            else if(BreakPointBuffer.at(i).BreakPointType == UE_MEMORY ||
                    BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_READ ||
                    BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_WRITE ||
                    BreakPointBuffer.at(i).BreakPointType == UE_MEMORY_EXECUTE)
            {
                lock.unlock();
                RemoveMemoryBPX((ULONG_PTR)BreakPointBuffer.at(i).BreakPointAddress, BreakPointBuffer.at(i).BreakPointSize);
                lock.relock();
            }
        }
        return true;
    }
    else if(RemoveOption == UE_OPTION_REMOVEALLDISABLED)
    {
        for(int i=bpcount-1; i>-1; i--)
        {
            if(BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT && BreakPointBuffer.at(i).BreakPointActive == UE_BPXINACTIVE)
            {
                lock.unlock();
                DeleteBPX((ULONG_PTR)BreakPointBuffer.at(i).BreakPointAddress);
                lock.relock();
            }
        }
        return true;
    }
    else if(RemoveOption == UE_OPTION_REMOVEALLENABLED)
    {
        for(int i=bpcount-1; i>-1; i--)
        {
            if(BreakPointBuffer.at(i).BreakPointType == UE_BREAKPOINT && BreakPointBuffer.at(i).BreakPointActive == UE_BPXACTIVE)
            {
                lock.unlock();
                DeleteBPX((ULONG_PTR)BreakPointBuffer.at(i).BreakPointAddress);
                lock.relock();
            }
        }
        return true;
    }
    return false;
}