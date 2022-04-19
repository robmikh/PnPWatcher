#include "pch.h"
#include "UsbEventWatcher.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::System;
}

UsbEventWatcher::UsbEventWatcher(winrt::DispatcherQueue const& dispatcherQueue, std::function<void(UsbEvent)> callback)
{
    m_dispatcherQueue = dispatcherQueue;
    m_callback = callback;

    auto locator = winrt::create_instance<IWbemLocator>(CLSID_WbemLocator);
    winrt::check_hresult(locator->ConnectServer(BSTR(L"ROOT\\CIMV2"), nullptr, nullptr, 0, 0, 0, 0, m_services.put()));

    winrt::check_hresult(CoSetProxyBlanket(
        m_services.get(),
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE));

    m_unsecuredApartment = winrt::create_instance<IUnsecuredApartment>(CLSID_UnsecuredApartment, CLSCTX_LOCAL_SERVER);
    m_sink = winrt::make_self<EventSink>(EventSink::EventSinkCallback([&](winrt::array_view<IWbemClassObject*> const& objs)
        {
            for (auto& objRaw : objs)
            {
                winrt::com_ptr<IWbemClassObject> obj;
                obj.copy_from(objRaw);

                auto classNameBstr = GetProperty<wil::unique_bstr>(obj, L"__CLASS");
                auto className = std::wstring(classNameBstr.get(), SysStringLen(classNameBstr.get()));
                std::optional<UsbEventType> usbEventType = std::nullopt;
                if (className == L"__InstanceCreationEvent")
                {
                    usbEventType = std::optional(UsbEventType::Added);
                }
                else if (className == L"__InstanceDeletionEvent")
                {
                    usbEventType = std::optional(UsbEventType::Removed);
                }

                if (usbEventType.has_value())
                {
                    // The docs (https://docs.microsoft.com/en-us/windows/win32/wmisdk/--instancecreationevent)
                    // claim that this property is a uint64... but the variant we get back is a BSTR. To make
                    // things even stranger, if we interpret the value as a uint64 the value isn't nearly high
                    // enough. But if we interpret it as a BSTR and parse the BSTR to a uint64... the value is
                    // correct. Not sure why this is the case...
                    //auto eventTime = GetProperty<uint64_t>(obj, L"TIME_CREATED");
                    auto eventTimeString = GetProperty<std::wstring>(obj, L"TIME_CREATED");
                    auto eventTime = std::stoull(eventTimeString);

                    auto targetInstance = GetProperty<winrt::com_ptr<IUnknown>>(obj, L"TargetInstance");
                    auto usbDevice = targetInstance.as<IWbemClassObject>();

                    auto type = usbEventType.value();
                    auto name = GetProperty<std::wstring>(usbDevice, L"Name");
                    auto description = GetProperty<std::wstring>(usbDevice, L"Description");
                    auto deviceId = GetProperty<std::wstring>(usbDevice, L"DeviceId");
                    auto fileTime = winrt::file_time(eventTime);
                    auto timestamp = winrt::clock::from_file_time(fileTime);

                    auto callback = m_callback;
                    UsbEvent usbEvent =
                    {
                        type,
                        name,
                        description,
                        deviceId,
                        timestamp,
                    };
                    m_dispatcherQueue.TryEnqueue([usbEvent, callback]()
                        {
                            callback(std::move(usbEvent));
                        });
                }
            }
        }));

    winrt::com_ptr<IUnknown> stubUnknown;
    winrt::check_hresult(m_unsecuredApartment->CreateObjectStub(m_sink.get(), stubUnknown.put()));
    m_sinkStub = stubUnknown.as<IWbemObjectSink>();
    winrt::check_hresult(m_services->ExecNotificationQueryAsync(
        BSTR(L"WQL"),
        BSTR(L"SELECT * FROM __InstanceOperationEvent WITHIN 2 WHERE TargetInstance ISA 'Win32_USBHub'"),
        WBEM_FLAG_SEND_STATUS,
        nullptr,
        m_sinkStub.get()));
}

UsbEventWatcher::~UsbEventWatcher()
{
    winrt::check_hresult(m_services->CancelAsyncCall(m_sinkStub.get()));
}
