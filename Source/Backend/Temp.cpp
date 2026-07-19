#define _WIN32_DCOM

#include "Temp.h"
#include "AMD/Utility.hpp"
#include "AMD/ICPUEx.h"
#include "AMD/IPlatform.h"
#include "AMD/IDeviceManager.h"
#include "AMD/IBIOSEx.h"

#define MAX_LENGTH 50
typedef IPlatform& (__stdcall* GetPlatformFunc)();

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "shlwapi.lib")

// Retry version (hope it works)

enum CPU_PackageType
{
	cptFP5 = 0,
	cptAM5 = 0,
	cptFP7 = 1,
	cptFL1 = 1,
	cptFP8 = 1,
	cptAM4 = 2,
	cptFP7r2 = 2,
	cptAM5_B0 = 3,
	cptSP3 = 4,
	cptFP7_B0 = 4,
	cptFP7R2_B0 = 5,
	cptSP3r2 = 7,
	cptSP6 = 8,
	cptUnknown = 0xF
};

inline void printFunc(LPCTSTR func, BOOL bCore, int i = 0)
{
	if (!bCore)
	{
		_tprintf(_T("%s "), func);
		for (size_t j = 0; j < MAX_LENGTH - _tcslen(func); ++j)
		{
			_tprintf(_T("%c"), '.');
		}
	}
	else
	{
		_tprintf(_T("%s Core : %-2d"), func, i);
		for (size_t j = 0; j < MAX_LENGTH - _tcslen(func) - 9; ++j)
		{
			_tprintf(_T("%c"), '.');
		}
	}
}

VOID ShowError(LPCTSTR userMsg, BOOL printErrorMsg, DWORD exitCode)
{
	DWORD eMsgLen, errNum = GetLastError();
	LPTSTR localSysMsg;

	_ftprintf(stderr, _T("%s\n"), userMsg);

	if (printErrorMsg) {
		eMsgLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, errNum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&localSysMsg, 0, NULL);

		if (eMsgLen > 0)
		{
			_ftprintf(stderr, _T("%s\n"), localSysMsg);
		}
		else
		{
			_ftprintf(stderr, _T("Last known Error; %d.\n"), errNum);
		}

		if (localSysMsg != NULL) LocalFree(localSysMsg);
	}

	if (exitCode > 0)
		ExitProcess(exitCode);

	return;
}

std::wstring GetWinSon()
{
	std::wstring wsOSVersion = L"Unknown";
	std::wstring wsBuildNumber = {};
	std::wstring wsMajorVersion = {};
	std::wstring wsMinorVersion = {};

	bool bRetCode = g_GetRegistryValue(HKEY_LOCAL_MACHINE, MS_OS_Version_REGISTRY_PATH, L"CurrentMajorVersionNumber", wsMajorVersion, true);
	if (!bRetCode)
		return wsOSVersion;

	bRetCode = g_GetRegistryValue(HKEY_LOCAL_MACHINE, MS_OS_Version_REGISTRY_PATH, L"CurrentMinorVersionNumber", wsMinorVersion, true);
	if (!bRetCode)
		return wsOSVersion;

	bRetCode = g_GetRegistryValue(HKEY_LOCAL_MACHINE, MS_OS_Version_REGISTRY_PATH, L"CurrentBuild", wsBuildNumber);
	if (!bRetCode)
		return wsOSVersion;

	wsOSVersion = wsMajorVersion + L"." + wsMinorVersion + L"." + wsBuildNumber;

	return wsOSVersion;
}

std::wstring GetSysName()
{
	WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(computerName) / sizeof(computerName[0]);
	if (GetComputerName(computerName, &size))
	{
		return computerName;
	}
	return L"Unknown System Name";
}

INT IsDRVInstalled()
{
	SERVICE_STATUS ServiceStatus;
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM)
		return -1;

	SC_HANDLE hOpenService = OpenService(hSCM, RM_DRIVER_NAME, SC_MANAGER_ALL_ACCESS);
	if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
	{
		CloseServiceHandle(hOpenService);
		CloseServiceHandle(hSCM);
		return   -1;
	}
	QueryServiceStatus(hOpenService, &ServiceStatus);
	if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
	{
		CloseServiceHandle(hOpenService);
		CloseServiceHandle(hSCM);
		return -1;
	}

	CloseServiceHandle(hOpenService);
	CloseServiceHandle(hSCM);
	return 0;
}

BOOL Authentic_AMD()
{
	char CPUString[0x20];
	int CPUInfo[4] = { -1 };
	unsigned nIds;
	char string[] = "AuthenticAMD";

	__cpuid(CPUInfo, 0);
	nIds = CPUInfo[0];
	memset(CPUString, 0, sizeof(CPUString));
	*((int*)CPUString) = CPUInfo[1];
	*((int*)(CPUString + 4)) = CPUInfo[3];
	*((int*)(CPUString + 8)) = CPUInfo[2];

	if (!strcmp(string, CPUString))
		return true;
	else
		return false;

}

BOOL IsSuppCPU(VOID)
{
	bool retBool = false;
	int CPUInfo[4] = { -1 };
	__cpuid(CPUInfo, 0x80000001);
	unsigned long uCPUID = CPUInfo[0];
	CPU_PackageType pkgType = (CPU_PackageType)((CPUInfo[1] >> 28) & 0x0F);

	switch (pkgType)
	{
	case cptFP5:
		// If it's AM5
		switch (uCPUID)
		{
		case 0x00810F80:
		case 0x00810F81:
		case 0x00860F00:
		case 0x00860F01:
		case 0x00A50F00:
		case 0x00A50F01:
		case 0x00860F81:
		case 0x00A60F00:
		case 0x00A60F01:
		case 0x00A60F10:
		case 0x00A60F11:
		case 0x00A60F12:
		case 0x00A60F13:
		case 0x00A70F80:
		case 0x00A70F52:

		case 0x00B40F40:
		case 0x00B40F41:
			retBool = true;
			break;
		default:
			break;
		}
		break;

	// If its AM4
	case cptAM4:
	case cptFP7R2_B0:
		switch (uCPUID)
		{
		case 0x00800F00:
		case 0x00800F10:
		case 0x00800F11:
		case 0x00800F12:

		case 0x00810F10:
		case 0x00810F11:

		case 0x00800F82:
		case 0x00800F83:

		case 0x00870F00:
		case 0x00870F10:

		case 0x00810F80:
		case 0x00810F81:

		case 0x00860F00:
		case 0x00860F01:

		case 0x00A20F00:
		case 0x00A20F10:
		case 0x00A20F12:

		case 0x00A50F00:
		case 0x00A50F01:

		//cptFP7r2
		case 0x00A40F00:
		case 0x00A40F40:
		case 0x00A40F41:

		case 0x00A70F00:
		case 0x00A70F40:
		case 0x00A70F41:
		case 0x00A70F42:
		case 0x00A70F80:
		
		case 0x00A70F52:
		case 0x00A70FC0:
			retBool = true;
			break;
		default:
			break;
		}
		break;

	case cptSP3r2:
		switch (uCPUID)
		{
		case 0x00800F10:
		case 0x00800F11:
		case 0x00800F12:

		case 0x00800F82:
		case 0x00800F83:
		case 0x00830F00:
		case 0x00830F10:
			retBool = true;
			break;
		case 0x00B00F11:
		case 0x00B00F80:
		case 0x00B00F81:
			retBool = true;
			break;
		default:
			break;
		}
		break;

	case cptFP7:
	
	//case cptFL1:
	//case cptFP8:
	//case cptFP7_B0:
	case cptSP3:
		switch (uCPUID)
		{
		case 0x00A40F00:
		case 0x00A40F40:
		case 0x00A40F41:

		case 0x00A60F11:
		case 0x00A60F12:
			retBool = true;
			break;
		case 0x00A00F80:
		case 0x00A00F82:

		case 0x00A70F00:
		case 0x00A70F40:
		case 0x00A70F41:
		case 0x00A70F42:
		case 0x00A70F80:
		case 0x00A70F52:
		case 0x00A70FC0:
		case 0x00B20F40:

		case 0x00B40F40:
			retBool = true;
			break;

			//STXH - FP11
		case 0x00B70F00:
			retBool = true;
			break;
			//KRK
		case 0x00B60F00:
			retBool = true;
			break;
			//KRK2
		case 0x00B60F80:
			retBool = true;
			break;
		default:
			break;
		}
		break;

	case cptSP6:
		switch (uCPUID)
		{
		case 0x00A10F81:
		case 0x00A10F80:
			retBool = true;
			break;
		}
		break;

	default:
		break;
	}
	return retBool;
}

BOOL IsUserAdmin(VOID)
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup );

    if(b) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
             b = FALSE;
        }

        FreeSid(AdministratorsGroup);
    }

    return(b);
}

bool IsWindows()
{
	bool bIsWindows = false;
	DWORD major = 0;
	DWORD minor = 0;
	LPBYTE pinfoRawData;
	if (IsWindowsServer())
	{
		return false;
	}
	if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
	{
		WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
		major = pworkstationInfo->wki100_ver_major;
		minor = pworkstationInfo->wki100_ver_minor;
		::NetApiBufferFree(pinfoRawData);
	}

	if (major >= 10)
	{
		bIsWindows = true;
	}
	
	return bIsWindows;
}

bool GetDriverPath(wchar_t* pDriverPath)
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
	std::wstring exeDir(exePath);
    exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/"));

    std::wstring fullPath = exeDir + L"\\Bin\\AMDRyzenMasterDriver.sys";
    wcscpy(pDriverPath, fullPath.c_str());

    return true;
}

bool g_GetRegistryValue(HKEY hRootKey, LPCWSTR keyPath, const wchar_t* valueName, std::wstring& ulValue, bool bIsDWORD)
{
	if (!valueName || (wcslen(valueName) == 0)) return false;
	HKEY hKey = NULL;
	DWORD dwLength = MAX_STRING_LEN;

	HRESULT hr = RegOpenKey(hRootKey, keyPath, &hKey);
	if (hr != ERROR_SUCCESS) return false;
	wchar_t buff[MAX_STRING_LEN] = { '\n' };

	if (bIsDWORD)
	{
		dwLength = sizeof(DWORD);
		DWORD dwValue = 0;
		hr = RegQueryValueEx(hKey, valueName, 0, NULL, reinterpret_cast<LPBYTE>(&dwValue), &dwLength);
		ulValue = std::to_wstring(dwValue);
	}
	else
	{
		hr = RegQueryValueEx(hKey, valueName, 0, NULL, reinterpret_cast<LPBYTE>(&buff), &dwLength);
		ulValue = std::wstring(buff);
	}
	RegCloseKey(hKey);

	return hr == ERROR_SUCCESS;
}

// 🙏🙏🙏🙏🙏🙏🙏
bool InstallDriver(void)
{
	bool bRetCode = false;
	bool bResult = false;
	DWORD dwLastError;
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService = NULL;
	wchar_t szDriverPath[256];


	HANDLE m_hDriver = CreateFile(L"\\\\.\\" RM_DRIVER_NAME,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (m_hDriver == INVALID_HANDLE_VALUE) {
		bRetCode = GetDriverPath(szDriverPath);
		LOG_PROCESS_ERROR(bRetCode);

		hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		LOG_PROCESS_ERROR(hSCManager);

		// Install the driver
		hService = CreateService(hSCManager,
			RM_DRIVER_NAME, RM_DRIVER_NAME, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
			SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, szDriverPath,
			NULL, NULL, NULL, NULL, NULL);
		
			if (hService == NULL) {
			dwLastError = GetLastError();
			
			if (dwLastError == ERROR_SERVICE_EXISTS)
				hService = OpenService(hSCManager, RM_DRIVER_NAME, SERVICE_ALL_ACCESS);
			
				else if (dwLastError == ERROR_SERVICE_MARKED_FOR_DELETE) {
				hService = OpenService(hSCManager, RM_DRIVER_NAME, SERVICE_ALL_ACCESS);
				SERVICE_STATUS ServiceStatus;
				ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);
				CloseServiceHandle(hService);
				
				hService = CreateService(hSCManager,
					RM_DRIVER_NAME, RM_DRIVER_NAME, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
					SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, szDriverPath,
					NULL, NULL, NULL, NULL, NULL);
			}
			
			else
				printf("InstallDriver: error code returned from CreateService is: %d", GetLastError());
		}
		LOG_PROCESS_ERROR(hService);

		// Start the driver
		BOOL bRet = StartService(hService, 0, NULL);
		if (!bRet) {
			dwLastError = GetLastError();
			if (dwLastError == ERROR_PATH_NOT_FOUND) {
				bRet = DeleteService(hService);
				LOG_PROCESS_ERROR(bRet);

				CloseServiceHandle(hService);

				hService = CreateService(hSCManager,
					RM_DRIVER_NAME, RM_DRIVER_NAME, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
					SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, szDriverPath,
					NULL, NULL, NULL, NULL, NULL);
				LOG_PROCESS_ERROR(hService);

				bRet = StartService(hService, 0, NULL);
				LOG_PROCESS_ERROR(bRet);
			}

			if (dwLastError != ERROR_SERVICE_ALREADY_RUNNING) {
				LOG_PROCESS_ERROR(bRet);
			}
		}

		// Try to create the file again
		m_hDriver = CreateFile(L"\\\\.\\" RM_DRIVER_NAME,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		LOG_PROCESS_ERROR(m_hDriver != INVALID_HANDLE_VALUE);
	}

	bResult = true;

Exit0:
	if (m_hDriver != INVALID_HANDLE_VALUE)
		CloseHandle(m_hDriver);

	if (hSCManager)
		CloseServiceHandle(hSCManager);
	if (hService)
		CloseServiceHandle(hService);


	return bResult;
}

TCHAR PrintErr[][63] = { _T("Failure"),
						 _T("Success") ,
						 _T("Invalid value"),
						 _T("Method is not implemented by the BIOS"),
						 _T("Cores are already parked. First Enable all the cores"),
						 _T("Unsupported Function") };

void CallAMDAPI()
{    
    bool bRetCode = false;
	
	HMODULE hPlatform_Handle = nullptr;
	LPCWSTR path = {};
	
	std::wstring buff = {};
	DWORD dwTemp = 0;
	
	bRetCode = g_GetRegistryValue(HKEY_LOCAL_MACHINE, AMDRM_Monitoring_SDK_REGISTRY_PATH, L"InstallationPath", buff, dwTemp);
	if (!bRetCode)
	{
		_tprintf(_T("Error E1001. Folder should be missing\n"));
		return;
	}

	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);
	
	std::wstring exeDir(exePath);
	exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/"));
	
	std::wstring temp = exeDir + L"\\Bin\\Platform.dll";
	path = temp.c_str();

	// hPlatform_Handle = LoadLibraryEx(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
	hPlatform_Handle = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	
	if (!hPlatform_Handle) {
		_tprintf(_T("Error E1002. You are missing Platform.dll\n"));
		return;
	}
	
	GetPlatformFunc platformFunc = (GetPlatformFunc)GetProcAddress(hPlatform_Handle, "GetPlatform");
	if (!platformFunc) {
		FreeLibrary(hPlatform_Handle);
		_tprintf(_T("Error E1003. Platform initilization failed\n"));
		return;
	}
	
	IPlatform& rPlatform = platformFunc();
	bRetCode = rPlatform.Init();
	if (!bRetCode) {
		FreeLibrary(hPlatform_Handle);
		_tprintf(_T("Error E1004. Platform initilization failed\n"));
		return;
	}
	
	IDeviceManager& rDeviceManager = rPlatform.GetIDeviceManager();
	ICPUEx* obj = (ICPUEx*)rDeviceManager.GetDevice(dtCPU, 0);
	IBIOSEx* objB = (IBIOSEx*)rDeviceManager.GetDevice(dtBIOS, 0);
	
	if (obj && objB) {
		
		CACHE_INFO result;
		CPUParameters stData;
		
		int i = 0;
		int j = 0;
		int iRet = 0;

		iRet = obj->GetCPUParameters(stData);
		if (iRet) {
			_tprintf(_T(" %s\n"), PrintErr[iRet + 1]);
		}
		
		else {

			std::cout << "Platform Init: " << (bRetCode ? "SUCCESS" : "FAILED") << std::endl;

			bool driverRunning = false;
			HANDLE hDriver = CreateFile(L"\\\\.\\AMDRyzenMaster", GENERIC_READ | GENERIC_WRITE, 
										FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			
			if (hDriver != INVALID_HANDLE_VALUE) {
				driverRunning = true;
				CloseHandle(hDriver);
			}
			
			std::cout << "Driver accessible: " << (driverRunning ? "YES" : "NO") << std::endl;

			Sleep(800);

			printFunc(_T("GetCPUParameters"), FALSE);
			LOG_PRINT(stData.fcHTCLimit, _T("Celsius"));

			std::cout << std::left << std::setw(10) << "\tTemperature(C)" << std::endl;
			for (i = 0; i < stData.stFreqData.uLength && nullptr != stData.stFreqData.dCurrentFreq; i++)
			{
				if (stData.stFreqData.dCurrentFreq[i] != 0) {
					std::cout << std::fixed << std::setprecision(1) << std::left << "\t" << std::setw(10) << j
					<< std::setw(15) << stData.stFreqData.dCurrentFreq[i] << std::setw(15) << stData.stFreqData.dFreq[i] << std::setw(15)
					<< stData.stFreqData.dState[i] << std::setw(15) << stData.stFreqData.dCurrentTemp[i] << std::endl;
					j++;
				}
			}
		
			printFunc(_T("GetCurrentTemperature"), FALSE);
			_tprintf(_T(" %0.2f Celsius\n"), stData.dTemperature);
		}
	}
	
	rPlatform.UnInit();
	FreeLibrary(hPlatform_Handle);
}

int main(int argc, char **argv)
{
	
	// Add a Boolean that will check if admin is running i will add it in the near future not now
	// Made it :) (i hope it works)
	if (!IsUserAdmin())
		ShowError(_T("Make sure the program is running as Admin for the Driver to load"), FALSE, 1);

    if (!Authentic_AMD())
		ShowError(_T("Either You Don't Have an AMD Processor Or The Program Didn't Manage To Find It"), FALSE, 1);

	if (!IsWindows())
		ShowError(_T("You Must Have Windows 10 or Higher"), FALSE, 1);

	if (IsDRVInstalled() < 0) {
		if (false == InstallDriver())
			ShowError(_T("Unable to install driver AMDRyzenMasterDriver.sys because Driver could not be found"), FALSE, 1);
	}

	if (!IsSuppCPU())
		ShowError(_T("The CPU is not supported."), FALSE, 1);
	
	CallAMDAPI();

	system("pause");
    return 0;
}


// My older version

/*
HRESULT GetCPUTemps(LPLONG pTemperature){
    if (pTemperature == NULL)
        return E_INVALIDARG;
    
    *pTemperature = -1;
    
    HRESULT ci = CoInitialize(NULL);
    HRESULT hr = CoInitializeSecurity(NULL, 
                                      -1  , 
                                      NULL, 
                                      NULL, 
                                      RPC_C_AUTHN_LEVEL_DEFAULT  , 
                                      RPC_C_IMP_LEVEL_IMPERSONATE, 
                                      NULL, 
                                      EOAC_NONE, 
                                      NULL);

    if (SUCCEEDED(hr)){
        IWbemLocator *pLocator;
        hr = CoCreateInstance(CLSID_WbemAdministrativeLocator, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IWbemLocator, 
                              (LPVOID*)&pLocator);

        if (SUCCEEDED(hr)){

            IWbemServices *pServices;
            //BSTR ns = SysAllocString(L"root\\LibreHardwareMonitor");
            BSTR ns = SysAllocString(L"root\\WMI");
            
            hr = pLocator->ConnectServer(ns, 
                                         NULL, 
                                         NULL, 
                                         NULL, 
                                         0,
                                         NULL, 
                                         NULL, 
                                         &pServices);
            pLocator->Release();
            SysFreeString(ns);
            
            if (SUCCEEDED(hr)){
                //BSTR query = SysAllocString(L"SELECT * FROM MSAcpi_ThermalZoneTemperature"); //  * FROM MSAcpi_ThermalZoneTemperature / CurrentTemperature FROM MSWMI_SMBiosTypeProcessor
                
                BSTR query = SysAllocString(L"SELECT Value FROM Sensor WHERE SensorType='Temperature' AND Name LIKE '%CPU%'");
                BSTR wql = SysAllocString(L"WQL");
                                                   
                IEnumWbemClassObject *pEnum;
                hr = pServices->ExecQuery(wql, 
                                          query,
                                          WBEM_FLAG_RETURN_IMMEDIATELY |
                                          WBEM_FLAG_FORWARD_ONLY,
                                          NULL, 
                                          &pEnum);
                
                SysFreeString(wql);
                SysFreeString(query);
                
                pServices->Release();
                
                if (SUCCEEDED(hr)){
                    IWbemClassObject *pObject;
                    ULONG returned;
                    
                    hr = pEnum->Next(WBEM_INFINITE,
                                     1,
                                     &pObject,
                                     &returned);
                    pEnum->Release();
                    
                    if(SUCCEEDED(hr)){
                        BSTR temp = SysAllocString(L"CurrentTemperature");
                        VARIANT v;
                        VariantInit(&v);

                        hr = pObject->Get(temp,
                                          0,
                                          &v,
                                          NULL,
                                          NULL);
                        
                        pObject->Release();
                        SysFreeString(temp);

                        if(SUCCEEDED(hr))
                            *pTemperature = V_I4(&v);
                        
                        VariantClear(&v);
                    }
                }
            }
            if (ci == S_OK)
                CoUninitialize();
        }
    }
    return hr;
}

    while (true){
            LONG temp;
            HRESULT hr = GetCPUTemps(&temp);

            if (SUCCEEDED(hr) && temp != 0){

                double celsius = ((double)temp / 10.0) - 273.15;
                printf("CPU Temp = %lf\n", celsius);    
            } 
            
            else{

                printf("The Programm didn't manage to find the CPU temperature.\n");
            }
        }
        
        getc(stdin);

*/