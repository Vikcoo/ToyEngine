//
// Created by yukai on 2026/1/7.
//

#pragma once
#include <functional>



using InputSystemKeyCallback = std::function<void()>;


namespace TE
{
    class Window;
    class InputSystem
    {
    public:
        InputSystem(const Window& window);
        InputSystem(InputSystem&) = delete;
        InputSystem& operator=(InputSystem&) = delete;

        static InputSystem& GetInstance();

        void Initialize(const Window& window);

    private:
        InputSystemKeyCallback m_keyCallback;
    };

}
