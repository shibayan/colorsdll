/*
    drawing.hpp
    COLORS Image Drawing Library
*/

#pragma once

#include "image.hpp"

bool draw_image(image &base, const image &elem, int x, int y, int opacity)
{
    // サイズを取得
    int width = elem.width();
    int height = elem.height();

    int stripe = width;
    int src_width = base.width();

    // クリッピング
    int sx = 0, sy = 0;
    if (!base.calc_clipping(x, y, sx, sy, width, height))
    {
        return false;
    }

    const color *p_elem = &elem[stripe * sy + sx];

    for (int i = y; i < height + y; ++i)
    {
        color *p_base = &base[src_width * i + x];

        for (int j = 0; j < width; ++j)
        {
            int alpha = p_base[j].alpha();
            int beta = p_elem[j].alpha() * opacity / 100;

            alpha = (255 - beta) * alpha / 255;

            int total = alpha + beta;

            if (total == 0)
            {
                continue;
            }

            p_base[j].red((p_base[j].red() * alpha + p_elem[j].red() * beta) / total);
            p_base[j].green((p_base[j].green() * alpha + p_elem[j].green() * beta) / total);
            p_base[j].blue((p_base[j].blue() * alpha + p_elem[j].blue() * beta) / total);
            p_base[j].alpha(total);
        }

        p_elem += stripe;
    }
    return true;
}

void fill_image(image &img, color &fill_color)
{
    int length = img.width() * img.height();

    color *pixels = img.buffer();

    for (int i = 0; i < length; ++i)
    {
        pixels[i] = fill_color;
    }
}