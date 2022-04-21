#pragma once
#include "PnPEvent.h"

class PnPEventWatcher
{
public:
	PnPEventWatcher(winrt::Windows::System::DispatcherQueue const& dispatcherQueue, std::function<void(PnPEvent)> callback);
	~PnPEventWatcher();

private:
	winrt::com_ptr<IWbemServices> m_services;
	winrt::com_ptr<IUnsecuredApartment> m_unsecuredApartment;
	winrt::com_ptr<EventSink> m_sink;
	winrt::com_ptr<IWbemObjectSink> m_sinkStub;

	std::function<void(PnPEvent)> m_callback;
	winrt::Windows::System::DispatcherQueue m_dispatcherQueue{ nullptr };
};