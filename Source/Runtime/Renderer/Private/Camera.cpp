//
// Created by yukai on 2026/1/7.
//

#include "Camera.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace TE
{
    void Camera::Update()
    {
        if (m_matrixDirty == true)
        {
            UpdateForce();
        }
    }

    void Camera::UpdateForce()
    {
        UpdateViewMatrix();
        UpdateProjectionMatrix();
    }

    void Camera::UpdateViewMatrix()
    {
        m_viewMatrix = glm::lookAt(m_position, m_position + m_front, m_worldUp);
        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix; // VP矩阵（投影*视图，注意顺序）
        m_matrixDirty = false; // 矩阵已更新，无需重复计算
    }

    void Camera::UpdateProjectionMatrix()
    {
        if (m_type == CameraType::Perspective) {
            // Vulkan 关键适配：Y轴翻转（Vulkan NDC Y轴向下，GLM默认向上）
            // glm::perspective 返回的投影矩阵 Y 轴是向上的，需乘 Y 翻转矩阵修正
            glm::mat4 proj = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
            proj[1][1] *= -1.0f; // 翻转 Y 轴，适配 Vulkan NDC
            m_projectionMatrix = proj;
        } else {
            // 正交相机：默认 2D 范围（-1~1），可通过 orthoSize 调整
            float orthoSize = 10.0f;
            glm::mat4 proj = glm::ortho(
                -orthoSize * m_aspectRatio, orthoSize * m_aspectRatio,
                -orthoSize, orthoSize,
                m_nearPlane, m_farPlane
            );
            proj[1][1] *= -1.0f; // 同样翻转 Y 轴
            m_projectionMatrix = proj;
        }
        m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
        m_matrixDirty = false;
    }

    void Camera::UpdateFrontFromEulerAngles()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        m_front = glm::normalize(front);
        // 同时更新 right/up 向量（可选，用于相机平移）
        m_right = glm::normalize(glm::cross(m_front, m_worldUp));
        m_up = glm::normalize(glm::cross(m_right, m_front));
    }

    void Camera::Move(CameraMovement direction, float deltaTime)
    {
        float velocity = m_moveSpeed * deltaTime;
        if (direction == CameraMovement::Forward)
            m_position += m_front * velocity;
        if (direction == CameraMovement::Backward)
            m_position -= m_front * velocity;
        if (direction == CameraMovement::Left)
            m_position -= glm::normalize(glm::cross(m_front, m_worldUp)) * velocity; // 右向量的反方向
        if (direction == CameraMovement::Right)
            m_position += glm::normalize(glm::cross(m_front, m_worldUp)) * velocity;
        if (direction == CameraMovement::Up)
            m_position += m_worldUp * velocity;
        if (direction == CameraMovement::Down)
            m_position -= m_worldUp * velocity;

        m_matrixDirty = true; // 位置变化，矩阵需要更新
    }

    void Camera::Rotate(float offsetX, float offsetY, float sensitivity)
    {
        offsetX *= sensitivity;
        offsetY *= sensitivity;

        // 更新欧拉角（Yaw：左右旋转，Pitch：上下旋转）
        m_yaw += offsetX;
        m_pitch += offsetY;

        // 限制 Pitch 范围，避免相机翻转（-89° ~ 89°）
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;

        // 从欧拉角更新 front/right/up 向量
        UpdateFrontFromEulerAngles();
        m_matrixDirty = true; // 朝向变化，矩阵需要更新
    }
} // TE