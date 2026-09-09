#pragma once

namespace RE
{
	[[nodiscard]] inline static RE::BSTHashMap<RE::FormID, RE::BSFixedString>& getActivateTextOverrideMap() // ACTI RNAM, FLOR RNAM
	{
		REL::Relocation<RE::BSTHashMap<RE::FormID, RE::BSFixedString>*> map{ REL::VariantID(501445, 360165, 0x1E71EA0), -0x18 }; // ptr to sentinal - offset to start of hashmap
		return *map;
	}

	RE::Setting* setStringValue(RE::Setting* setting, const char* str);
	// case sensitive BSFixedString change
	void setBSFixedString(RE::BSFixedString& str, const char* data);
	void enumReferencesCloseToRef(RE::TESDataHandler* handler, RE::TESObjectREFR* ref, float radius1, const RE::NiPoint3& point2, float radius2, bool(*enumFunc)(RE::TESObjectREFR*, std::uint64_t), std::uint64_t enumData);

	RE::OBJECT_TYPE stringToObjectType(std::string_view str);
}