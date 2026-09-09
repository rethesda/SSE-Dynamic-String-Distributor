#include "Hooks.h"
#include "Manager.h"

namespace Hook
{
	struct GetLogEntryHook //QUST CNAM
	{
		static const char* thunk(RE::TESQuestStageItem* item, const RE::TESQuest* ownerQuest)
		{
			auto result = func(item, ownerQuest);

			const char* translation = nullptr;
			if (ownerQuest && item)
			{
				const std::uint32_t uniqueID = item->index + ownerQuest->currentStage;
				translation = Manager::GetSingleton()->getTranslation(ownerQuest->formID, uniqueID, TranslationType::kRuntimeLegacy, result);
			}

			return translation == nullptr ? result : translation;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(24778, 25259) };
			stl::hook_function_prologue<GetLogEntryHook, 6>(target.address());
		}
	};

	struct GetDescription
	{
		static void thunk(RE::TESDescription* description, RE::BSString& out, const RE::TESForm* parent, std::uint32_t chunkID)
		{
			// In SE we hook LoadDescriptionFromFile func where the game already castet the description to TESForm
			// AE inlined, we hook the GetDescription func where the game didn't cast it yet
			const auto safeForm = parent ? parent : skyrim_cast<const RE::TESForm*>(description);

			func(description, out, safeForm, chunkID); // call original func with our cast, so we don't cast twice at least
			if (!safeForm)
				return;

			const char* translation = nullptr;

			// 0x4D414E43 == 'MANC' (CNAM)
			const bool isCNAM = chunkID == 'MANC';
			const bool isDESC = chunkID == 'CSED';
			if (isDESC || isCNAM) // skip garbage data, not caused by Skyrim but other modders
			{
				const auto type = isDESC ? TranslationType::kRuntime1 : TranslationType::kRuntime2;
				translation = Manager::GetSingleton()->getTranslation(safeForm->formID, 0, type);
			}

			if (translation)
			{
				out = translation;
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::VariantID(14401, 14552, 0x1A0300) };
			stl::hook_function_prologue<GetDescription, 6>(target.address());
		}
	};

	struct GetResponseListHook //INFO NAM1
	{
		static RE::TESTopicInfo::TESResponseList* thunk(RE::TESTopicInfo* topicInfo, RE::TESTopicInfo::TESResponseList* list)
		{
			auto result = func(topicInfo, list);
			if (!topicInfo || !result)
				return result;

			auto responseTopicInfo = topicInfo;
			auto linkedResponseInfo = topicInfo->dataInfo;

			if (linkedResponseInfo)
			{
				for (auto i = linkedResponseInfo; i; i = i->dataInfo)
				{
					responseTopicInfo = i;
				}
			}

			const auto manager = Manager::GetSingleton();
			for (auto response = result->head; response; response = response->next)
			{
				if (!response)
					continue;

				SKSE::log::debug("Original string: {} - TopicInfoFormID: {:08X} - LinkedResponseFormID: {:08X} - ResponseNumber: {}", response->responseText.c_str(), topicInfo->formID, responseTopicInfo->formID, response->responseNumber);

				const auto translation = manager->getTranslation(responseTopicInfo->formID, response->responseNumber, TranslationType::kRuntimeIndex);
				if (translation)
				{
					RE::setBSFixedString(response->responseText, translation);
				}
			}

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25083, 25626) };
			stl::hook_function_prologue<GetResponseListHook, 6>(target.address());
		}
	};

	struct DialogueMenuTextHook //DIAL FULL, INFO RNAM
	{
		static void thunk(RE::MenuTopicManager::Dialogue& out, const char* source, std::uint64_t maxLen)
		{
			const auto manager = Manager::GetSingleton();
			const char* translation = nullptr;

			const auto parent = out.parentTopic;
			if (parent)
			{
				translation = manager->getTranslation(parent->formID, 0, TranslationType::kRuntime1);
			}

			const auto parentInfo = out.parentTopicInfo;
			if (parentInfo)
			{
				const auto rnamTranslation = manager->getTranslation(parentInfo->formID, 0, TranslationType::kRuntime2);
				if (rnamTranslation)
				{
					translation = rnamTranslation;
				}
			}

			func(out, translation == nullptr ? source : translation, maxLen);
		};
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			constexpr auto address = RELOCATION_ID(34434, 35254);

			REL::Relocation<std::uintptr_t> target1{ address, REL::Relocate(0xCC, 0x226) };
			stl::write_thunk_call<DialogueMenuTextHook>(target1.address());

			if (REL::Module::IsAE())
			{
				REL::Relocation<std::uintptr_t> target2{ address, 0x115 };
				stl::write_thunk_call<DialogueMenuTextHook>(target2.address());
			}
		}
	};

	struct DataHandlerInitAllForms
	{
		// This can run multiple times, rebuild our stuff here too (since there can be new plugin files)
		static void thunk(RE::TESDataHandler* handler)
		{
			Manager::GetSingleton()->parseTranslationFiles();
			func(handler); // first NPCFullNameCopyComponent calls run here
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			// datahandler compile files
			REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(13645, 13753), REL::Relocate(0x341, 0x363) };
			stl::write_thunk_call<DataHandlerInitAllForms>(target1.address());

			// plugin hot reload
			REL::Relocation<std::uintptr_t> target2{ RELOCATION_ID(13672, 13785), REL::Relocate(0xB05, 0xB8B) };
			stl::write_thunk_call<DataHandlerInitAllForms>(target2.address());
		}
	};

	struct NPCFullNameCopyComponent
	{
		static void thunk(RE::TESFullName* to, RE::BaseFormComponent* from)
		{
			auto fromForm = skyrim_cast<RE::TESForm*>(from);
			if (fromForm)
			{
				// NPCs copy their FullName all the time
				// also reload NPC_ SHRT here, gets copied afterwards
				Manager::GetSingleton()->reloadConstTranslation(fromForm);
			}

			func(to, from); // call original just for compatibility reasons
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(14544, 14716) };
			stl::hook_function_prologue<NPCFullNameCopyComponent, 6>(target.address());
		}
	};

	struct ReconstructForms
	{
		// runs on save load right after finished
		// runs in the big main reset the game performs for example on main menu quit
		static void thunk(RE::BGSConstructFormsInAllFilesMap* data, bool stillLoading)
		{
			func(data, stillLoading);
			if (!data || data->count == 0)
				return;

			const auto mgr = Manager::GetSingleton();
			for (int i = 0; i < 3; i++)
			{
				const auto& list = data->constructedForms.data[i];
				for (const auto& entry : list)
				{
					const auto& form = entry.form;
					if (!form)
						continue;

					mgr->reloadConstTranslation(form);
				}
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			const bool ae2 = REL::Module::get().version() >= SKSE::RUNTIME_SSE_1_7_99;
			REL::Relocation<std::uintptr_t> target1{ REL::VariantID(34644, 35566, 0x581D10), REL::Relocate(0x3AA, ae2 ? 0x265 : 0x25F) };
			stl::write_thunk_call<ReconstructForms>(target1.address());
		}
	};

	struct MainUpdate
	{
		static void thunk()
		{
			func();

			const auto ui = RE::UI::GetSingleton();
			if (!ui || ui->GameIsPaused())
				return;

			const auto datahandler = RE::TESDataHandler::GetSingleton();
			const auto player = RE::PlayerCharacter::GetSingleton();
			if (!player || !datahandler)
				return;

			const auto callback = [](RE::TESObjectREFR* ref, [[maybe_unused]] std::uint64_t data) -> bool
				{
					if (ref)
					{
						const auto base = ref->GetBaseObject();
						const std::string baseMessage = base ? std::format("{:08X} - {}", base->GetFormID(), RE::FormTypeToString(base->GetFormType())) : "Unknown";
						SKSE::log::debug("Found Reference {:08X} - {} with base object {}", ref->GetFormID(), RE::FormTypeToString(ref->GetFormType()), baseMessage);
					}

					return false; // exit on true, continue on false
				};

			constexpr float radius = 10000.0f;
			RE::enumReferencesCloseToRef(datahandler, player, radius, player->data.location, radius, callback, 0);
		};
		static inline REL::Relocation<decltype(thunk)> func;

		static void Install()
		{
			const auto runtime = REL::Module::get().version();

			// just kill me at this point
			int AEOffset = 0xC26;
			if (runtime >= SKSE::RUNTIME_SSE_1_7_99)
			{
				AEOffset = 0xC3D;
			}
			else if (runtime >= SKSE::RUNTIME_SSE_1_6_1130)
			{
				AEOffset = 0xC2B;
			}

			REL::Relocation<std::uintptr_t> target1{ RELOCATION_ID(35565, 36564), REL::Relocate(0x748, AEOffset, 0x7EE) };
			stl::write_thunk_call<MainUpdate>(target1.address());
		}
	};

	void InstallHooks()
	{
		DataHandlerInitAllForms::Install();
		NPCFullNameCopyComponent::Install();
		GetDescription::Install();
		GetLogEntryHook::Install();
		GetResponseListHook::Install();
		DialogueMenuTextHook::Install();
		ReconstructForms::Install();
		MainUpdate::Install();

		SKSE::log::info("{} Done!", __FUNCTION__);
	}
}