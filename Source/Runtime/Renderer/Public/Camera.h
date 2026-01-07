//
// Created by yukai on 2026/1/7.
//

#pragma once
#include "glm/glm.hpp"


namespace TE
{
    enum class CameraMovement {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    // 相机类型
    enum class CameraType {
        Perspective, // 透视相机（3D 场景默认）
        Orthographic // 正交相机（2D UI/编辑器）
    };

    class Camera
    {
    public:
        Camera():   m_position( glm::vec3(0.0f, 0.0f, 3.0f)),
                    m_front(glm::vec3(0.0f, 0.0f, -1.0f)),
                    m_up(glm::vec3(0.0f, 1.0f, 0.0f)),
                    m_fov(45.0f),
                    m_nearPlane(0.1f),
                    m_farPlane(100.0f),
                    m_aspectRatio(16.0f / 9.0f),
        m_right(),
        m_worldUp(glm::vec3(0.0f, 1.0f, 0.0f))
        {}
        ~Camera() = default;

    void Update();
    void UpdateForce();
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();
    void UpdateFrontFromEulerAngles();
    void Move(CameraMovement direction, float deltaTime);
    void Rotate(float offsetX, float offsetY, float sensitivity = 0.1f);

    glm::mat4 GetViewMatrix() const { return m_viewMatrix; };
    glm::mat4 GetProjectionMatrix() const { return m_projectionMatrix; };
    glm::vec3 GetPosition() const { return m_position; };


    private:
        // 基础参数
        glm::vec3 m_position;     // 相机位置
        glm::vec3 m_front;        // 相机前方向（看向的方向）
        glm::vec3 m_right;        // 相机右方向
        glm::vec3 m_up;           // 相机上方向
        glm::vec3 m_worldUp;      // 世界坐标系上方向（默认 Y 轴）
        float m_fov;              // 透视FOV（角度）
        float m_nearPlane;        // 近裁剪面
        float m_farPlane;         // 远裁剪面
        float m_aspectRatio;      // 屏幕宽高比

        float m_yaw = -90.0f;     // 偏航角（Y轴旋转，左右）
        float m_pitch = 0.0f;     // 俯仰角（X轴旋转，上下）
        float m_moveSpeed = 5.0f; // 相机自由移动速度

        // 矩阵缓存
        glm::mat4 m_viewMatrix;           // 视图矩阵
        glm::mat4 m_projectionMatrix;     // 投影矩阵
        glm::mat4 m_viewProjectionMatrix; // VP矩阵（投影*视图）
        bool m_matrixDirty = false;       // 矩阵是否脏（需要更新）

       CameraType m_type = CameraType::Perspective;

    };
} // TE