#pragma once

enum class UsbEventType
{
	Added,
	Removed,
};

struct UsbEvent
{
	UsbEventType Type;
	std::wstring Name;
	std::wstring Description;
	std::wstring DeviceId;
	winrt::Windows::Foundation::DateTime Timestamp;
};

enum class UsbEventColumn
{
	Type,
	Name,
	Description,
	DeviceId,
	Timestamp,
};

inline std::wostream& operator<< (std::wostream& os, UsbEventType const& type)
{
	switch (type)
	{
	case UsbEventType::Added:
		os << L"Added";
		break;
	case UsbEventType::Removed:
		os << L"Removed";
		break;
	}
	return os;
}

inline std::wostream& operator<< (std::wostream& os, UsbEventColumn const& column)
{
	switch (column)
	{
	case UsbEventColumn::Type:
		os <<  L"Event Type";
		break;
	case UsbEventColumn::Name:
		os << L"Name";
		break;
	case UsbEventColumn::Description:
		os << L"Description";
		break;
	case UsbEventColumn::DeviceId:
		os << L"DeviceId";
		break;
	case UsbEventColumn::Timestamp:
		os << L"Timestamp";
		break;
	}
	return os;
}
