#ifndef VEC3_H
#define VEC3_H

#include <cmath>
#include <iostream>

/**
 * 3次元ベクトルクラス
 * 位置(x, y, z)や色(r, g, b)、速度などを表現するために使います。
 */
class Vec3 {
public:
    double x, y, z;

    // コンストラクタ
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x, double y, double z) : x(x), y(y), z(z) {}

    // ベクトルの反転 (-v)
    Vec3 operator-() const { return Vec3(-x, -y, -z); }

    // ベクトル同士の加算 (v + u)
    Vec3 operator+(const Vec3& v) const {
        return Vec3(x + v.x, y + v.y, z + v.z);
    }

    // ベクトルへの代入加算 (v += u)
    Vec3& operator+=(const Vec3& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    // ベクトル同士の減算 (v - u)
    Vec3 operator-(const Vec3& v) const {
        return Vec3(x - v.x, y - v.y, z - v.z);
    }

    // ベクトルとスカラーの乗算 (v * t)
    Vec3 operator*(double t) const {
        return Vec3(x * t, y * t, z * t);
    }

    // スカラーによる除算 (v / t)
    Vec3 operator/(double t) const {
        return Vec3(x / t, y / t, z / t);
    }

    // ベクトルの長さ（大きさ）の2乗
    double length_squared() const {
        return x*x + y*y + z*z;
    }

    // ベクトルの長さ
    double length() const {
        return std::sqrt(length_squared());
    }

    // 単位ベクトル（長さが1のベクトル）を返す
    Vec3 normalized() const {
        double len = length();
        if (len == 0) return Vec3(0, 0, 0);
        return *this / len;
    }
    
    // 内積 (dot product)
    static double dot(const Vec3& u, const Vec3& v) {
        return u.x * v.x + u.y * v.y + u.z * v.z;
    }

    // 外積 (cross product)
    static Vec3 cross(const Vec3& u, const Vec3& v) {
        return Vec3(u.y * v.z - u.z * v.y,
                    u.z * v.x - u.x * v.z,
                    u.x * v.y - u.y * v.x);
    }
};

// スカラー * ベクトル (t * v) を可能にするための演算子オーバーロード
inline Vec3 operator*(double t, const Vec3& v) {
    return Vec3(t * v.x, t * v.y, t * v.z);
}

// ベクトルの内容を表示するための演算子オーバーロード (std::cout << v)
inline std::ostream& operator<<(std::ostream& out, const Vec3& v) {
    return out << v.x << ' ' << v.y << ' ' << v.z;
}

#endif // VEC3_H
