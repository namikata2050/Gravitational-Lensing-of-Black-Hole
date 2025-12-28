#include <iostream>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cstring>
#include "vec3.h"
#include "ode_solver.h"
#include "physics.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <emscripten.h>
#include <emscripten/bind.h>
#include <SDL2/SDL.h>

// グローバル変数
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;

// 内部解像度（負荷と画質のバランス調整）
const int image_width = 500; // Medium Quality
const int image_height = 300; // Medium Quality

std::vector<uint8_t> pixels(image_width * image_height * 4);

// -- 計算済みデータ --
struct PrecomputedPixel {
    bool hit;       // ブラックホールに衝突したか
    Vec3 local_dir; // 衝突しなかった場合の「カメラローカル座標系での」脱出方向ベクトル
};
std::vector<PrecomputedPixel> distortion_map(image_width * image_height);
bool distortion_map_ready = false;
int calc_row = 0;
bool is_calculating = false;

// -- シミュレーション変数 --
double g_rs = 4.0; // Schwarzschild Radius (Initial: Massive)
double camera_yaw = 0.0;
double camera_pitch = 0.0;
double camera_dist = 150.0;
Vec3 camera_pos(0.0, 0.0, -150.0);

// 背景画像データ
std::vector<uint8_t> bg_image_data;
int bg_width = 0;
int bg_height = 0;

void reset_calculation(); //追加した

// 前方宣言
// void precompute_distortion();

// -- JS連携 --
void set_mass(double val) { 
    if (g_rs != val) { 
        g_rs = val; 
        // 質量が変わったので再計算
        reset_calculation();
    }
}

// (中間省略なしで一括置換はできないので、multi_replaceを使うべき局面ですが、行が離れているため2回に分けるかmulti_replaceを使います。今回はmulti_replaceを使います。)

void set_dist(double val) {
    if (camera_dist != val) { 
        camera_dist = val; 
        // 距離が変わったので再計算
        reset_calculation();
    }
}

void set_background_image(int width, int height, uintptr_t data_ptr) {
    bg_width = width;
    bg_height = height;
    bg_image_data.resize(width * height * 4);
    uint8_t* src = reinterpret_cast<uint8_t*>(data_ptr);
    std::memcpy(bg_image_data.data(), src, width * height * 4);
}

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("set_mass", &set_mass);
    emscripten::function("set_dist", &set_dist);
    emscripten::function("set_background_image", &set_background_image, emscripten::allow_raw_pointers());
}


// -- 描画処理 --

// Simple hash function for procedural generation
double hash13(double x, double y, double z) {
    double p = x * 12.9898 + y * 78.233 + z * 37.719;
    return std::fmod(std::sin(p) * 43758.5453, 1.0);
}

Vec3 get_background_color(const Vec3& dir) {
    // カスタム背景画像があれば平面投影（Planar Projection）を使用
    // カスタム背景画像があれば平面投影（Planar Projection）を使用
    if (!bg_image_data.empty() && bg_width > 0 && bg_height > 0) {
        // ■ 修正: 画像を背後（z < 0）に配置する
        // 前方（z >= 0）は黒。
        // ■ 修正: 画像を前方（z > 0）に配置する（ブラックホールの奥）
        // 背後（z <= 0）は黒。
        if (dir.z <= 0.0) return Vec3(0, 0, 0);

        double dist = dir.z; // 距離はそのまま正の値
        double scale = 0.15; // FOVスケール調整: 画像をさらに小さく（0.05 -> 0.15）
        double aspect = static_cast<double>(bg_height) / bg_width;

        // 平面投影
        double v = 0.5 - (dir.y / dist) * scale;
        double u = 0.5 + (dir.x / dist) * scale * aspect;

        if (u >= 0.0 && u <= 1.0 && v >= 0.0 && v <= 1.0) {
            int tx = static_cast<int>(u * (bg_width - 1));
            int ty = static_cast<int>(v * (bg_height - 1));
            // Clamp just in case
            tx = std::max(0, std::min(tx, bg_width - 1));
            ty = std::max(0, std::min(ty, bg_height - 1));
            int idx = (ty * bg_width + tx) * 4;
            return Vec3(bg_image_data[idx]/255.0, bg_image_data[idx+1]/255.0, bg_image_data[idx+2]/255.0);
        } else {
            return Vec3(0, 0, 0); // 画像範囲外は黒
        }
    } 
    
    // 画像がない場合はプロシージャル星空（変更なし）
    // 球面マッピングによる座標取得
    double theta = acos(-dir.y);
    double phi = atan2(dir.z, dir.x);
    double u = (phi + M_PI) / (2.0 * M_PI);
    double v = theta / M_PI; 
    
    // -- プロシージャル星空 --
    
    // ベースとなる宇宙の色（深い青黒）
    Vec3 color(0.02, 0.02, 0.05);

    // 星の生成
    // 方向ベクトルをグリッドで区切って確率的に星を配置する簡易実装
    
    // Layer 1: Small stars
    {
        double scale = 100.0;
        double dx = floor(dir.x * scale);
        double dy = floor(dir.y * scale);
        double dz = floor(dir.z * scale);
        double h = std::abs(hash13(dx, dy, dz));
        if (h > 0.98) {
            double intensity = (h - 0.98) / 0.02; // 0.0 - 1.0
            color = color + Vec3(1,1,1) * intensity * 0.8;
        }
    }
    
    // Layer 2: Medium stars
    {
        double scale = 50.0;
        double dx = floor(dir.x * scale + 100.0);
        double dy = floor(dir.y * scale + 100.0);
        double dz = floor(dir.z * scale + 100.0);
        double h = std::abs(hash13(dx, dy, dz));
        if (h > 0.99) {
            double intensity = (h - 0.99) / 0.01; 
            color = color + Vec3(0.8, 0.9, 1.0) * intensity * 1.5;
        }
    }

    // Layer 3: Nebulae (Milk way approximation)
    // 銀河面っぽい帯 (y成分が0に近いところ)
    double band = std::exp(-dir.y * dir.y * 10.0); // y=0付近で1.0
    // 少しムラをつける
    double noise = std::abs(std::sin(dir.x * 5.0) * std::cos(dir.z * 5.0));
    color = color + Vec3(0.1, 0.05, 0.2) * band * (0.5 + 0.5 * noise);

    return color;
}

// ベクトルの回転 (Rodrigues' rotation formula or Manual Matrix mult)
Vec3 rotate_vector(const Vec3& v, double yaw, double pitch) {
    // 1. Pitch (X軸周り)
    double cp = cos(pitch);
    double sp = sin(pitch);
    double y1 = v.y * cp - v.z * sp;
    double z1 = v.y * sp + v.z * cp;
    double x1 = v.x;
    
    // 2. Yaw (Y軸周り)
    double cy = cos(yaw);
    double sy = sin(yaw);
    double x2 = x1 * cy + z1 * sy;
    double z2 = -x1 * sy + z1 * cy;
    double y2 = y1;
    
    return Vec3(x2, y2, z2);
}

// 事前計算ロジック
void reset_calculation() {
    calc_row = 0;
    is_calculating = true;
    distortion_map_ready = false;
    // Show loading screen
    EM_ASM({
        document.getElementById('loading').style.display = 'block';
        document.getElementById('progress-bar').style.width = '0%';
        document.getElementById('status').textContent = "Calculating...";
    });
}

void step_calculation() {
    if (!is_calculating) return;

    // Process N rows per frame
    int rows_per_step = 5; 
    int end_row = std::min(calc_row + rows_per_step, image_height);

    Vec3 calc_cam_pos(0, 0, -camera_dist);
    Vec3 forward(0, 0, 1);
    Vec3 right(1, 0, 0);
    Vec3 up(0, 1, 0);

    double screen_dist = 10.0;
    double screen_width = 32.0; // 8.0 * 4 to match PIP Ultra Wide Angle
    double screen_height = screen_width * ((double)image_height / image_width);
    
    Vec3 screen_origin = calc_cam_pos + forward * screen_dist 
                         - right * (screen_width * 0.5) 
                         - up * (screen_height * 0.5);
    
    Vec3 screen_x = right * (screen_width / image_width);
    Vec3 screen_y = up * (screen_height / image_height);

    double rh = g_rs;

    for (int j = calc_row; j < end_row; ++j) {
        for (int i = 0; i < image_width; ++i) {
            Vec3 pixel_pos = screen_origin + screen_x * (double)i + screen_y * (double)(image_height - 1 - j);
            Vec3 ray_dir = (pixel_pos - calc_cam_pos).normalized();
            
            PhotonState state = { calc_cam_pos, ray_dir };
            
            bool hit_bh = false;
            double t = 0;
            
            // Adaptive Step Size
            int max_steps = 2000;
            for (int step = 0; step < max_steps; ++step) {
                double r_curr = std::sqrt(state.pos.length_squared());
                double dt = 0.5;
                if (r_curr < g_rs * 10.0) dt = 0.1;
                if (r_curr < g_rs * 3.0) dt = 0.02;
                
                auto equations = [&](const PhotonState& s, double t) {
                    return schwarzschild_equations(s, t, g_rs);
                };
                state = rk4_step(state, t, dt, equations);
                
                double r_sq = state.pos.length_squared();
                if (r_sq < rh * rh * 1.1) { hit_bh = true; break; }
                if (r_sq > 1000.0 * 1000.0) break;
            }
            
            int idx = j * image_width + i;
            distortion_map[idx].hit = hit_bh;
            if (!hit_bh) {
                distortion_map[idx].local_dir = state.mom.normalized();
            }
        }
    }
    
    calc_row = end_row;

    // Update progress
    double progress = (double)calc_row / image_height * 100.0;
    EM_ASM({
        document.getElementById('progress-bar').style.width = $0 + '%';
    }, progress);

    if (calc_row >= image_height) {
        is_calculating = false;
        distortion_map_ready = true;
        // Hide loading screen
        EM_ASM({
            document.getElementById('loading').style.display = 'none';
            document.getElementById('status').textContent = "Ready";
        });
        std::cout << "[C++] Calculation finished." << std::endl;
    }
}

// 前方宣言
void reset_calculation();
void step_calculation();

void main_loop() {
    static bool first = true;
    if (first) {
        reset_calculation();
        first = false;
    }
    
    if (is_calculating) {
        step_calculation();
        // 計算中は描画をスキップするか、途中経過を表示してもよいが、
        // 今回はロード画面を出しているのでスキップでOK。
        // ただし画面クリアくらいはしておくと良いかも。
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        return; 
    }

    // イベント処理
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_MOUSEMOTION) {
            if (event.motion.state & SDL_BUTTON_LMASK) {
                camera_yaw -= event.motion.xrel * 0.005;
                camera_pitch += event.motion.yrel * 0.005;
                camera_pitch = std::max(-1.5, std::min(1.5, camera_pitch));
            }
        }
    }

    if (!distortion_map_ready) return;

    // カメラの前方ベクトル（ワールド座標）を計算
    Vec3 cam_fwd_local(0, 0, 1);
    Vec3 cam_fwd = rotate_vector(cam_fwd_local, camera_yaw, camera_pitch);

    // 高速描画ループ
    // 全ピクセルを一気に更新
    for (int j = 0; j < image_height; ++j) {
        for (int i = 0; i < image_width; ++i) {
            int idx = j * image_width + i;
            const PrecomputedPixel& p = distortion_map[idx];
            
            Vec3 pixel_color(0,0,0);
            
            if (!p.hit) {
                Vec3 world_dir = rotate_vector(p.local_dir, camera_yaw, camera_pitch);
                
                // 可視化制限解除: 重力レンズ効果で回り込んだ光もすべて表示する
                pixel_color = get_background_color(world_dir);
            }
            
            int p_idx = idx * 4;
            pixels[p_idx + 0] = static_cast<int>(255.99 * std::min(pixel_color.x, 1.0));
            pixels[p_idx + 1] = static_cast<int>(255.99 * std::min(pixel_color.y, 1.0));
            pixels[p_idx + 2] = static_cast<int>(255.99 * std::min(pixel_color.z, 1.0));
            pixels[p_idx + 3] = 255;
        }
    }

    // -- Reference View (PIP: No Gravity) --
    // ブラックホールがない場合の星空を右上に表示
    int pip_scale = 4; // 1/4 size
    int pip_w = image_width / pip_scale;
    int pip_h = image_height / pip_scale;
    int pip_offset_x = image_width - pip_w - 20; // 20px padding
    int pip_offset_y = 20;

    // PIP用のスクリーン設定 (超広角: 4倍のFOV)
    double pip_screen_dist = 10.0;
    double pip_screen_width = 32.0; // 8.0 * 4.0 (Ultra Wide Angle)
    double pip_screen_height = pip_screen_width * ((double)image_height / image_width);
    
    // カメラ（ローカル座標）
    Vec3 local_cam_pos(0, 0, -camera_dist);
    Vec3 local_forward(0, 0, 1);
    Vec3 local_right(1, 0, 0);
    Vec3 local_up(0, 1, 0);

    Vec3 screen_origin = local_cam_pos + local_forward * pip_screen_dist 
                         - local_right * (pip_screen_width * 0.5) 
                         - local_up * (pip_screen_height * 0.5);
    Vec3 screen_dx = local_right * (pip_screen_width / pip_w);   // Step per PIP pixel (not main pixel)
    Vec3 screen_dy = local_up * (pip_screen_height / pip_h);

    for (int y = 0; y < pip_h; ++y) {
        for (int x = 0; x < pip_w; ++x) {
            // PIPピクセルごとのレイ方向計算 (広角)
            // ユーザー指示により上下反転: y -> (pip_h - 1 - y)
            Vec3 pixel_pos = screen_origin + screen_dx * (double)x + screen_dy * (double)(pip_h - 1 - y);
            Vec3 local_dir = (pixel_pos - local_cam_pos).normalized();
            
            // Apply current rotation (Sync with main camera, looking forward at the background)
            Vec3 world_dir = rotate_vector(local_dir, camera_yaw, camera_pitch);
            
            // Get background color (No Gravity)
            Vec3 color = get_background_color(world_dir);

            // Draw to buffer
            int dest_x = pip_offset_x + x;
            int dest_y = pip_offset_y + y;
            int idx = (dest_y * image_width + dest_x) * 4;

            // White Border
            if (x == 0 || x == pip_w - 1 || y == 0 || y == pip_h - 1) {
                pixels[idx+0] = 255; pixels[idx+1] = 255; pixels[idx+2] = 255;
            } else {
                pixels[idx+0] = static_cast<int>(255.99 * std::min(color.x, 1.0));
                pixels[idx+1] = static_cast<int>(255.99 * std::min(color.y, 1.0));
                pixels[idx+2] = static_cast<int>(255.99 * std::min(color.z, 1.0));
            }
            pixels[idx+3] = 255;
        }
    }
    // Add "No Gravity" Label? (Too complex for pixel manipulation, skip for now)
    
    SDL_UpdateTexture(texture, nullptr, pixels.data(), image_width * 4);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    SDL_CreateWindowAndRenderer(image_width, image_height, 0, &window, &renderer);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, image_width, image_height);
    
    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}
