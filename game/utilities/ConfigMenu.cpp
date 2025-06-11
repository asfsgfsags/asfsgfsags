#include "ConfigMenu.hpp"

namespace ConfigMenu
{
    bool showEsp = false;
    bool showCroshair = false;
    bool showBones = false;
    bool showTracers = false;
    bool filledBox = false;
    bool showPeds = false;
    bool showDistance = false;  // مقداردهی اولیه

    TracerStartPosition tracerStartPosition = TracerStartPosition::Top;

    void ResetToDefaults()
    {
        showEsp = true;
        showCroshair = false;
        showBones = true;
        showTracers = false;
        tracerStartPosition = TracerStartPosition::Top;
        filledBox = false;
        showPeds = false;
        showDistance = false;  // مقدار پیش‌فرض
    }
}
