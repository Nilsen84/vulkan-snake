#pragma once

#include <memory>
#include <deque>
#include <queue>
#include <random>

#include <glm/ext/matrix_clip_space.hpp>
#include <GLFW/glfw3.h>

#include "renderer.h"

class SnakeGame {
public:
    SnakeGame();
    void runMainLoop();

private:
    GLFWwindow* createWindow();
    struct WindowDeleter {
        void operator()(GLFWwindow* window) const noexcept;
    };

    void onKeyPress(int key, int scancode, int action, int mods);
    void placeFood();
    void tick();
    void reset();

private:
    static constexpr int WIDTH = 25, HEIGHT = 21, PIXELS_PER_CELL = 25;
    static inline glm::mat4 PROJECTION = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f);

    std::unique_ptr<GLFWwindow, WindowDeleter> m_window { createWindow() };
    Renderer m_renderer { m_window.get() };

    std::deque<glm::ivec2> m_snake {{ 1, HEIGHT/2 }};
    glm::ivec2 m_foodPos {};
    std::queue<glm::ivec2> m_directionQueue {};
    glm::ivec2 m_direction { 0, 0 };
    std::default_random_engine m_randomEngine {std::random_device{}()};

    double m_tickTime = glfwGetTime();
    double m_tickDelay = 0.1;
};
