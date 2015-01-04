/*
    image.hpp
    COLORS Imaging Library
*/

#pragma once

#include <memory>

template<int min, int max>
inline int round_pixel(int val)
{
    return val < min ? min : (val > max ? max : val);
}

inline int round_pixel(int val)
{
    return round_pixel<0, 255>(val);
}

template<class T>
inline T reverse_endian(T val)
{
    return (val >> 24) + ((val >> 8) & 0xFF00) + ((val << 8) & 0xFF0000) + (val << 24);
}

struct color
{
public:
    // 内部データ型を公開する
    typedef unsigned long value_type;
	typedef unsigned char pixel_type;
	// 実データ
    union
    {
        value_type _abgr;
        pixel_type _pixel[4];
    };
    // コンストラクタ
    color()
        : _abgr(0)
    {
    }
    color(int r, int g, int b)
		: _abgr((round_pixel(b) << 16) + (round_pixel(g) << 8) + round_pixel(r))
    {
    }
    color(int a, int r, int g, int b)
		: _abgr((round_pixel(a) << 24) + (round_pixel(b) << 16) + (round_pixel(g) << 8) + round_pixel(r))
    {
    }
    color(value_type argb)
        : _abgr(reverse_endian((argb << 8)) + (argb & 0xFF000000))
    {
    }
    color(int a, value_type rgb)
        : _abgr(reverse_endian(rgb << 8) + (round_pixel(a) << 24))
    {
    }
    color(const color &obj)
        : _abgr(obj.to_abgr())
    {
    }
    color(int a, const color &obj)
        : _abgr((obj.to_abgr() & 0xFFFFFF) + (round_pixel(a) << 24))
    {
    }
    inline int alpha() const
    {
        return _pixel[3];
    }
    inline void alpha(int value)
    {
        _pixel[3] = round_pixel(value);
    }
    inline void add_alpha(int value)
    {
        alpha(alpha() + value);
    }
    inline int red() const
    {
        return _pixel[0];
    }
    inline void red(int value)
    {
        _pixel[0] = round_pixel(value);
    }
    inline void add_red(int value)
    {
        red(red() + value);
    }
    inline int green() const
    {
        return _pixel[1];
    }
    inline void green(int value)
    {
        _pixel[1] = round_pixel(value);
    }
    inline void add_green(int value)
    {
        green(green() + value);
    }
    inline int blue() const
    {
        return _pixel[2];
    }
    inline void blue(int value)
    {
        _pixel[2] = round_pixel(value);
    }
    inline void add_blue(int value)
    {
        blue(blue() + value);
    }
    inline value_type to_rgb() const
    {
        return (_pixel[0] << 16) + (_pixel[1] << 8) + _pixel[2];
    }
    inline void from_rgb(value_type rgb)
    {
        _abgr = reverse_endian(rgb << 8) + (_abgr & 0xFF000000);
    }
    inline value_type to_abgr() const
    {
        return _abgr;
    }
    inline void from_abgr(value_type abgr)
    {
        _abgr = abgr;
    }
    inline value_type to_argb() const
    {
        return (_pixel[3] << 24) + (_pixel[0] << 16) + (_pixel[1] << 8) + _pixel[2];
    }
    inline color &operator=(const color &obj)
    {
        _abgr = obj.to_abgr();
        return *this;
    }
    inline bool equals_without_alpha(const color &obj) const
    {
        return (_abgr & 0xFFFFFF) == (obj.to_abgr() & 0xFFFFFF);
    }
    inline bool operator==(const color &obj) const
    {
        return _abgr == obj.to_abgr();
    }
};

class image
{
public:
    image()
        : _width(0), _height(0)
    {
    }
    image(int width, int height)
        : _width(width), _height(height), _buffer(new color[width * height])
    {
    }
    inline int width() const
    {
        return _width;
    }
    inline int height() const
    {
        return _height;
    }
    inline color *buffer()
    {
        return _buffer.get();
    }
    inline const color *buffer() const
    {
        return _buffer.get();
    }
    inline color &operator[](int index)
    {
        return _buffer[index];
    }
    inline const color &operator[](int index) const
    {
        return _buffer[index];
    }
    inline void pixel(const color &value, int x, int y)
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
        {
            _buffer[_width * y + x] = value;
        }
    }
    inline const color &pixel(int x, int y) const
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
        {
            return _buffer[_width * y + x];
        }
        return _buffer[0];
    }
    inline void pixel_no_check(const color &value, int x, int y)
    {
        _buffer[_width * y + x] = value;
    }
    inline const color &pixel_no_check(int x, int y) const
    {
        return _buffer[_width * y + x];
    }
    bool sub_image(image &dst, int x, int y, int width, int height) const
    {
        int sx = 0, sy = 0;
        int original_width = width;
        if (!calc_clipping(x, y, sx, sy, width, height))
        {
            return false;
        }
        int src_width = _width;
        int stride = width * sizeof(color);
        dst.resize(width, height);
        color *p = &dst[original_width * sy + sx];
        for (int i = y; i < height + y; ++i)
        {
            memcpy(p, &_buffer[src_width * i + x], stride);
            p += original_width;
        }
        return true;
    }
    bool resize(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return false;
        }
        _width = width;
        _height = height;
        _buffer.reset(new color[width * height]);
        return true;
    }
    template<class Sampler>
    void resize(image &dst, Sampler s) const
    {
        // リサイズ画像のサイズを取得
        int width = dst.width();
        int height = dst.height();

        // ピクセルの最大値を計算する
        int px_width = _width - 1;
        int px_height = _height - 1;

        // スケールを計算
        double scale_x = static_cast<double>(width) / _width;
        double scale_y = static_cast<double>(height) / _height;

        // 高速化のために一時的にポインタを使う
        color *pixels = dst.buffer();

        // 実際の処理
        for (int y = 0; y < height; ++y)
        {
            // 高速化のため事前に計算する
            double calc_y = (y / scale_y);

            // X 方向にループを回す
            for (int x = 0; x < width; ++x)
            {
                // オリジナルでの位置を計算して、sampler に処理を任す
                s(*this, (x / scale_x), calc_y, px_width, px_height, pixels[x]);
            }

            // ポインタを移動させる
            pixels += width;
        }
    }
    template<class Function>
    void transform(Function f)
    {
        // 高速化のため一時的にポインタを使う
        color *pixels = buffer();

        // 前もってピクセル数を計算しておく
        int length = _width * _height;

		// ループをアンロールする
		int mod = length & 7;
		length >>= 3;

        // ループでひたすら処理
        for (int i = 0; i < length; ++i)
        {
            f(pixels[0]);
            f(pixels[1]);
            f(pixels[2]);
            f(pixels[3]);
            f(pixels[4]);
            f(pixels[5]);
            f(pixels[6]);
            f(pixels[7]);

			pixels += 8;
        }

		for (int i = 0; i < mod; ++i)
		{
			f(pixels[i]);
		}
    }
    bool calc_clipping(int &x, int &y, int &sx, int &sy, int &width, int &height) const
    {
	    // 高速化のために取得しておく
	    int src_width = _width;
	    int src_height = _height;

	    // 描画先の位置が正しいか判定
	    if(x >= src_width || x >= src_height)
	    {
		    // 領域外参照 - 描画不可
		    return false;
	    }

	    // 描画先の位置が負になってるか判定
	    if (x < 0)
	    {
		    sx = -x;
		    width += x;
		    x = 0;
	    }
	    if (y < 0)
	    {
		    sy = -y;
		    height += y;
		    y = 0;
	    }

	    // 描画するサイズが正しいか判定
        if ((x + width) > src_width)
        {
            width = src_width - x;
        }
        if ((y + height) > src_height)
        {
            height = src_height - y;
        }

	    return true;
    }
private:
    int _width;
    int _height;
	std::unique_ptr<color[]> _buffer;
};