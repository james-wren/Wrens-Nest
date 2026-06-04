#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>

using namespace ftxui;

int homeScreen() {
    auto screen = ScreenInteractive::Fullscreen();

    auto renderer = Renderer([] {
        Elements servers;
        for (int i = 0; i < 3; i++){
            servers.push_back(text("Server " + std::to_string(i)));
        }

        return hbox({
            vbox({
                window(
                    text("Dev Server"),
                    filler()
                ) | flex,

                window(
                    text("Terminal"),
                    text("Input Box")
                ) | size(HEIGHT, EQUAL, 10),
            }) | flex,

            window(
                text("Servers"),
                vbox(std::move(servers))
            ) | size(WIDTH, EQUAL, 75)
        }) | flex;
    });

    screen.Loop(renderer);
    return 0;
}