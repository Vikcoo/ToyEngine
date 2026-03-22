// ToyEngine Core Module
// 整数向量和矩形类型 - 静态常量定义

#include "Math/VectorInt.h"

namespace TE {

// ==================== IntVector2 常量 ====================
const IntVector2 IntVector2::Zero(0, 0);
const IntVector2 IntVector2::One(1, 1);

// ==================== IntVector3 常量 ====================
const IntVector3 IntVector3::Zero(0, 0, 0);
const IntVector3 IntVector3::One(1, 1, 1);

// ==================== Rect 常量 ====================
const Rect Rect::Zero(0.0f, 0.0f, 0.0f, 0.0f);
const Rect Rect::Unit(0.0f, 0.0f, 1.0f, 1.0f);

// ==================== IntRect 常量 ====================
const IntRect IntRect::Zero(0, 0, 0, 0);

} // namespace TE
