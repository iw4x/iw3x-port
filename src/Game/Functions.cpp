#include "STDInclude.hpp"
#include "Structs.IW4.hpp"

namespace Game
{
	// C-Style casts are fine here, that's where we're doing our dirty stuff anyways...
	Cbuf_AddText_t Cbuf_AddText = Cbuf_AddText_t(0x4F8D90);

	Cmd_ExecuteSingleCommand_t Cmd_ExecuteSingleCommand = Cmd_ExecuteSingleCommand_t(0x4F9AB0);

	Com_Error_t Com_Error = Com_Error_t(0x4FD330);
	Com_PrintMessage_t Com_PrintMessage = Com_PrintMessage_t(0x4FCA50);
	
	DB_IsXAssetDefault_t DB_IsXAssetDefault = DB_IsXAssetDefault_t(0x4898A0);
	DB_FindXAssetHeader_t DB_FindXAssetHeader = DB_FindXAssetHeader_t(0x489570);
	DB_EnumXAssets_FastFile_t DB_EnumXAssets_FastFile = DB_EnumXAssets_FastFile_t(0x489120);
	DB_GetXAssetNameHandler_t* DB_GetXAssetNameHandlers = reinterpret_cast<DB_GetXAssetNameHandler_t*>(0x7268D0);
    DB_LoadXAssets_t DB_LoadXAssets = DB_LoadXAssets_t(0x48A2B0);
	DB_GetXAssetSizeHandler_t* DB_GetXAssetSizeHandlers = reinterpret_cast<DB_GetXAssetSizeHandler_t*>(0x726A10);

	Dvar_RegisterNew_t* Dvar_RegisterNew = reinterpret_cast<Dvar_RegisterNew_t*>(0x56C130);

	IW3::XAssetHeader* DB_XAssetPool = reinterpret_cast<IW3::XAssetHeader*>(0x7265E0);
	unsigned int* g_poolSize = reinterpret_cast<unsigned int*>(0x7263A0);

	FS_Read_t FS_Read = FS_Read_t(0x55C120);
	FS_FOpenFileRead_t FS_FOpenFileRead = FS_FOpenFileRead_t(0x55C050);
	Image_LoadFromFileWithReader_t Image_LoadFromFileWithReader = Image_LoadFromFileWithReader_t(0x642380);
	FS_FOpenFileReadDatabase_t FS_FOpenFileReadDatabase = FS_FOpenFileReadDatabase_t(0x55C030);
	SEH_LocalizeTextMessage_t SEH_LocalizeTextMessage = SEH_LocalizeTextMessage_t(0x538D30);
	SL_GetStringOfSize_t SL_GetStringOfSize = SL_GetStringOfSize_t(0x518290);

	DWORD* cmd_id = reinterpret_cast<DWORD*>(0x1410B40);
	DWORD* cmd_argc = reinterpret_cast<DWORD*>(0x1410B84);
	char*** cmd_argv = reinterpret_cast<char***>(0x1410BA4);
	cmd_function_s** cmd_ptr = reinterpret_cast<cmd_function_s**>(0x1410B3C);

	IW3::XZone* g_zones = reinterpret_cast<IW3::XZone*>(0xFFEFD0);
	IW3::XAssetEntryPoolEntry* g_assetEntryPool = reinterpret_cast<IW3::XAssetEntryPoolEntry*>(0xF0D640);
	unsigned short* db_hashTable = reinterpret_cast<unsigned short*>(0xE62A80);
	IW3::sharedUiInfo_t* sharedUiInfo = reinterpret_cast<IW3::sharedUiInfo_t*>(0x0CAF06D0);

	IW3::infoParm_t* infoParams = reinterpret_cast<Game::IW3::infoParm_t*>(0x71FBD0); // Count 0x1C

	IW3::cgMedia_t* cgMedia = reinterpret_cast<Game::IW3::cgMedia_t*>(0x84CB30);

	void DB_EnumXAssetEntries(IW3::XAssetType type, std::function<void(IW3::XAssetEntryPoolEntry*)> callback, bool overrides)
	{
		volatile long* lock = reinterpret_cast<volatile long*>(0x10D01A4);
		InterlockedIncrement(lock);
		
		while (*reinterpret_cast<volatile long*>(0x10D01A8)) std::this_thread::sleep_for(1ms);

		unsigned int index = 0;
		do
		{
			unsigned short hashIndex = db_hashTable[index];
			if (hashIndex)
			{
				do
				{
					IW3::XAssetEntryPoolEntry* asset = &g_assetEntryPool[hashIndex];
					hashIndex = asset->entry.nextHash;
					if (asset->entry.asset.type == type)
					{
						callback(asset);
						if (overrides)
						{
							unsigned short overrideIndex = asset->entry.nextOverride;
							if (asset->entry.nextOverride)
							{
								do
								{
									asset = &g_assetEntryPool[overrideIndex];
									callback(asset);
									overrideIndex = asset->entry.nextOverride;
								} while (overrideIndex);
							}
						}
					}
				} while (hashIndex);
			}
			++index;
		} while (index < 32768); /// size of db_hashtable is 32768 (65536 bytes)
		InterlockedDecrement(lock);
	}

	int DB_HashForName(Game::IW3::XAssetType type, const char* name)
	{
		int result = static_cast<int>(type);
		int v4; // eax

		while (1)
		{
			while (1)
			{
				v4 = tolower(*name);
				if (v4 != 92)
				{
					break;
				}
				result = 31 * result + 47;
				++name;
			}
			if (!v4)
			{
				break;
			}
			result = v4 + 31 * result;
			++name;
		}

		return result & 0x7FFF;
	}

	Game::IW3::XAssetEntryPoolEntry* DB_FindXAssetEntry(Game::IW3::XAssetType type, const char* name)
	{
		Game::IW3::XAssetEntryPoolEntry* entry = nullptr;
		auto hash = db_hashTable[DB_HashForName(type, name)];
		if (!hash)
		{
			return 0;
		}
		while (1)
		{
			entry = &g_assetEntryPool[hash];
			if (entry->entry.asset.type == type)
			{
				auto assetName = DB_GetXAssetNameHandlers[entry->entry.asset.type](&entry->entry.asset.header);
				if (!_stricmp(name, assetName))
					break;
			}

			hash = entry->entry.nextHash;
			if (!entry->entry.nextHash)
				return 0;
		}

		return entry;
	}

	int FS_ReadFile(const char* path, char** buffer)
	{
		__asm
		{
			push ecx

			push buffer
			mov eax, path
			mov ecx, 55C440h
			call ecx

			add esp, 4h
			pop ecx
		}
	}

	void FS_FreeFile(void* buffer)
	{
		__asm
		{
			push esi

			mov esi, buffer
			mov eax, 564240h
			call eax

			pop esi
		}
	}

	void FS_FCloseFile(int handle)
	{
		__asm
		{
			push ecx

			mov eax, handle
			mov ecx, 55B3B0h
			call ecx

			pop ecx
		}
	}

	void Cmd_AddCommand(const char* name, void(*callback)(), cmd_function_s* data, char)
	{
		data->name = name;
		data->function = callback;
		data->next = *cmd_ptr;
		*cmd_ptr = data;
	}

	unsigned int R_HashString(const char* string)
	{
		unsigned int hash = 0;

		while (*string)
		{
			hash = (*string | 0x20) ^ (33 * hash);
			string++;
		}

		return hash;
	}

	const char* FindHash(unsigned int hash)
	{
		static const char* textureMaps[]
		{
			"colorMap",
			"colorDetailMap",
			"normalMap",
			"specularMap",
			"occlusionMap",
			"detailMap",
			"waterMap",

			"colorMap00",
			"colorMap01",
			"colorMap02",
			"colorMap03",
			"colorMap04",
			"colorMap05",
			"colorMap06",
			"colorMap07",
			"colorMap08",
			"colorMap09",
			"colorMap10",
			"colorMap11",
			"colorMap12",
			"colorMap13",
			"colorMap14",
			"colorMap15", // T6's occlusion map?

			"Detail_Map",
			"Normal_Map",
			"Diffuse_MapSampler",
			"RadiantDiffuse",
			nullptr
		};

		const char** ptr = textureMaps;

		while (*ptr)
		{
			if (hash == R_HashString(*ptr))
			{
				return *ptr;
			}
			
			ptr++;
		}

		return "error";
	}

	const char* SL_ConvertToString(int idx)
	{
		struct stringList
		{
			int unk;
			char string[1];
			int unk2;
		};

		return (*reinterpret_cast<stringList**>(0x14E8A04))[idx & 0xFFFF].string;
	}

	__declspec(naked) dvar_s* Dvar_FindVar(const char* /*dvar*/)
	{
		__asm
		{
			push eax
			pushad

			mov edi, [esp + 28h]
			mov eax, 56B5D0h
			call eax

			mov [esp + 20h], eax
			popad

			pop eax
			retn
		}
	}

	__declspec(naked) dvar_s* Dvar_RegisterVariant(const char* /*dvar*/)
	{
		__asm
		{
			push eax
			pushad

			mov edi, [esp + 28h]
			mov eax, 56B5D0h
			call eax

			mov[esp + 20h], eax
			popad

			pop eax
			retn
		}
	}

	__declspec(naked) Game::IW3::PackedUnitVec Vec3PackUnitVec(vec3_t *)
	{
		__asm
		{
			mov eax, [esp + 4h]
			push 5645A0h
			retn
		}
	}

	__declspec(naked) const char* UI_SafeTranslateString(const char*)
	{
		__asm
		{
			mov eax, [esp + 4h]
			push 54ACA0h
			retn
		}
	}

	__declspec(naked) void Vec3UnpackUnitVec(Game::IW3::PackedUnitVec, vec3_t*)
	{
		__asm
		{
			push ecx
			mov ecx, [esp + 0Ch]
			push [esp + 08h]

			mov eax, 5647D0h
			call eax

			add esp, 4h
			pop ecx
			retn
		}
	}

	__declspec(naked) void AxisToAngles(vec3_t*, vec3_t*)
	{
		__asm
		{
			pushad
			mov ecx, [esp + 0x8 + 0x20]
			push ecx
			mov eax, [esp + 0x8 + 0x20]
			push eax

			mov ebx, 0x561B50
			call ebx
			
			pop eax
			pop ecx
			popad

			retn

		}
	}


	// Bounds in IW3, as stored as two coordinates
	// e.g. Bottom-Left and Top-Right
	// (example is in R2, but works the same for R3):
	//
	// This one -> +-------+
	//             |       |
	//             +-------+ <- And this one
	//
	// In IW4 however, bounds are stored as a center point
	// and the distance to one of the corner points:
	//
	//             +-------+
	//             |   + <-|- That point + the distance to one
	//             +-------+  of the other points as vector
	//
	// By adding the vector to the point, we get a corner,
	// by subtracting it, we get the opposite corner.
	// That way the calculation can be done just like in IW3.
	void ConvertBounds(Game::IW4::Bounds* bounds, vec3_t mins, vec3_t maxs)
	{
		for (int i = 0; i < 3; ++i)
		{
			bounds->halfSize[i] = (maxs[i] - mins[i]) / 2;
			bounds->midPoint[i] = bounds->halfSize[i] + mins[i];
		}
	}

	namespace IW4
	{
		void VectorSubtract(const vec3_t& a, const vec3_t& b, vec3_t& out)
		{
			out[0] = a[0] - b[0];
			out[1] = a[1] - b[1];
			out[2] = a[2] - b[2];
		}

		void Bounds::compute(vec3_t mins, vec3_t maxs)
		{
			ConvertBounds(this, mins, maxs);
		}

		void Bounds::min(vec3_t &out)
		{
			for (int i = 0; i < 3; ++i)
			{
				out[i] = midPoint[i] - halfSize[i];
			}
		}

		bool Bounds::overlaps(const Bounds& other)
		{
			vec3_t points[6]{};

			VectorSubtract(other.midPoint, other.halfSize, points[0]);
			VectorSubtract(other.midPoint, { -other.halfSize[0], other.halfSize[1], other.halfSize[2] }, points[1]);
			VectorSubtract(other.midPoint, { -other.halfSize[0], -other.halfSize[1], other.halfSize[2] }, points[2]);
			VectorSubtract(other.midPoint, { -other.halfSize[0], -other.halfSize[1], -other.halfSize[2] }, points[3]);
			VectorSubtract(other.midPoint, { other.halfSize[0], -other.halfSize[1], -other.halfSize[2] }, points[4]);
			VectorSubtract(other.midPoint, { -other.halfSize[0], other.halfSize[1], -other.halfSize[2] }, points[5]);

			for (size_t i = 0; i < ARRAYSIZE(points); i++)
			{
				if (contains(points[i]))
				{
					return true;
				}
			}

			return false;
		}

		bool Bounds::contains(const vec3_t& point)
		{
			vec3_t min{};
			vec3_t max{};
			this->min(min);
			this->max(max);

			return
				point[0] >= min[0] && point[0] <= max[0] &&
				point[1] >= min[1] && point[1] <= max[1] &&
				point[2] >= min[2] && point[2] <= max[2];
		}

		void Bounds::max(vec3_t& out)
		{
			for (int i = 0; i < 3; ++i)
			{
				out[i] = midPoint[i] + halfSize[i];
			}
		}
	}
}
