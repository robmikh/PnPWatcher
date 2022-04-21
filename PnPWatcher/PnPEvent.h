#pragma once

enum class PnPEventType
{
	Added,
	Removed,
};

struct PnPEvent
{
	PnPEventType Type;
	std::wstring Name;
	std::wstring Description;
	std::wstring DeviceId;
	winrt::Windows::Foundation::DateTime Timestamp;
};

enum class PnPEventColumn
{
	Type,
	Name,
	Description,
	DeviceId,
	Timestamp,
};

inline std::wostream& operator<< (std::wostream& os, PnPEventType const& type)
{
	switch (type)
	{
	case PnPEventType::Added:
		os << L"Added";
		break;
	case PnPEventType::Removed:
		os << L"Removed";
		break;
	}
	return os;
}

inline std::wostream& operator<< (std::wostream& os, PnPEventColumn const& column)
{
	switch (column)
	{
	case PnPEventColumn::Type:
		os <<  L"Event Type";
		break;
	case PnPEventColumn::Name:
		os << L"Name";
		break;
	case PnPEventColumn::Description:
		os << L"Description";
		break;
	case PnPEventColumn::DeviceId:
		os << L"DeviceId";
		break;
	case PnPEventColumn::Timestamp:
		os << L"Timestamp";
		break;
	}
	return os;
}
