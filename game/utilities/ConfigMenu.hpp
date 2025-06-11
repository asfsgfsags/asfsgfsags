#pragma once

namespace ConfigMenu
{
    extern bool showEsp;
    extern bool showCroshair;
    extern bool showBones;
    extern bool showTracers;
    extern bool filledBox;
    extern bool showPeds;
    extern bool showDistance;  // اضافه کردن گزینه نمایش Distance

    enum class TracerStartPosition
    {
        Top,
        Center,
        Bottom
    };

    extern TracerStartPosition tracerStartPosition;

    void ResetToDefaults();
}
