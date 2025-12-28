#ifndef PHYSICS_H
#define PHYSICS_H

#include <cmath>
#include "vec3.h"

// 状態ベクトル（位置と運動量）
struct PhotonState {
    Vec3 pos;
    Vec3 mom;
    
    PhotonState operator+(const PhotonState& other) const {
        return { pos + other.pos, mom + other.mom };
    }
    PhotonState operator*(double s) const {
        return { pos * s, mom * s };
    }
};

inline PhotonState operator*(double s, const PhotonState& p) {
    return p * s;
}

// シュワルツシルト時空における光の運動方程式
// 近似モデルではなく、適切な運動方程式（ハミルトニアン由来）を使用
inline PhotonState schwarzschild_equations(const PhotonState& s, double /*lambda*/, double rs) {
    // 現在の位置の半径 r
    double r2 = s.pos.length_squared();
    double r = std::sqrt(r2);
    
    // r=0 での特異点回避
    if (r < 0.1 * rs) {
        return { Vec3(0,0,0), Vec3(0,0,0) };
    }

    // dx/dl = p (運動量)
    Vec3 d_pos = s.mom;

    // dp/dl = F (重力的な力)
    // ニュートン力学的な重力屈曲の一般相対論的補正を含む有効な形式
    // d^2x/dl^2 = - (3/2) * rs * (h^2 / r^5) * x
    // ここで h = x cross v (角運動量)
    
    Vec3 Lx = Vec3::cross(s.pos, s.mom);
    double h2 = Lx.length_squared();
    
    double factor = -1.5 * rs * h2 / (r2 * r2 * r);
    
    Vec3 d_mom = s.pos * factor;

    return { d_pos, d_mom };
}

#endif // PHYSICS_H
