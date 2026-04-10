// ToyEngine Input Module
// InputManager - 输入状态管理（键盘/鼠标）

#pragma once

#include "Math/MathTypes.h"
#include <unordered_map>

namespace TE
{

class Window;

class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void Init(Window* window);
    void Shutdown();

    void Tick();
    void PostTick();

    [[nodiscard]] bool IsKeyDown(int key) const;
    [[nodiscard]] bool IsKeyJustPressed(int key) const;
    [[nodiscard]] bool IsKeyJustReleased(int key) const;

    [[nodiscard]] bool IsMouseButtonDown(int button) const;
    [[nodiscard]] bool IsMouseButtonJustPressed(int button) const;
    [[nodiscard]] bool IsMouseButtonJustReleased(int button) const;

    [[nodiscard]] Vector2 GetMousePosition() const { return m_MousePosition; }
    [[nodiscard]] Vector2 GetMouseDelta() const { return m_MouseDelta; }
    [[nodiscard]] Vector2 GetScrollDelta() const { return m_ScrollDelta; }

private:
    void OnKeyEvent(int key, int scancode, int action, int mods);
    void OnMouseButtonEvent(int button, int action, int mods);
    void OnCursorPosEvent(double xpos, double ypos);
    void OnScrollEvent(double xoffset, double yoffset);

    [[nodiscard]] static bool GetState(const std::unordered_map<int, bool>& states, int code);

private:
    Window* m_Window = nullptr;
    bool m_Initialized = false;

    std::unordered_map<int, bool> m_KeyStates;
    std::unordered_map<int, bool> m_KeyJustPressed;
    std::unordered_map<int, bool> m_KeyJustReleased;

    std::unordered_map<int, bool> m_MouseButtonStates;
    std::unordered_map<int, bool> m_MouseButtonJustPressed;
    std::unordered_map<int, bool> m_MouseButtonJustReleased;

    Vector2 m_MousePosition = Vector2::Zero;
    Vector2 m_LastMousePosition = Vector2::Zero;
    Vector2 m_MouseDelta = Vector2::Zero;
    bool m_FirstMouseInput = true;

    Vector2 m_ScrollDelta = Vector2::Zero;
};

} // namespace TE
