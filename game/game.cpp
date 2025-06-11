#include "game.h"
#include "../window/window.hpp"
#include "game.h"
#include "../game/utilities/ConfigMenu.hpp"
#include <Windows.h>
#include <iostream>

std::vector<std::string> processlist = { "IRFive_b3095_GameProcess.exe" };

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
// const std::string bones_sig		=	"48 89 5C ?24 ? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 60 48 8B 01 41 8B E8 48 8B F2 48 8B F9 33 DB";

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
        uintptr_t localPlayer = 0x8;
        uintptr_t boneList = 0x430;
        uintptr_t boneMatrix = 0x60;
        uintptr_t playerInfo = 0x10A8;
        uintptr_t playerHealth = 0x280;
        uintptr_t playerPosition = 0x90;
        uintptr_t base = 0;
    }
}
Vec3 get_bone_position(uintptr_t ped, boneID bone_id) {
    Matrix bone_matrix = mem.Read<Matrix>(ped + FiveM::offset::boneMatrix);
    uintptr_t bone_list = ped + 0x430;
    Vector3 bone_pos = mem.Read<Vector3>(bone_list + (0x28 * static_cast<int>(bone_id)));
    return Vec3(
        bone_pos.x * bone_matrix.m[0][0] + bone_pos.y * bone_matrix.m[1][0] + bone_pos.z * bone_matrix.m[2][0] + bone_matrix.m[3][0],
        bone_pos.x * bone_matrix.m[0][1] + bone_pos.y * bone_matrix.m[1][1] + bone_pos.z * bone_matrix.m[2][1] + bone_matrix.m[3][1],
        bone_pos.x * bone_matrix.m[0][2] + bone_pos.y * bone_matrix.m[1][2] + bone_pos.z * bone_matrix.m[2][2] + bone_matrix.m[3][2]
    );
}
void FiveM::Setup()
{
    ConfigMenu::ResetToDefaults();

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

    // bones = FindPattern(process_handle, scan_start, scan_end, bones_sig, "bones");
    // bones = mem.Read<uintptr_t>(bones);
    // std::cout << "bonesPos: 0x" << std::hex << bones << std::endl;

    localPlayer = mem.Read<uintptr_t>(world + 0x8);

    CloseHandle(process_handle);

}

struct Bone
{
    float matrix[3][4];  // GTA V/FiveM często używa 3x4
};

void draw_valid_bones(uintptr_t ped, const Matrix& view_matrix) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    if (!draw_list) return;

    const std::vector<boneID> all_bones = {
        boneID::SKEL_Head, boneID::SKEL_Neck_1, boneID::SKEL_Spine3,
        boneID::SKEL_L_UpperArm, boneID::SKEL_R_UpperArm,
        boneID::SKEL_L_Hand, boneID::SKEL_R_Hand,
        boneID::SKEL_L_Thigh, boneID::SKEL_R_Thigh,
        boneID::SKEL_L_Foot, boneID::SKEL_R_Foot
    };

    for (const auto& bone : all_bones) {
        Vec3 bone_pos = get_bone_position(ped, bone);
        Vec2 screen_pos;

        if (bone_pos.world_to_screen(view_matrix, screen_pos)) {
            draw_list->AddCircleFilled(
                ImVec2(screen_pos.x, screen_pos.y),
                3.0f,
                IM_COL32(0, 255, 0, 255)  // سبز برای استخوان‌های معتبر
            );
        }
    }
}
// این تابع تمام نقاط ESP را جمع‌آوری می‌کند (سر، استخوان‌ها، یا هر نقطه دیگر)
std::vector<ImVec2> GetAllESPPoints() {
    std::vector<ImVec2> espPoints;
    auto view_matrix = mem.Read<Matrix>(FiveM::offset::viewport + 0x24C);

    // مثال: نقاط سر بازیکنان
    uintptr_t pedList = mem.Read<uintptr_t>(FiveM::offset::replay + 0x18 + 0x100);
    for (int i = 0; i < 200; i++) {
        uintptr_t ped = mem.Read<uintptr_t>(pedList + i * 0x10);
        if (!ped) continue;

        // نقاط سر
        Vec3 headPos = mem.Read<Vec3>(ped + FiveM::offset::playerPosition);
        headPos.z += 0.9f;
        Vec2 screenPos;
        if (headPos.world_to_screen(view_matrix, screenPos)) {
            espPoints.push_back(ImVec2(screenPos.x, screenPos.y));
        }

        // نقاط استخوان‌ها (اسکلت)
        if (ConfigMenu::showBones) {
            uintptr_t boneList = mem.Read<uintptr_t>(ped + FiveM::offset::boneList);
            if (boneList) {
                const std::vector<boneID> bones = {
                    boneID::SKEL_Head, boneID::SKEL_Neck_1, boneID::SKEL_Spine3,
                    boneID::SKEL_L_Hand, boneID::SKEL_R_Hand
                };
                for (auto bone : bones) {
                    Vec3 bonePos = get_bone_position(ped, bone);
                    Vec2 boneScreenPos;
                    if (bonePos.world_to_screen(view_matrix, boneScreenPos)) {
                        espPoints.push_back(ImVec2(boneScreenPos.x, boneScreenPos.y));
                    }
                }
            }
        }
    }
    return espPoints;
}

// تابع اصلی Lock-On (همیشه فعال)



Vec3 readBonePos(uintptr_t boneList, boneID id) {
    uintptr_t bonePtr = mem.Read<uintptr_t>(boneList + static_cast<uintptr_t>(id));
    return bonePtr ? mem.Read<Vec3>(bonePtr + FiveM::offset::boneMatrix) : Vec3{ 0, 0, 0 };
}

uintptr_t GetBoneAddress(uintptr_t boneList, boneID id)
{
    // Zakładamy, że 'id' jest już offsetem/indeksem do kości
    return mem.Read<uintptr_t>(boneList + static_cast<uintptr_t>(id));
}
void draw_skeleton(uintptr_t ped, const Matrix& view_matrix) {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    if (!draw_list) return;

    // لیست تمام استخوان‌های ممکن
    const std::vector<boneID> all_bones = {
        boneID::SKEL_Pelvis,
        boneID::SKEL_Spine0,
        boneID::SKEL_Spine1,
        boneID::SKEL_Spine2,
        boneID::SKEL_Spine3,
        boneID::SKEL_Neck_1,
        boneID::SKEL_Head,
        boneID::SKEL_L_Clavicle,
        boneID::SKEL_L_UpperArm,
        boneID::SKEL_L_Forearm,
        boneID::SKEL_L_Hand,
        boneID::SKEL_R_Clavicle,
        boneID::SKEL_R_UpperArm,
        boneID::SKEL_R_Forearm,
        boneID::SKEL_R_Hand,
        boneID::SKEL_L_Thigh,
        boneID::SKEL_L_Calf,
        boneID::SKEL_L_Foot,
        boneID::SKEL_R_Thigh,
        boneID::SKEL_R_Calf,
        boneID::SKEL_R_Foot
    };

    // رسم نقطه برای هر استخوان
    for (const auto& bone : all_bones) {
        Vec3 bone_pos = get_bone_position(ped, bone);
        Vec2 screen_pos;

        if (bone_pos.world_to_screen(view_matrix, screen_pos)) {
            draw_list->AddCircleFilled(
                ImVec2(screen_pos.x, screen_pos.y),
                3.0f,  // اندازه نقطه
                IM_COL32(255, 0, 0, 255)  // رنگ قرمز
            );

            // نمایش نام استخوان (اختیاری)
            char bone_name[32];
            snprintf(bone_name, sizeof(bone_name), "%d", static_cast<int>(bone));
            draw_list->AddText(
                ImVec2(screen_pos.x + 5, screen_pos.y - 5),
                IM_COL32(255, 255, 255, 255),
                bone_name
            );
        }
    }
}
void FiveM::ESP::RunESP()
{
    uintptr_t ped_replay_interface = mem.Read<uintptr_t>(offset::replay + 0x18);
    if (!ped_replay_interface) return;

    uintptr_t localPlayer = mem.Read<uintptr_t>(offset::world + 0x8);
    if (!localPlayer)
    {
        std::cout << "[local Player (returning to for loop)]" << std::endl;
        return;
    }
    uintptr_t pedList = mem.Read<uintptr_t>(ped_replay_interface + 0x100);
    if (!pedList) return;

    auto view_matrix = mem.Read<Matrix>(offset::viewport + 0x24C);
    int screenWidth = GetSystemMetrics(0);
    int screenHeight = GetSystemMetrics(1);

    Vec3 localPlayerPos = mem.Read<Vec3>(localPlayer + offset::playerPosition);

    for (int i = 0; i < 200; i++)
    {
        uintptr_t ped = mem.Read<uintptr_t>(pedList + i * 0x10);
        if (!ped || ped == localPlayer) continue;

        // Skip NPCs if showPeds is disabled
        if (!ConfigMenu::showPeds) {
            uintptr_t playerInfo = mem.Read<uintptr_t>(ped + offset::playerInfo);
            if (!playerInfo) continue;
        }

        int health = mem.Read<int>(localPlayer + offset::playerHealth);
        if (health < 1) continue;
        health = std::clamp(health, 0, 100);

        Vec3 origin = mem.Read<Vec3>(ped + offset::playerPosition);
        Vec3 headPos = origin; headPos.z += 0.9f;
        Vec3 feetPos = origin; feetPos.z -= 0.9f;

        Vec2 screenHead, screenFeet;
        if (!headPos.world_to_screen(view_matrix, screenHead) ||
            !feetPos.world_to_screen(view_matrix, screenFeet)) continue;

        if (screenHead.x <= 0 || screenHead.x >= screenWidth ||
            screenFeet.x <= 0 || screenFeet.x >= screenWidth) continue;

        if (ConfigMenu::showEsp)
        {
            float boxHeight = screenFeet.y - screenHead.y;
            float boxWidth = boxHeight * 0.45f;

            ImVec2 p1 = ImVec2(screenHead.x - boxWidth * 0.5f, screenHead.y);
            ImVec2 p2 = ImVec2(screenHead.x + boxWidth * 0.5f, screenFeet.y);

            ImU32 fillColor = IM_COL32(128, 128, 128, 100);
            ImU32 outlineColor = IM_COL32(255, 255, 255, 255);

            auto drawList = ImGui::GetBackgroundDrawList();
            if (ConfigMenu::filledBox)
            {
                drawList->AddRectFilled(p1, p2, fillColor);
            }

            drawList->AddRect(p1, p2, outlineColor);

            // Health bar
            float healthPerc = health / 100.0f;
            float barWidth = 5.0f;
            float barHeight = boxHeight * healthPerc;
            float barX = p1.x - barWidth - 2.0f;
            float barBotY = p2.y;
            float barFillY = barBotY - barHeight;

            drawList->AddRectFilled(
                ImVec2(barX, p1.y),
                ImVec2(barX + barWidth, barBotY),
                IM_COL32(0, 0, 0, 150)
            );

            ImU32 healthColor = ImColor::HSV(healthPerc * 0.33f, 1.0f, 1.0f);
            drawList->AddRectFilled(
                ImVec2(barX, barFillY),
                ImVec2(barX + barWidth, barBotY),
                healthColor
            );

            // Health text
            char hpText[8];
            snprintf(hpText, sizeof(hpText), "%d", health);
            ImVec2 textSize = ImGui::CalcTextSize(hpText);
            drawList->AddText(
                ImVec2(barX + (barWidth * 0.5f) - (textSize.x * 0.5f), barBotY + 2.0f),
                IM_COL32(255, 255, 255, 255),
                hpText
            );

            // Head marker
            drawList->AddCircleFilled(
                ImVec2(screenHead.x, screenHead.y),
                3.0f,
                IM_COL32(255, 255, 255, 100)
            );

            // Tracers
            if (ConfigMenu::showTracers)
            {
                ImVec2 startPos;

                switch (ConfigMenu::tracerStartPosition)
                {
                case ConfigMenu::TracerStartPosition::Top:
                    startPos = ImVec2(screenWidth / 2.0f, 0); // بالای صفحه
                    break;

                case ConfigMenu::TracerStartPosition::Center:
                    startPos = ImVec2(screenWidth / 2.0f, screenHeight / 2.0f); // وسط صفحه
                    break;

                case ConfigMenu::TracerStartPosition::Bottom:
                    startPos = ImVec2(screenWidth / 2.0f, (float)screenHeight); // پایین صفحه
                    break;
                }

                drawList->AddLine(
                    startPos,
                    ImVec2(screenHead.x, screenHead.y),
                    IM_COL32(255, 255, 255, 255),
                    1.5f
                );
            }
            enum class boneID : int {
                SKEL_Pelvis = 0,
                SKEL_Spine0 = 1,
                SKEL_Spine1 = 2,
                SKEL_Spine2 = 3,
                SKEL_Spine3 = 4,
                SKEL_Neck_1 = 5,
                SKEL_Head = 6,
                SKEL_L_Clavicle = 7,
                SKEL_L_UpperArm = 8,
                SKEL_L_Forearm = 9,
                SKEL_L_Hand = 10,
                SKEL_R_Clavicle = 11,
                SKEL_R_UpperArm = 12,
                SKEL_R_Forearm = 13,
                SKEL_R_Hand = 14,
                SKEL_L_Thigh = 15,
                SKEL_L_Calf = 16,
                SKEL_L_Foot = 17,
                SKEL_R_Thigh = 18,
                SKEL_R_Calf = 19,
                SKEL_R_Foot = 20
            };

            // Skeleton (Bone ESP)
// در حلقه ESP خود:
// در حلقه ESP اصلی:
            if (ConfigMenu::showBones) {
                uintptr_t bone_list = mem.Read<uintptr_t>(ped + FiveM::offset::boneList);
                if (bone_list) {
                    draw_skeleton(ped, view_matrix);
                }
            }

            // Rysowanie szkieletu (kości)
            // Rysowanie szkieletu (kości)

           // (ConfigMenu::showDisctance)
            {
                //Vec3 localplayer = mem.Read<Vec3>(offset::world + offset::localPlayer);
                //Vec3 pedPos = mem.Read<Vec3>(ped);

                //float distance = localplayer.Distance(pedPos);

            }

        }
    }
}

void FiveM::visuals::Croshair()
{
    if (ConfigMenu::showCroshair)
    {
        auto view_matrix = mem.Read<Matrix>(offset::viewport + 0x24C);
        auto drawList = ImGui::GetBackgroundDrawList();
        Vec3 screen;
        Vec2 org;
        if (screen.world_to_screen(view_matrix, org))
        {
            drawList->AddLine(
                { (float)(GetSystemMetrics(SM_CXSCREEN) / 2), (float)(GetSystemMetrics(SM_CYSCREEN) / 2 - 10) },
                { (float)(GetSystemMetrics(SM_CXSCREEN) / 2), (float)(GetSystemMetrics(SM_CYSCREEN) / 2 + 10) },
                ImColor(255, 0, 0, 255), 1.5f
            );
            drawList->AddLine(
                { (float)(GetSystemMetrics(SM_CXSCREEN) / 2 - 10), (float)(GetSystemMetrics(SM_CYSCREEN) / 2) },
                { (float)(GetSystemMetrics(SM_CXSCREEN) / 2 + 10), (float)(GetSystemMetrics(SM_CYSCREEN) / 2) },
                ImColor(255, 0, 0, 255), 1.5f
            );
        }
    }
}

