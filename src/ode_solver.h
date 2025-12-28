#ifndef ODE_SOLVER_H
#define ODE_SOLVER_H

/**
 * 4次のルンゲ・クッタ法 (Runge-Kutta 4th order method)
 * 微分方程式 dx/dt = f(x, t) を数値的に解くためのアルゴリズムです。
 * 
 * @tparam State 状態変数の型 (Vec3など、加算とスカラー倍ができる型)
 * @tparam RateFunc 微分方程式を表す関数の型 (State func(const State& x, double t))
 * 
 * @param x 現在の状態
 * @param t 現在の時刻
 * @param dt タイムステップ（時間の進み幅）
 * @param func 現在の状態と時刻から、変化率(dx/dt)を計算する関数
 * @return dt秒後の推定される状態
 */
template <typename State, typename RateFunc>
State rk4_step(const State& x, double t, double dt, RateFunc func) {
    // k1: 現在の点での傾き
    State k1 = func(x, t);
    
    // k2: 中点(t + dt/2)での傾き。k1を使って中点の位置を予測。
    State k2 = func(x + k1 * (dt * 0.5), t + dt * 0.5);
    
    // k3: 中点(t + dt/2)での傾き。k2を使って中点の位置を予測（より高精度）。
    State k3 = func(x + k2 * (dt * 0.5), t + dt * 0.5);
    
    // k4: 終点(t + dt)での傾き。k3を使って終点の位置を予測。
    State k4 = func(x + k3 * dt, t + dt);

    // 4つの傾きを重み付け平均して、次の位置を決定
    return x + (k1 + k2 * 2.0 + k3 * 2.0 + k4) * (dt / 6.0);
}

#endif // ODE_SOLVER_H
