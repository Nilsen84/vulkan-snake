#include "snakegame.h"

#include <iostream>
#include <stdexcept>

#include <glm/ext/matrix_transform.hpp>

SnakeGame::SnakeGame() {
    placeFood();
}

void SnakeGame::runMainLoop() {
    while (!glfwWindowShouldClose(m_window.get())) {
        glfwPollEvents();

        auto time = glfwGetTime();
        if (time - m_tickTime >= m_tickDelay) {
            tick();
            m_tickTime = time;
        }

        m_renderer.begin({ 0.08, 0.08, 0.08, 1.0f });
        m_renderer.setColor({ 1.0f, 1.0f, 1.0f });
        for (auto snake: m_snake) {
            m_renderer.drawQuad(translate(PROJECTION, { snake, 0 }));
        }

        m_renderer.setColor({ 1.0f, 0.0f, 0.0f });
        m_renderer.drawQuad(translate(PROJECTION, { m_foodPos, 0 }));
        m_renderer.end();
    }
}

void SnakeGame::onKeyPress(int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_W:
        case GLFW_KEY_UP:
            m_directionQueue.emplace(0, 1);
            break;
        case GLFW_KEY_S:
        case GLFW_KEY_DOWN:
            m_directionQueue.emplace(0, -1);
            break;
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
            m_directionQueue.emplace(-1, 0);
            break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
            m_directionQueue.emplace(1, 0);
            break;
    }
}

void SnakeGame::placeFood() {
    static std::uniform_int_distribution X(0, WIDTH - 1);
    static std::uniform_int_distribution Y(0, HEIGHT - 1);

    do {
        m_foodPos = { X(m_randomEngine), Y(m_randomEngine) };
    } while (std::ranges::contains(m_snake, m_foodPos));
}

void SnakeGame::tick() {
    while (!m_directionQueue.empty()) {
        glm::ivec2 dir = m_directionQueue.front();
        m_directionQueue.pop();
        if (dir + m_direction != glm::ivec2 { 0, 0 }) {
            m_direction = dir;
            break;
        }
    }

    auto head = m_snake.front() + m_direction;
    if (head.x < 0 || head.x >= WIDTH || head.y < 0 || head.y >= HEIGHT) return reset();

    if (head == m_foodPos) {
        m_snake.push_front(head);
        if (m_snake.size() == WIDTH * HEIGHT) return reset();
        placeFood();
    } else {
        m_snake.pop_back();
        if (std::ranges::contains(m_snake, head)) return reset();
        m_snake.push_front(head);
    }
}

void SnakeGame::reset() {
    m_snake.assign({{ 1, HEIGHT/2 }});
    m_direction = { 0, 0 };
    m_directionQueue = {};
    placeFood();
}

GLFWwindow* SnakeGame::createWindow() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(WIDTH * PIXELS_PER_CELL, HEIGHT * PIXELS_PER_CELL, "Vulkan Snake", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        ((SnakeGame*)glfwGetWindowUserPointer(window))->onKeyPress(key, scancode, action, mods);
    });

    return window;
}

void SnakeGame::WindowDeleter::operator()(GLFWwindow *window) const noexcept {
    glfwDestroyWindow(window);
    glfwTerminate();
}
