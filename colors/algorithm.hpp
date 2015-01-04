/*
    algorithm.hpp
    COLORS Imaging Algorithm Library
*/

#pragma once

#include <cmath>

#include "image.hpp"

class repaint_function
{
public:
    repaint_function(const color &before, const color &after)
        : _before(before), _after(after)
    {
    }
    inline void operator()(color &pixel) const
    {
        if (pixel.equals_without_alpha(_before))
        {
            pixel.from_rgb(_after.to_rgb());
        }
    }
private:
    const color _before;
    const color _after;
};

class tone_function
{
public:
    tone_function(const int r, const int g, const int b)
        : _red(round_pixel<-255, 255>(r)), _green(round_pixel<-255, 255>(g)), _blue(round_pixel<-255, 255>(b))
    {
    }
    inline void operator()(color &pixel) const
    {
        if (pixel.alpha() != 0)
        {
            pixel.add_red(_red);
            pixel.add_green(_green);
            pixel.add_blue(_blue);
        }
    }
private:
    const int _red;
    const int _green;
    const int _blue;
};

class trans_function
{
public:
    trans_function(const color &color)
        : _transparent(255, color)
    {
    }
    inline void operator()(color &pixel) const
    {
        if (pixel == _transparent)
        {
            pixel.alpha(0);
        }
    }
private:
    const color _transparent;
};

class opacity_function
{
public:
    opacity_function(const int opacity)
        : _opacity(opacity)
    {
    }
    inline void operator()(color &pixel) const
    {
        pixel.alpha(round_pixel<0, 255>(pixel.alpha() * _opacity / 100));
    }
private:
    const int _opacity;
};

class nearest_neighbor_sampler
{
public:
    inline void operator()(const image &src, const double x, const double y, const int px_width, const int px_height, color &result) const
    {
        // 基準ピクセルを求める
        int x0 = static_cast<int>(x);
        int y0 = static_cast<int>(y);

        result = src.pixel(x0, y0);
    }
};

class bilinear_sampler
{
public:
    inline void operator()(const image &src, const double x, const double y, const int px_width, const int px_height, color &result) const
    {
        // 基準ピクセルを求める
        int x0 = static_cast<int>(x);
        int y0 = static_cast<int>(y);

        // ピクセルの距離を計算する
        double a = x - x0;
        double b = y - y0;

        // 先に +1 した座標を求めておく
        int x1 = min(x0 + 1, px_width);
        int y1 = min(y0 + 1, px_height);

        // 周辺の 4 ピクセルを取得
        const color &p00 = src.pixel_no_check(x0, y0);
        const color &p10 = src.pixel_no_check(x1, y0);
        const color &p01 = src.pixel_no_check(x0, y1);
        const color &p11 = src.pixel_no_check(x1, y1);

        // 重み付けを行う
        result.alpha(bilinear_interpolation(a, b, p00.alpha(), p10.alpha(), p01.alpha(), p11.alpha()));
        result.red(bilinear_interpolation(a, b, p00.red(), p10.red(), p01.red(), p11.red()));
        result.green(bilinear_interpolation(a, b, p00.green(), p10.green(), p01.green(), p11.green()));
        result.blue(bilinear_interpolation(a, b, p00.blue(), p10.blue(), p01.blue(), p11.blue()));
    }
private:
    inline int bilinear_interpolation(double a, double b, int p00, int p10, int p01, int p11) const
    {
        return static_cast<int>((1 - a) * (1 - b) * p00 + a * (1 - b) * p10 + (1 - a) * b * p01 + a * b * p11);
    }
};

class bicubic_sampler
{
public:
    inline void operator()(const image &src, const double x, const double y, const int px_width, const int px_height, color &result) const
    {
        // パラメータ作成
        static const int a = -1;
        static const double params[] = { a + 3.0, a + 2.0, -a * 4.0, a * 8.0, a * 5.0 };

        // 基準ピクセルを求める
        int x0 = static_cast<int>(x);
        int y0 = static_cast<int>(y);

        // 補間した色を初期化しておく
        double alpha = 0.0, red = 0.0, green = 0.0, blue = 0.0;

        // 4x4 の範囲で処理を行う
        for (int i = -1; i <= 2; ++i)
        {
            // 補間先のピクセル位置を計算する
            int x1 = x0 + i;

            // ピクセル間の距離を求める
            double dist_x = abs(x1 - x);

            // 領域外を参照しないようにする
            x1 = min(max(x1, 0), px_width);

            // X 方向の重み
            double x_weight = 0.0;

            // X 方向に計算する
            if (dist_x <= 1.0)
            {
                x_weight = 1.0 - params[0] * dist_x * dist_x + params[1] * dist_x * dist_x * dist_x;
            }
            else if (dist_x <= 2.0)
            {
                x_weight = params[2] + params[3] * dist_x - params[4] * dist_x * dist_x + a * dist_x * dist_x * dist_x;
            }
            else
            {
                continue;
            }

            // Y 方向に処理を行う
            for (int j = -1; j <= 2; ++j)
            {
                // 補間先のピクセル位置を計算する
                int y1 = y0 + j;

                // ピクセル間の距離を求める
                double dist_y = abs(y1 - y);

                // 重み
                double weight = x_weight;

                // Y 方向に計算する
                if (dist_y <= 1.0)
                {
                    weight *= 1.0 - params[0] * dist_y * dist_y + params[1] * dist_y * dist_y * dist_y;
                }
                else if (dist_y <= 2.0)
                {
                    weight *= params[2] + params[3] * dist_y - params[4] * dist_y * dist_y + a * dist_y * dist_y * dist_y;
                }
                else
                {
                    continue;
                }

                // 範囲内なのは確定済みなのでノーチェックでピクセル値を取得
                const color &color = src.pixel_no_check(x1, min(max(y1, 0), px_height));

                // 重み付けしながらそれぞれの画素値を足していく
                alpha += color.alpha() * weight;
                red += color.red() * weight;
                green += color.green() * weight;
                blue += color.blue() * weight;
            }
        }

        result.alpha(static_cast<int>(alpha));
        result.red(static_cast<int>(red));
        result.green(static_cast<int>(green));
        result.blue(static_cast<int>(blue));
    }
};

template<int n>
class lanczos_sampler
{
public:
    inline void operator()(const image &src, const double x, const double y, const int px_width, const int px_height, color &result) const
    {
        // Lanczos のパラメータを計算
        static const int nx = n - 1;
        
        // PI を計算
        static const double PI = 6.0 * asin( 0.5 );

        // 基準ピクセルを求める
        int x0 = static_cast<int>(x);
        int y0 = static_cast<int>(y);

        // 重みの合計値
        double total_weight = 0.0;

        // 補間した色を初期化しておく
        double alpha = 0.0, red = 0.0, green = 0.0, blue = 0.0;

        // (n * 2)x(n * 2) の範囲で処理を行う
        for (int i = nx; i <= n; ++i)
        {
            // 補間先のピクセル位置を計算する
            int x1 = x0 + i;

            // ピクセル間の距離を求める
            double dist_x = abs(x1 - x);

            // 領域外を参照しないようにする
            x1 = min(max(x1, 0), px_width);

            // X 方向の重み
            double x_weight = 0.0;

            // X 方向に計算する
            if (dist_x == 0.0)
            {
                x_weight = 1.0;
            }
            else if (dist_x <= n)
            {
                double dpx = PI * dist_x;
                x_weight = (sin(dpx) * sin(dpx / n)) / (dpx * (dpx / n));
            }
            else
            {
                continue;
            }

            // Y 方向に処理を行う
            for (int j = -nx; j <= n; ++j)
            {
                // 補間先のピクセル位置を計算する
                int y1 = y0 + j;

                // ピクセル間の距離を求める
                double dist_y = abs(y1 - y);

                // 重み
                double weight = x_weight;

                // Y 方向に計算する
                if (dist_y == 0.0)
                {
                }
                else if (dist_y < n)
                {
                    double dpy = PI * dist_y;
                    weight *= (sin(dpy) * sin(dpy / n)) / (dpy * (dpy / n));
                }
                else
                {
                    continue;
                }

                // 範囲内なのは確定済みなのでノーチェックでピクセル値を取得
                const color &color = src.pixel_no_check(x1, y1);

                // 重み付けしながらそれぞれの画素値を足していく
                alpha += color.alpha() * weight;
                red += color.red() * weight;
                green += color.green() * weight;
                blue += color.blue() * weight;

                // 重みも足していく
                total_weight += weight;
            }
        }

        result.alpha(static_cast<int>(alpha / total_weight));
        result.red(static_cast<int>(red / total_weight));
        result.green(static_cast<int>(green / total_weight));
        result.blue(static_cast<int>(blue / total_weight));
    }
};

typedef lanczos_sampler<2> lanczos2_sampler;

typedef lanczos_sampler<3> lanczos3_sampler;

typedef lanczos_sampler<4> lanczos4_sampler;