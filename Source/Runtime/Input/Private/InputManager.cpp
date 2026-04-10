// ToyEngine Input Module
// InputManager 实现

#include "InputManager.h"
#include "InputKeys.h"
#include "Window.h"

namespace TE
{

void InputManager::Init(Window* window)
{
    if (m_Initialized)
    {
        return;
    }

    m_Window = window;
    if (!m_Window)
    {
        return;
    }

    m_Window->SetKeyCallback([this](int key, int scancode, int action, int mods)
    {
        OnKeyEvent(key, scancode, action, mods);
    });
    m_Window->SetMouseButtonCallback([this](int button, int action, int mods)
    {
        OnMouseButtonEvent(button, action, mods);
    });
    m_Window->SetCursorPosCallback([this](double xpos, double ypos)
    {
        OnCursorPosEvent(xpos, ypos);
    });
    m_Window->SetScrollCallback([this](double xoffset, double yoffset)
    {
        OnScrollEvent(xoffset, yoffset);
    });

    m_Initialized = true;
}

void InputManager::Shutdown()
{
    if (!m_Initialized)
    {
        return;
    }

    if (m_Window)
    {
        m_Window->SetKeyCallback({});
        m_Window->SetMouseButtonCallback({});
        m_Window->SetCursorPosCallback({});
        m_Window->SetScrollCallback({});
    }

    m_KeyStates.clear();
    m_KeyJustPressed.clear();
    m_KeyJustReleased.clear();
    m_MouseButtonStates.clear();
    m_MouseButtonJustPressed.clear();
    m_MouseButtonJustReleased.clear();

    m_MousePosition = Vector2::Zero;
    m_LastMousePosition = Vector2::Zero;
    m_MouseDelta = Vector2::Zero;
    m_ScrollDelta = Vector2::Zero;
    m_FirstMouseInput = true;

    m_Window = nullptr;
    m_Initialized = false;
}

void InputManager::Tick()
{
    if (!m_Initialized)
    {
        return;
    }

    if (m_FirstMouseInput)
    {
        m_LastMousePosition = m_MousePosition;
        m_MouseDelta = Vector2::Zero;
        m_FirstMouseInput = false;
    }
    else
    {
        m_MouseDelta = m_MousePosition - m_LastMousePosition;
        m_LastMousePosition = m_MousePosition;
    }
}

void InputManager::PostTick()
{
    if (!m_Initialized)
    {
        return;
    }

    m_KeyJustPressed.clear();
    m_KeyJustReleased.clear();
    m_MouseButtonJustPressed.clear();
    m_MouseButtonJustReleased.clear();
    m_ScrollDelta = Vector2::Zero;
}

bool InputManager::IsKeyDown(int key) const
{
    return GetState(m_KeyStates, key);
}

bool InputManager::IsKeyJustPressed(int key) const
{
    return GetState(m_KeyJustPressed, key);
}

bool InputManager::IsKeyJustReleased(int key) const
{
    return GetState(m_KeyJustReleased, key);
}

bool InputManager::IsMouseButtonDown(int button) const
{
    return GetState(m_MouseButtonStates, button);
}

bool InputManager::IsMouseButtonJustPressed(int button) const
{
    return GetState(m_MouseButtonJustPressed, button);
}

bool InputManager::IsMouseButtonJustReleased(int button) const
{
    return GetState(m_MouseButtonJustReleased, button);
}

void InputManager::OnKeyEvent(int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    if (action == InputAction::Press)
    {
        if (!GetState(m_KeyStates, key))
        {
            m_KeyJustPressed[key] = true;
        }
        m_KeyStates[key] = true;
        return;
    }

    if (action == InputAction::Release)
    {
        if (GetState(m_KeyStates, key))
        {
            m_KeyJustReleased[key] = true;
        }
        m_KeyStates[key] = false;
        return;
    }

    if (action == InputAction::Repeat)
    {
        m_KeyStates[key] = true;
    }
}

void InputManager::OnMouseButtonEvent(int button, int action, int mods)
{
    (void)mods;

    if (action == InputAction::Press)
    {
        if (!GetState(m_MouseButtonStates, button))
        {
            m_MouseButtonJustPressed[button] = true;
        }
        m_MouseButtonStates[button] = true;
        return;
    }

    if (action == InputAction::Release)
    {
        if (GetState(m_MouseButtonStates, button))
        {
            m_MouseButtonJustReleased[button] = true;
        }
        m_MouseButtonStates[button] = false;
    }
}

void InputManager::OnCursorPosEvent(double xpos, double ypos)
{
    m_MousePosition = Vector2(static_cast<float>(xpos), static_cast<float>(ypos));
}

void InputManager::OnScrollEvent(double xoffset, double yoffset)
{
    m_ScrollDelta += Vector2(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

bool InputManager::GetState(const std::unordered_map<int, bool>& states, int code)
{
    const auto it = states.find(code);
    return it != states.end() ? it->second : false;
}

} // namespace TE
