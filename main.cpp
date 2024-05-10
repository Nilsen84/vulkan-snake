#include <iostream>
#include <stdexcept>
#include <print>

#include "snakegame.h"

int main() {
    try {
        SnakeGame game;
        game.runMainLoop();
    } catch (const vk::SystemError& ex) {
        std::cerr << "Vulkan Error: " << ex.what() << " (" << ex.code().value() << ")\n";
        return 1;
    } catch (const std::runtime_error& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    return 0;
}
