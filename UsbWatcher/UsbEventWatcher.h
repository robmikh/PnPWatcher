#pragma once
#include "UsbEvent.h"

class UsbEventWatcher
{
public:
	UsbEventWatcher(winrt::Windows::System::DispatcherQueue const& dispatcherQueue, std::function<void(UsbEvent)> callback);
	~UsbEventWatcher();

private:
	winrt::com_ptr<IWbemServices> m_services;
	winrt::com_ptr<IUnsecuredApartment> m_unsecuredApartment;
	winrt::com_ptr<EventSink> m_sink;
	winrt::com_ptr<IWbemObjectSink> m_sinkStub;

	std::function<void(UsbEvent)> m_callback;
	winrt::Windows::System::DispatcherQueue m_dispatcherQueue{ nullptr };
};