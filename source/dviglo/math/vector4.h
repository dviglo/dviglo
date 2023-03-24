// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "vector3.h"

namespace dviglo
{

/// Four-dimensional vector.
struct DV_API Vector4
{
    /// Construct a zero vector.
    Vector4() noexcept :
        x(0.0f),
        y(0.0f),
        z(0.0f),
        w(0.0f)
    {
    }

    /// Copy-construct from another vector.
    Vector4(const Vector4& vector) noexcept = default;

    /// Construct from a 3-dimensional vector and the W coordinate.
    Vector4(const Vector3& vector, float w) noexcept :
        x(vector.x),
        y(vector.y),
        z(vector.z),
        w(w)
    {
    }

    /// Construct from coordinates.
    Vector4(float x, float y, float z, float w) noexcept :
        x(x),
        y(y),
        z(z),
        w(w)
    {
    }

    /// Construct from a float array.
    explicit Vector4(const float* data) noexcept :
        x(data[0]),
        y(data[1]),
        z(data[2]),
        w(data[3])
    {
    }

    /// Assign from another vector.
    Vector4& operator =(const Vector4& rhs) noexcept = default;

    /// Test for equality with another vector without epsilon.
    bool operator ==(const Vector4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }

    /// Test for inequality with another vector without epsilon.
    bool operator !=(const Vector4& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

    /// Add a vector.
    Vector4 operator +(const Vector4& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }

    /// Return negation.
    Vector4 operator -() const { return Vector4(-x, -y, -z, -w); }

    /// Subtract a vector.
    Vector4 operator -(const Vector4& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }

    /// Multiply with a scalar.
    Vector4 operator *(float rhs) const { return Vector4(x * rhs, y * rhs, z * rhs, w * rhs); }

    /// Multiply with a vector.
    Vector4 operator *(const Vector4& rhs) const { return Vector4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }

    /// Divide by a scalar.
    Vector4 operator /(float rhs) const { return Vector4(x / rhs, y / rhs, z / rhs, w / rhs); }

    /// Divide by a vector.
    Vector4 operator /(const Vector4& rhs) const { return Vector4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }

    /// Add-assign a vector.
    Vector4& operator +=(const Vector4& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    /// Subtract-assign a vector.
    Vector4& operator -=(const Vector4& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    /// Multiply-assign a scalar.
    Vector4& operator *=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    Vector4& operator *=(const Vector4& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }

    /// Divide-assign a scalar.
    Vector4& operator /=(float rhs)
    {
        float invRhs = 1.0f / rhs;
        x *= invRhs;
        y *= invRhs;
        z *= invRhs;
        w *= invRhs;
        return *this;
    }

    /// Divide-assign a vector.
    Vector4& operator /=(const Vector4& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
    }

    /// Return const value by index.
    float operator[](i32 index) const { return (&x)[index]; }

    /// Return mutable value by index.
    float& operator[](i32 index) { return (&x)[index]; }

    /// Calculate dot product.
    float DotProduct(const Vector4& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }

    /// Calculate absolute dot product.
    float AbsDotProduct(const Vector4& rhs) const
    {
        return dviglo::Abs(x * rhs.x) + dviglo::Abs(y * rhs.y) + dviglo::Abs(z * rhs.z) + dviglo::Abs(w * rhs.w);
    }

    /// Project vector onto axis.
    float ProjectOntoAxis(const Vector3& axis) const { return DotProduct(Vector4(axis.normalized(), 0.0f)); }

    /// Return absolute vector.
    Vector4 Abs() const { return Vector4(dviglo::Abs(x), dviglo::Abs(y), dviglo::Abs(z), dviglo::Abs(w)); }

    /// Linear interpolation with another vector.
    Vector4 Lerp(const Vector4& rhs, float t) const { return *this * (1.0f - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const Vector4& rhs) const
    {
        return dviglo::Equals(x, rhs.x) && dviglo::Equals(y, rhs.y) && dviglo::Equals(z, rhs.z) && dviglo::Equals(w, rhs.w);
    }

    /// Return whether any component is NaN.
    bool IsNaN() const { return dviglo::IsNaN(x) || dviglo::IsNaN(y) || dviglo::IsNaN(z) || dviglo::IsNaN(w); }

    /// Return whether any component is Inf.
    bool IsInf() const { return dviglo::IsInf(x) || dviglo::IsInf(y) || dviglo::IsInf(z) || dviglo::IsInf(w); }

    /// Convert to Vector2.
    explicit operator Vector2() const { return { x, y }; }

    /// Convert to Vector3.
    explicit operator Vector3() const { return { x, y, z }; }

    /// Return float data.
    const float* Data() const { return &x; }

    /// Return as string.
    String ToString() const;

    /// Return hash value for HashSet & HashMap.
    hash32 ToHash() const
    {
        hash32 hash = 37;
        hash = 37 * hash + FloatToRawIntBits(x);
        hash = 37 * hash + FloatToRawIntBits(y);
        hash = 37 * hash + FloatToRawIntBits(z);
        hash = 37 * hash + FloatToRawIntBits(w);

        return hash;
    }

    /// X coordinate.
    float x;
    /// Y coordinate.
    float y;
    /// Z coordinate.
    float z;
    /// W coordinate.
    float w;

    /// Zero vector.
    static const Vector4 ZERO;
    /// (1,1,1) vector.
    static const Vector4 ONE;
};

/// Multiply Vector4 with a scalar.
inline Vector4 operator *(float lhs, const Vector4& rhs) { return rhs * lhs; }

/// Per-component linear interpolation between two 4-vectors.
inline Vector4 VectorLerp(const Vector4& lhs, const Vector4& rhs, const Vector4& t) { return lhs + (rhs - lhs) * t; }

/// Per-component min of two 4-vectors.
inline Vector4 VectorMin(const Vector4& lhs, const Vector4& rhs) { return Vector4(Min(lhs.x, rhs.x), Min(lhs.y, rhs.y), Min(lhs.z, rhs.z), Min(lhs.w, rhs.w)); }

/// Per-component max of two 4-vectors.
inline Vector4 VectorMax(const Vector4& lhs, const Vector4& rhs) { return Vector4(Max(lhs.x, rhs.x), Max(lhs.y, rhs.y), Max(lhs.z, rhs.z), Max(lhs.w, rhs.w)); }

/// Per-component floor of 4-vector.
inline Vector4 VectorFloor(const Vector4& vec) { return Vector4(Floor(vec.x), Floor(vec.y), Floor(vec.z), Floor(vec.w)); }

/// Per-component round of 4-vector.
inline Vector4 VectorRound(const Vector4& vec) { return Vector4(Round(vec.x), Round(vec.y), Round(vec.z), Round(vec.w)); }

/// Per-component ceil of 4-vector.
inline Vector4 VectorCeil(const Vector4& vec) { return Vector4(Ceil(vec.x), Ceil(vec.y), Ceil(vec.z), Ceil(vec.w)); }

}
