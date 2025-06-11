#include "window/window.hpp"
#include "game/game.h"
#include "../src/game/utilities/ConfigMenu.hpp"
#include <thread>
#include <iostream>

int main() {
    ShowWindow(GetConsoleWindow(), SW_SHOW);
    Overlay overlay;
    overlay.SetupOverlay("SteamWebHelper");
    FiveM::Setup();

    while (overlay.shouldRun) {
        std::this_thread::sleep_for(std::chrono::milliseconds(0));
        overlay.StartRender();

        if (overlay.RenderMenu) {
            overlay.Render();
        }

        FiveM::ESP::RunESP();

        overlay.EndRender();
    }

}
