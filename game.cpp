#include "game.h"
#include "../math/math.h"
#include "../window/window.hpp"
#include "game.h"

std::vector<std::string> processlist = { "IRFive_GameProcess.exe" };

memify mem(processlist);

void PatternToBytes(const std::string& pattern, std::vector<uint8_t>& bytes, std::string& mask)
{
	bytes.clear();
	mask.clear();
	std::istringstream stream(pattern);
	std::string byte_str;

	while (stream >> byte_str)
	{
		if (byte_str == "??" || byte_str == "?")
		{
			bytes.push_back(0x00);
			mask.push_back('?');
		}
		else
		{
			bytes.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
			mask.push_back('x');
		}
	}
}

const std::string world_sig = "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74 52 8B 81";
const std::string replay_sig = "48 8B 05 ? ? ? ? 66 89 0D ? ? ? ? 4C 89 2C D0";
const std::string viewport_sig = "48 8B 15 ? ? ? ? 48 8D 2D ? ? ? ? 48 8B CD";
const std::string camera_sig = "48 8B 05 ? ? ? ? 48 8B 98 ? ? ? ? EB";

uintptr_t FindPattern(HANDLE process, uintptr_t start, uintptr_t end, const std::string& pattern, const std::string& sig_name)
{
	std::vector<uint8_t> pattern_bytes;
	std::string mask;
	PatternToBytes(pattern, pattern_bytes, mask);

	MEMORY_BASIC_INFORMATION mbi;
	std::vector<uint8_t> buffer;
	SIZE_T bytes_read;

	for (uintptr_t current = start; current < end;)
	{
		if (!VirtualQueryEx(process, (LPCVOID)current, &mbi, sizeof(mbi)))
		{
			current += 0x1000;
			continue;
		}

		if (mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS || (mbi.Protect & PAGE_GUARD))
		{
			current += mbi.RegionSize;
			continue;
		}

		buffer.resize(mbi.RegionSize);
		if (!ReadProcessMemory(process, (LPCVOID)current, buffer.data(), mbi.RegionSize, &bytes_read))
		{
			current += mbi.RegionSize;
			continue;
		}

		for (size_t i = 0; i <= bytes_read - pattern_bytes.size(); ++i)
		{
			bool found = true;
			for (size_t j = 0; j < pattern_bytes.size(); ++j)
			{
				if (mask[j] == 'x' && buffer[i + j] != pattern_bytes[j])
				{
					found = false;
					break;
				}
			}

			if (found) {
				uintptr_t match_addr = current + i;

				if (pattern_bytes[0] == 0x48 && (pattern_bytes[2] == 0x05 || pattern_bytes[2] == 0x15))
				{
					int32_t offset;
					if (!ReadProcessMemory(process, (LPCVOID)(match_addr + 3), &offset, sizeof(offset), &bytes_read))
						return 0;

					return match_addr + offset + 7;
				}

				return match_addr;
			}
		}

		current += mbi.RegionSize;
	}

	return 0;
}

uintptr_t GetProcessId(std::string_view processName)
{
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	HANDLE ss = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(ss, &pe)) {
		if (!processName.compare(pe.szExeFile)) {
			return pe.th32ProcessID;
		}
	}
}

namespace FiveM {
	namespace offset {
		uintptr_t world = 0;
		uintptr_t replay = 0;
		uintptr_t viewport = 0;
		uintptr_t localPlayer = 0;
		uintptr_t boneList = 0;
		uintptr_t boneMatrix = 0x60;
		uintptr_t playerInfo = 0;
		uintptr_t playerHealth = 0x280;
		uintptr_t playerPosition = 0x90;
		uintptr_t base = 0;
	}
}

void FiveM::Setup()
{
	auto process_name = mem.GetProcessName();
	using namespace offset;

	auto game_base = mem.GetBase(process_name);

	HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessId(process_name));
	if (!process_handle)
	{
		std::cout << "[>>] Failed to open process. " << std::endl;
		exit(0);
	}

	MODULEINFO mod_info;
	if (!GetModuleInformation(process_handle, (HMODULE)game_base, &mod_info, sizeof(MODULEINFO)))
	{
		std::cout << "[>>] Failed to get module information. " << std::endl;
		CloseHandle(process_handle);
		exit(0);
	}

	uintptr_t scan_start = game_base;
	uintptr_t scan_end = game_base + mod_info.SizeOfImage;

	world = FindPattern(process_handle, scan_start, scan_end, world_sig, "world");
	world = mem.Read<uintptr_t>(world);
	std::cout << "World: 0x" << std::hex << world << std::endl;

	replay = FindPattern(process_handle, scan_start, scan_end, replay_sig, "replay");
	replay = mem.Read<uintptr_t>(replay);
	std::cout << "replay: 0x" << std::hex << replay << std::endl;

	viewport = FindPattern(process_handle, scan_start, scan_end, viewport_sig, "viewport");
	viewport = mem.Read<uintptr_t>(viewport);
	std::cout << "viewport: 0x" << std::hex << viewport << std::endl;

	localPlayer = mem.Read<uintptr_t>(world + 0x8);

	CloseHandle(process_handle);

}

void FiveM::ESP::RunESP()
{
	uintptr_t ped_replay_interface = mem.Read<uintptr_t>(offset::replay + 0x18);
	if (!ped_replay_interface)
		return;

	uintptr_t pedList = mem.Read<uintptr_t>(ped_replay_interface + 0x100);
	if (!pedList)
		return;

	auto view_matrix = mem.Read<Matrix>(offset::viewport + 0x24C);
	float playerHeight = 1.50;

	for (int i = 0; i < 200; i++) {

		uintptr_t ped = mem.Read<uintptr_t>(pedList + i * 0x10);
		if (!ped) continue;

		if (ped == offset::localPlayer) continue;

		int health = mem.Read<int>(ped + offset::playerHealth);
		if (health < 1.0f) continue;

		health = std::clamp(health, 0, 100);

		uintptr_t player_info = mem.Read<uintptr_t>(ped + offset::playerInfo);
		if (!player_info) continue;

		Vec3 origin = mem.Read<Vec3>(ped + offset::playerPosition); // وسط بدن
		Vec3 headPos = origin;
		headPos.z += 0.9f; // سر بازیکن (نسبی هست)

		Vec3 feetPos = origin;
		feetPos.z -= 0.9f;
		Vec2 screenHead, screenFeet;
		if (!headPos.world_to_screen(view_matrix, screenHead)) continue;
		if (!feetPos.world_to_screen(view_matrix, screenFeet)) continue;


		Vec2 org;
		if (origin.world_to_screen(view_matrix, org)) {

			float boxHeight = screenFeet.y - screenHead.y;
			float boxWidth = boxHeight * 0.45f;

			ImVec2 p1 = ImVec2(screenHead.x - boxWidth * 0.5f, screenHead.y);
			ImVec2 p2 = ImVec2(screenHead.x + boxWidth * 0.5f, screenFeet.y);

			ImU32 fillColor = IM_COL32(128, 128, 128, 100);
			ImU32 outlineColor = IM_COL32(255, 255, 255, 255);

			auto drawList = ImGui::GetBackgroundDrawList();
			drawList->AddRectFilled(p1, p2, fillColor);
			drawList->AddRect(p1, p2, outlineColor);


			float healthPerc = health / 100.0f;
			float barWidth = 5.0f;
			float barHeight = boxHeight * healthPerc;
			float barX = p1.x - barWidth - 2.0f;
			float barTopY = p1.y;
			float barBotY = p2.y;
			float barFillY = barBotY - barHeight;

			drawList->AddRectFilled(
				ImVec2(barX, barTopY),
				ImVec2(barX + barWidth, barBotY),
				IM_COL32(0, 0, 0, 150)
			);

			ImU32 healthColor = ImColor::HSV(healthPerc * 0.33f, 1.0f, 1.0f);
			drawList->AddRectFilled(
				ImVec2(barX, barFillY),
				ImVec2(barX + barWidth, barBotY),
				healthColor
			);

			char hpText[8];
			snprintf(hpText, sizeof(hpText), "%d", static_cast<int>(health));
			ImVec2 textSize = ImGui::CalcTextSize(hpText);

			float textX = barX + (barWidth * 0.5f) - (textSize.x * 0.5f);
			float textY = barBotY + 2.0f;

			drawList->AddText(
				ImVec2(textX, textY),
				IM_COL32(255, 255, 255, 255),
				hpText
			);
		}
	}
}


