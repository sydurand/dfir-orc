//
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright © 2011-2019 ANSSI. All Rights Reserved.
//
// Author(s): Jean Gautier (ANSSI)
//
#include "stdafx.h"

#include "DriverMgmt.h"
#include "EmbeddedResource.h"
#include "ParameterCheck.h"
#include "Temporary.h"
#include "SystemDetails.h"
#include "Robustness.h"

#include "Buffer.h"

#include "boost\scope_exit.hpp"

using namespace Orc;

class Orc::DriverTermination : public TerminationHandler
{

    friend class DriverMgmt;
    friend class Driver;

public:
    DriverTermination(const std::shared_ptr<Driver>& driver)
        : TerminationHandler(driver->Name(), ROBUSTNESS_TEMPFILE)
        , m_driver(driver)
    {
    }

    HRESULT operator()();

private:
    std::weak_ptr<Driver> m_driver;
};

HRESULT DriverTermination::operator()()
{

    HRESULT hr = E_FAIL;

    auto driver = m_driver.lock();

    if (driver == nullptr)
        return S_OK;  // Driver is already deleted

    if (!driver->Name().empty())
    {

        SC_HANDLE SchSCManager =
            OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT | SC_MANAGER_ALL_ACCESS);

        if (SchSCManager == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Log::Error("Failed OpenSCManager in termination handler (code: {:#x})", hr);
        }
        else
        {
            BOOST_SCOPE_EXIT(&SchSCManager)
            {
                if (SchSCManager != NULL)
                    CloseServiceHandle(SchSCManager);
            }
            BOOST_SCOPE_EXIT_END;

            if (FAILED(hr = Orc::DriverMgmt::StopDriver(SchSCManager, driver->Name().c_str())))
            {
                Log::Error("Failed StopDriver in termination handler (code: {:#x})", hr);
            }
            else
            {
                Log::Debug(L"Successfully stopped driver {}", driver->Name());
                if (FAILED(hr = Orc::DriverMgmt::RemoveDriver(SchSCManager, driver->Name().c_str())))
                {
                    Log::Error("Failed RemoveDriver in termination handler (code: {:#x})", hr);
                }
                else
                {
                    Log::Debug(L"Successfully removed driver '{}'", driver->Name());
                }
            }
        }
    }
    if (!driver->m_strDriverFileName.empty())
    {
        if (FAILED(hr = UtilDeleteTemporaryFile(driver->m_strDriverFileName.c_str())))
        {
            Log::Error(
                L"Attempt to remove driver file '{}' in termination handler failed (code: {:#x})",
                driver->m_strDriverFileName,
                hr);
            return hr;
        }
    }
    return S_OK;
}

HRESULT DriverMgmt::ConnectToSCM()
{
    HRESULT hr = E_FAIL;

    if (m_SchSCManager != NULL)
        return S_OK;

    m_SchSCManager = OpenSCManager(
        m_strComputerName.empty() ? NULL : m_strComputerName.c_str(),
        SERVICES_ACTIVE_DATABASE,
        SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

    if (m_SchSCManager == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error("Failed to connect to Service Control Manager (code: {:#x})", hr);
        return hr;
    }
    return S_OK;
}

HRESULT DriverMgmt::Disconnect()
{
    if (m_SchService != NULL)
    {
        auto temp = m_SchService;
        m_SchService = NULL;
        CloseServiceHandle(temp);
    }
    if (m_SchSCManager != NULL)
    {
        auto temp = m_SchSCManager;
        m_SchSCManager = NULL;
        CloseServiceHandle(temp);
    }
    return S_OK;
}

HRESULT DriverMgmt::SetTemporaryDirectory(const std::wstring& strTempDir)
{
    HRESULT hr = E_FAIL;
    if (FAILED(hr = GetOutputDir(strTempDir.c_str(), m_strTempDir, true)))
    {
        Log::Error(L"Failed to create temporary directory '{}' (code: {:#x})", strTempDir, hr);
        return hr;
    }
    return S_OK;
}

HRESULT Orc::Driver::Install(const std::wstring& strX86DriverRef, const std::wstring& strX64DriverRef)
{
    HRESULT hr = E_FAIL;

    if (FAILED(hr = m_manager->ConnectToSCM()))
        return hr;

    WORD wArch = 0;
    if (FAILED(hr = SystemDetails::GetArchitecture(wArch)))
        return hr;

    switch (wArch)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            m_strDriverRef = strX86DriverRef;
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            m_strDriverRef = strX64DriverRef;
            break;
        default:
            return E_FAIL;
    }

    auto pTerminationHandler = std::make_shared<DriverTermination>(shared_from_this());

    std::wstring strDriverFileName;

    Log::Debug(L"ExtensionLibrary: Loading value '{}'", m_strDriverRef);

    std::wstring strNewLibRef;
    if (SUCCEEDED(EmbeddedResource::ExtractValue(L"", m_strDriverRef, strNewLibRef)))
    {
        Log::Debug(L"ExtensionLibrary: Loaded value {}={} successfully", m_strDriverRef, strNewLibRef);

        if (EmbeddedResource::IsResourceBased(strNewLibRef))
        {
            if (m_manager->m_strTempDir.empty())
            {
                if (FAILED(hr = m_manager->SetTemporaryDirectory(L"%TEMP%")))
                    return hr;
            }

            if (FAILED(
                    hr = EmbeddedResource::ExtractToFile(
                        strNewLibRef,
                        m_strServiceName,
                        RESSOURCE_READ_EXECUTE_BA,
                        m_manager->m_strTempDir,
                        m_strDriverFileName)))
            {
                Log::Error(
                    L"Failed to extract driver resource '{}' into '{}' (code: {:#x})",
                    strNewLibRef,
                    m_manager->m_strTempDir,
                    hr);

                return hr;
            }
        }
    }
    else
    {
        if (EmbeddedResource::IsResourceBased(m_strDriverRef))
        {
            if (m_manager->m_strTempDir.empty())
            {
                if (FAILED(hr = m_manager->SetTemporaryDirectory(L"%TEMP%")))
                    return hr;
            }

            if (FAILED(
                    hr = EmbeddedResource::ExtractToFile(
                        m_strDriverRef,
                        m_strServiceName,
                        RESSOURCE_READ_EXECUTE_BA,
                        m_manager->m_strTempDir,
                        strDriverFileName)))
            {
                Log::Error(
                    L"Failed to extract driver resource '{}' into '{}' (code: {:#x})",
                    m_strDriverRef,
                    m_manager->m_strTempDir,
                    hr);
                return hr;
            }
        }
        else
        {
            m_strDriverFileName = m_strDriverRef;
        }
    }

    if (FAILED(
            hr = DriverMgmt::InstallDriver(
                m_manager->m_SchSCManager, m_strServiceName.c_str(), m_strDriverFileName.c_str())))
    {
        Log::Error(
            L"Failed to install driver '{}' with service name '{}' (code: {:#x})",
            m_strDriverRef,
            m_strServiceName,
            hr);

        if (m_bDeleteDriverOnClose)
        {
            if (FAILED(hr = UtilDeleteTemporaryFile(m_strDriverFileName.c_str())))
            {
                Log::Error(L"Failed to delete temporary driver file name '{}' (code: {:#x})", m_strDriverFileName, hr);
                return hr;
            }
        }
        return hr;
    }

    pTerminationHandler->m_driver = shared_from_this();

    m_manager->m_pTerminationHandlers.push_back(pTerminationHandler);
    Robustness::AddTerminationHandler(pTerminationHandler);

    return S_OK;
}

std::shared_ptr<Orc::Driver> DriverMgmt::GetDriver(
    const std::wstring& strServiceName,
    const std::wstring& strX86DriverRef,
    const std::wstring& strX64DriverRef)
{
    auto retval = std::make_shared<Driver>(shared_from_this(), strServiceName);

    if (auto hr = retval->Install(strX86DriverRef, strX64DriverRef); FAILED(hr))
    {
        Log::Error(L"Failed to install driver '{}' (code: {:#x})", strServiceName, hr);
        return nullptr;
    }
    return retval;
}

HRESULT Driver::UnInstall()
{
    HRESULT hr = E_FAIL;
    if (FAILED(hr = m_manager->ConnectToSCM()))
        return hr;

    if (FAILED(hr = DriverMgmt::RemoveDriver(m_manager->m_SchSCManager, m_strServiceName.c_str())))
    {
        Log::Error(L"Failed RemoveDriver on '{}' (code: {:#x})", m_strServiceName, hr);
        return hr;
    }

    for (auto phandler : m_manager->m_pTerminationHandlers)
    {
        auto driver = phandler->m_driver.lock();
        if (driver)
        {
            if (driver->m_strServiceName == m_strServiceName)
            {
                Robustness::RemoveTerminationHandler(phandler);

                if (!driver->m_strDriverFileName.empty() && driver->m_bDeleteDriverOnClose)
                {
                    if (FAILED(hr = UtilDeleteTemporaryFile(driver->m_strDriverFileName.c_str())))
                    {
                        Log::Error(
                            L"Failed to delete temporary driver file '{}' (code: {:#x})",
                            driver->m_strDriverFileName,
                            hr);
                    }
                }
            }
        }
    }

    return S_OK;
}

HRESULT Driver::Start()
{
    HRESULT hr = E_FAIL;
    if (FAILED(hr = m_manager->ConnectToSCM()))
        return hr;

    if (FAILED(hr = DriverMgmt::StartDriver(m_manager->m_SchSCManager, m_strServiceName.c_str())))
    {
        Log::Error(L"Failed to start driver '{}' (code: {:#x})", m_strServiceName, hr);
        return hr;
    }
    return S_OK;
}
HRESULT Driver::Stop()
{
    HRESULT hr = E_FAIL;
    if (FAILED(hr = m_manager->ConnectToSCM()))
        return hr;

    if (FAILED(hr = DriverMgmt::StopDriver(m_manager->m_SchSCManager, m_strServiceName.c_str())))
    {
        if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE)  // We ignore error if driver was already stopped...
        {
            Log::Error(L"Failed to stop driver '{}' (code: {:#x})", m_strServiceName, hr);
            return hr;
        }
    }
    return S_OK;
}

ServiceStatus Orc::Driver::GetStatus()
{
    HRESULT hr = E_FAIL;
    if (FAILED(hr = m_manager->ConnectToSCM()))
        return ServiceStatus::Inexistent;

    return ServiceStatus();
}

HRESULT Orc::DriverMgmt::InstallDriver(__in SC_HANDLE SchSCManager, __in LPCTSTR DriverName, __in LPCTSTR ServiceExe)

{
    SC_HANDLE schService = NULL;
    HRESULT hr = E_FAIL;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    //
    // Create a new a service object.
    //
    schService = CreateServiceW(
        SchSCManager,  // handle of service control manager database
        DriverName,  // address of name of service to start
        DriverName,  // address of display name
        SERVICE_ALL_ACCESS,  // type of access to service
        SERVICE_KERNEL_DRIVER,  // type of service
        SERVICE_SYSTEM_START,  // when to start service
        SERVICE_ERROR_NORMAL,  // severity if service fails to start
        ServiceExe,  // address of name of binary file
        L"Base",  // service does not belong to a group
        NULL,  // no tag requested
        NULL,  // no dependency names
        NULL,  // use LocalSystem account
        NULL  // no password for service account
    );

    BOOST_SCOPE_EXIT(&schService)
    {
        if (schService != NULL)
        {
            CloseServiceHandle(schService);
        }
    }
    BOOST_SCOPE_EXIT_END;

    if (schService == NULL)
    {

        if (GetLastError() == ERROR_SERVICE_EXISTS)
        {
            //
            // Ignore this error, make sure the parameters are rigth
            //
            schService = OpenServiceW(SchSCManager, DriverName, SERVICE_CHANGE_CONFIG);

            if (schService == NULL)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                Log::Error("Failed OpenService (code: {:#x})", hr);
                return hr;
            }
            else
            {
                if (!ChangeServiceConfigW(
                        schService,
                        SERVICE_NO_CHANGE,
                        SERVICE_SYSTEM_START,
                        SERVICE_NO_CHANGE,
                        ServiceExe,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    Log::Error("Failed ChangeServiceConfig (code: {:#x})", hr);
                    return hr;
                }
            }
            return S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Log::Error(L"Failed CreateService (code: {:#x})", hr);
            return hr;
        }
    }

    return S_OK;
}

HRESULT Orc::DriverMgmt::ManageDriver(__in LPCTSTR DriverName, __in LPCTSTR ServiceName, __in USHORT Function)
{
    HRESULT hr = E_FAIL;
    SC_HANDLE schSCManager;

    //
    // Insure (somewhat) that the driver and service names are valid.
    if (!DriverName || !ServiceName)
    {
        Log::Info(L"Invalid Driver or Service provided to ManageDriver()");
        return E_INVALIDARG;
    }

    //
    // Connect to the Service Control Manager and open the Services database.
    schSCManager = OpenSCManager(
        NULL,  // local machine
        NULL,  // local database
        SC_MANAGER_ALL_ACCESS  // access required
    );

    if (!schSCManager)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Info("Failed OpenSCManager (code: {:#x})", hr);
        return hr;
    }

    //
    // Do the requested function.
    switch (Function)
    {
        case DRIVER_FUNC_INSTALL:
            //
            // Install the driver service.
            if (FAILED(hr = InstallDriver(schSCManager, DriverName, ServiceName)))
            {
                //
                // Indicate an error.
                hr = E_FAIL;
            }
            break;

        case DRIVER_FUNC_REMOVE:
            //
            // Stop the driver.
            static_cast<void>(StopDriver(schSCManager, DriverName));
            //
            // Remove the driver service.
            static_cast<void>(RemoveDriver(schSCManager, DriverName));
            //
            // Ignore all errors.
            hr = S_OK;
            break;

        default:
            Log::Info("Unknown ManageDriver() function");
            hr = E_INVALIDARG;
            break;
    }

    //
    // Close handle to service control manager.
    if (schSCManager)
        CloseServiceHandle(schSCManager);

    return hr;
}  // ManageDriver

HRESULT Orc::DriverMgmt::RemoveDriver(SC_HANDLE SchSCManager, __in LPCTSTR DriverName)
{
    HRESULT hr = E_FAIL;
    SC_HANDLE schService;

    //
    // Open the handle to the existing service.
    schService = OpenService(SchSCManager, DriverName, DELETE);

    BOOST_SCOPE_EXIT(&schService)
    {
        // Close the service object.
        if (schService)
        {
            SC_HANDLE temp = schService;
            schService = NULL;
            CloseServiceHandle(temp);
        }
    }
    BOOST_SCOPE_EXIT_END;

    if (schService == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error("Failed OpenService (code: {:#x})", hr);
        return hr;
    }

    //
    // Mark the service for deletion from the service control manager database.
    if (!DeleteService(schService))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error(L"Failed DeleteService (code: {:#x})", hr);
        return hr;
    }

    return S_OK;
}  // RemoveDriver

HRESULT Orc::DriverMgmt::StartDriver(SC_HANDLE SchSCManager, __in LPCTSTR DriverName)
{
    HRESULT hr = E_FAIL;

    SC_HANDLE schService;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager, DriverName, SERVICE_START);
    BOOST_SCOPE_EXIT(&schService)
    {
        // Close the service object.
        if (schService)
        {
            SC_HANDLE temp = schService;
            schService = NULL;
            CloseServiceHandle(temp);
        }
    }
    BOOST_SCOPE_EXIT_END;

    if (schService == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error("Failed OpenService (code: {:#x})", hr);
        return hr;
    }

    //
    // Start the execution of the service (i.e. start the driver).
    if (!StartService(schService, 0, NULL))
    {
        DWORD err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING)
        {
            // Ignore this error.
            return S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(err);
            Log::Error("Failed StartService (code: {:#x})", hr);
            return hr;
        }
    }
    return S_OK;
}

HRESULT Orc::DriverMgmt::StopDriver(SC_HANDLE SchSCManager, __in LPCTSTR DriverName)
{
    HRESULT hr = E_FAIL;
    SC_HANDLE schService;
    SERVICE_STATUS serviceStatus;

    //
    // Open the handle to the existing service.
    schService = OpenService(SchSCManager, DriverName, SERVICE_STOP);
    BOOST_SCOPE_EXIT(&schService)
    {
        // Close the service object.
        if (schService)
        {
            SC_HANDLE temp = schService;
            schService = NULL;
            CloseServiceHandle(temp);
        }
    }
    BOOST_SCOPE_EXIT_END;

    if (schService == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error("Failed OpenService (code: {:#x})", hr);
        return hr;
    }

    //
    // Request that the service stop.
    if (!ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error("Failed ControlService (code: {:#x})", hr);
        return hr;
    }

    return S_OK;
}  //  StopDriver

HRESULT Orc::DriverMgmt::GetDriverStatus(SC_HANDLE SchSCManager, LPCTSTR DriverName)
{
    HRESULT hr = E_FAIL;
    SC_HANDLE schService;

    //
    // Open the handle to the existing service.
    schService = OpenService(SchSCManager, DriverName, SERVICE_QUERY_STATUS);
    BOOST_SCOPE_EXIT(&schService)
    {
        // Close the service object.
        if (schService)
        {
            SC_HANDLE temp = schService;
            schService = NULL;
            CloseServiceHandle(temp);
        }
    }
    BOOST_SCOPE_EXIT_END;

    if (schService == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error("Failed OpenService (code: {:#x})", hr);
        return hr;
    }

    //
    // Request that the service stop.
    Buffer<BYTE, sizeof(SERVICE_STATUS_PROCESS)> serviceStatus;
    DWORD cbNeeded = 0L;
    if (!QueryServiceStatusEx(
            schService, SC_STATUS_PROCESS_INFO, serviceStatus.get(), serviceStatus.capacity(), &cbNeeded))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            serviceStatus.reserve(cbNeeded);
            if (!QueryServiceStatusEx(
                    schService, SC_STATUS_PROCESS_INFO, serviceStatus.get(), serviceStatus.capacity(), &cbNeeded))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                Log::Error("Failed QueryServiceStatusEx (code: {:#x})", hr);
                return hr;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Log::Error(L"Failed QueryServiceStatusEx (code: {:#x})", hr);
            return hr;
        }
    }
    serviceStatus.use(serviceStatus.capacity());

    auto pStatus = serviceStatus.get_as<SERVICE_STATUS_PROCESS>();

    switch (pStatus->dwCurrentState)
    {
        case SERVICE_CONTINUE_PENDING:
            return ServiceStatus::PendingContinue;
        case SERVICE_PAUSE_PENDING:
            return ServiceStatus::PendingPause;
        case SERVICE_PAUSED:
            return ServiceStatus::Paused;
        case SERVICE_RUNNING:
            return ServiceStatus::Started;
        case SERVICE_START_PENDING:
            return ServiceStatus::PendingStart;
        case SERVICE_STOP_PENDING:
            return ServiceStatus::PendingStop;
        case SERVICE_STOPPED:
            return ServiceStatus::Stopped;
        default:
            return ServiceStatus::Inexistent;
    }
}

HRESULT
Orc::DriverMgmt::SetupDriverName(WCHAR* DriverLocation, WCHAR* szDriverFileName, ULONG BufferLength)
{
    HRESULT hr = E_FAIL;
    //
    // Get the current directory.
    //
    if (auto driverLocLen = GetCurrentDirectory(BufferLength, DriverLocation); driverLocLen == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error(L"Failed GetCurrentDirectory (code: {:#x})", hr);
        return hr;
    }

    //
    // Setup path name to driver file.
    //
    if (FAILED(hr = StringCbCat(DriverLocation, BufferLength, L"\\")))
        return hr;

    if (FAILED(hr = StringCbCat(DriverLocation, BufferLength, szDriverFileName)))
        return hr;

    //
    // Insure driver file is in the specified directory.
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    if ((fileHandle = CreateFile(DriverLocation, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))
        == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error(L"'{}' is not present in '{}' (code: {:#x})", szDriverFileName, DriverLocation, hr);
        return hr;
    }

    //
    // Close open file handle.
    if (fileHandle != INVALID_HANDLE_VALUE)
        CloseHandle(fileHandle);

    //
    // Indicate success.
    return S_OK;
}  // SetupDriverName

HRESULT Orc::DriverMgmt::GetDriverBinaryPathName(
    __in SC_HANDLE SchSCManager,
    const WCHAR* DriverName,
    WCHAR* szDriverFileName,
    ULONG BufferLength)
{
    HRESULT hr = E_FAIL;
    SC_HANDLE schService;

    //
    // Open the handle to the existing service.
    schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);
    if (schService == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Log::Error(L"Failed OpenService on '{}' (code: {:#x})", DriverName, hr);
        return hr;
    }

    BOOST_SCOPE_EXIT(&schService)
    {
        if (schService)
        {
            CloseServiceHandle(schService);
            schService = NULL;
        }
    }
    BOOST_SCOPE_EXIT_END;

    //
    // Request the service configuration.
    std::vector<uint8_t> serviceConfig;
    LPQUERY_SERVICE_CONFIG pServiceConfig = NULL;
    DWORD cbBytesNeeded = 0;

    DWORD lastError = ERROR_INSUFFICIENT_BUFFER;
    while (lastError == ERROR_INSUFFICIENT_BUFFER)
    {
        if (cbBytesNeeded)
        {
            serviceConfig.resize(cbBytesNeeded);
            pServiceConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIG>(serviceConfig.data());
        }

        QueryServiceConfig(schService, pServiceConfig, cbBytesNeeded, &cbBytesNeeded);
        lastError = GetLastError();
    }

    if (lastError != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lastError);
        Log::Error(L"Failed QueryServiceConfig on '{}' (code: {:#x})", DriverName, hr);
        return hr;
    }

    wcscpy_s(szDriverFileName, BufferLength, pServiceConfig->lpBinaryPathName);
    return S_OK;
}
