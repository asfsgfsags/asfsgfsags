#pragma once
#include <cstdint>
#include <cstdio>
#include "../mem/memify.h"
#include <vector>
#include <string>
#include <thread>
#include <iostream>
#include <sstream>
#include <iomanip>    // dla std::fixed, std::setprecision
#include <algorithm>
#include "../math/math.h"
#include "../../src/game/utilities/ConfigMenu.hpp"


extern std::vector<std::string> processes;
extern memify mem;    

enum class boneID : uintptr_t
{
    SKEL_Pelvis = 0x2E28,
    SKEL_Spine0 = 0x5C01,
    SKEL_Spine1 = 0x60F0,
    SKEL_Spine2 = 0x60F1,
    SKEL_Spine3 = 0x60F2,
    SKEL_Neck_1 = 0x9995,
    SKEL_Head = 0x796E,
    SKEL_L_Thigh = 0xE39F,
    SKEL_L_Calf = 0xF9BB,
    SKEL_L_Foot = 0x3779,
    SKEL_R_Thigh = 0xCA72,
    SKEL_R_Calf = 0x9000,
    SKEL_R_Foot = 0xCC4D,
    SKEL_L_Clavicle = 0xFCD9,
    SKEL_L_UpperArm = 0xB1C5,
    SKEL_L_Forearm = 0xEEEB,
    SKEL_L_Hand = 0x49D9,
    SKEL_R_Clavicle = 0x29D2,
    SKEL_R_UpperArm = 0x9D4D,
    SKEL_R_Forearm = 0x6E5C,
    SKEL_R_Hand = 0xDEAD
};
Vec3 readBonePos(uintptr_t boneList, boneID id);
uintptr_t GetBoneAddress(uintptr_t boneList, boneID id);

namespace FiveM {
	namespace offset {
		extern uintptr_t world, replay, viewport, localPlayer;
		extern uintptr_t boneList, boneMatrix;
		extern uintptr_t playerInfo, playerHealth, playerPosition;
		extern uintptr_t base;
	}

	namespace ESP
	{
		void RunESP(); 
	}

	namespace self
	{
		void NoRecoil();
	}

	namespace visuals
	{
		void Croshair();
	}

	void Setup();
}