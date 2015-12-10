/*
    colors.cpp
    COLORS Core Library
*/

#include <map>

#include "saori.h"

#include "image.hpp"
#include "png.hpp"
#include "algorithm.hpp"
#include "drawing.hpp"

typedef std::multimap<int, std::unique_ptr<image>> image_multimap;

// 全ての画像を保持するコンテナ
static image_multimap images;

#define GENERATE_IMAGE_INDEX(id) int id = static_cast<int>(images.size() + 1)
#define VERIFY_IMAGE_INDEX(index) if (index < 1 || static_cast<int>(images.size()) < index) { return SAORIRESULT_BAD_REQUEST; }

// 新しく画像を作成する
DEFINE_SAORI_FUNCTION(new)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(2);

    // 追加パラメータを取得する
    int width = conv<int>(in.args[0]);
    int height = conv<int>(in.args[1]);

    // イメージ ID を生成する
    GENERATE_IMAGE_INDEX(id);

    // 新しく画像を作成
    std::unique_ptr<image> img(new image(width, height));

    // リストに追加する
    images.insert(std::make_pair(id, std::move(img)));

    // イメージ ID を返す
    out.result = conv<string_t>(id);

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// 画像ファイルを読み込む
DEFINE_SAORI_FUNCTION(load)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(1);

    // イメージ ID を生成する
    GENERATE_IMAGE_INDEX(id);

    for (auto it = in.args.cbegin(); it != in.args.cend(); ++it)
    {
        // 新しく画像を作成
        std::unique_ptr<image> img(new image());

        // ファイルを読み込む
        if (!png_load_image(*it, *img))
        {
            return SAORIRESULT_BAD_REQUEST;
        }

        // リストに追加する
        images.insert(std::make_pair(id, std::move(img)));
    }

    // イメージ ID を返す
    out.result = conv<string_t>(id);

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// 画像ファイルとして保存する
DEFINE_SAORI_FUNCTION(save)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(2);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    int id = 1;

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // 保存する画像を取得
        image &img = *it->second;

        // ファイルに書き込む
        if (!png_save_image(in.args[id], img))
        {
            return SAORIRESULT_BAD_REQUEST;
        }

        id += 1;
    }

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// 保持している画像を全て破棄する
DEFINE_SAORI_FUNCTION(clear)
{
    // イメージをすべて開放する
    images.clear();

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// 画像を描画する
DEFINE_SAORI_FUNCTION(draw)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(4);

    // イメージのインデックスを取得する
    int base_index = conv<int>(in.args[0]);
    int elem_index = conv<int>(in.args[1]);

    // インデックスを確認する
    VERIFY_IMAGE_INDEX(base_index);
    VERIFY_IMAGE_INDEX(elem_index);

    // イメージを取得する
    image &base = *images.lower_bound(base_index)->second;
    image &elem = *images.lower_bound(elem_index)->second;

    // 追加パラメータを取得する
    int x = conv<int>(in.args[2]);
    int y = conv<int>(in.args[3]);
    int opacity = CHECK_ARGUMENT(5) ? conv<int>(in.args[4]) : 100;

    // 描画する
    if (!draw_image(base, elem, x, y, opacity))
    {
        return SAORIRESULT_BAD_REQUEST;
    }

    // 200 OK を返す
    return SAORIRESULT_OK;
}

DEFINE_SAORI_FUNCTION(fill)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(2);

    // イメージのインデックスを取得する
    int index = conv<int>(in.args[0]);

    // インデックスを確認する
    VERIFY_IMAGE_INDEX(index);

    // 追加パラメータを取得する
    color fill_color(conv<color::value_type>(in.args[1]));

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // イメージを取得する
        image &img = *it->second;

        // 塗りつぶす
        fill_image(img, fill_color);
    }

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// ピクセルを取得する
DEFINE_SAORI_FUNCTION(pixel)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(3);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // イメージを取得する
    image &img = *images.lower_bound(index)->second;

    // パラメータを取得する
    int x = conv<int>(in.args[1]);
    int y = conv<int>(in.args[2]);

    // 引数の数によって挙動が変わる
    if (CHECK_ARGUMENT(3))
    {
        // 指定された座標のピクセル値を取得し、返す
        out.result = conv<string_t>(img.pixel(x, y).to_rgb());
    }
    else
    {
        // 指定された座標のピクセル値を変更する
        img.pixel(color(conv<color::value_type>(in.args[3])), x, y);
    }

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// 特定の色を新しい色で塗りなおす
DEFINE_SAORI_FUNCTION(repaint)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(3);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // 変換前、変換後の色を取得する
    color before(conv<color::value_type>(in.args[1]));
    color after(conv<color::value_type>(in.args[2]));

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // イメージを取得する
        image &img = *it->second;

        // repaint_function 処理を実行する
        img.transform(repaint_function(before, after));
    }

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// 色合いを変更する
DEFINE_SAORI_FUNCTION(tone)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(4);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // 追加パラメータを取得する
    int red = conv<int>(in.args[1]);
    int green = conv<int>(in.args[2]);
    int blue = conv<int>(in.args[3]);

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // イメージを取得する
        image &img = *it->second;

        // tone_function 処理を行う
        img.transform(tone_function(red, green, blue));
    }

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// 画像を一部分だけ切り出す
DEFINE_SAORI_FUNCTION(cut)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(5);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // イメージを取得する
    image &src = *images.lower_bound(index)->second;

    // 追加パラメータを取得
    int x = conv<int>(in.args[1]);
    int y = conv<int>(in.args[2]);
    int width = conv<int>(in.args[3]);
    int height = conv<int>(in.args[4]);

    // イメージ ID を生成する
    GENERATE_IMAGE_INDEX(id);

    // 新しくイメージを作成する
    std::unique_ptr<image> dst(new image());

    // 部分画像を取得する
    if (!src.sub_image(*dst, x, y, width, height))
    {
        return SAORIRESULT_BAD_REQUEST;
    }

    // リストに追加する
    images.insert(std::make_pair(id, std::move(dst)));

    // イメージ ID を返す
    out.result = conv<string_t>(images.size());

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// リサイズ
DEFINE_SAORI_FUNCTION(resize)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(3);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // イメージを取得する
    image &src = *images.lower_bound(index)->second;

    int width;
    int height;

    // 追加パラメータを取得
    const string_t &method = in.args[1];

    // 引数の数によって挙動が変わる
    if (CHECK_ARGUMENT(3))
    {
        // スケールが指定された
        double scale = conv<double>(in.args[2]);

        // スケールを元にサイズを決定
        width = static_cast<int>(src.width() * scale / 100.0);
        height = static_cast<int>(src.height() * scale / 100.0);
    }
    else
    {
        // リサイズ後のサイズが指定されている
        width = conv<int>(in.args[2]);
        height = conv<int>(in.args[3]);

        // 片方だけ固定の場合は、比率を保ったままリサイズ
        if (width == 0)
        {
            width = static_cast<int>(src.width() * (static_cast<double>(height) / src.height()));
        }
        else if (height == 0)
        {
            height = static_cast<int>(src.height() * (static_cast<double>(width) / src.width()));
        }
    }

    // サイズを確認する
    if (width <= 0 || height <= 0)
    {
        return SAORIRESULT_BAD_REQUEST;
    }

    // イメージ ID を生成する
    GENERATE_IMAGE_INDEX(id);

    // 新しくイメージを作成し、リストに追加する
    std::unique_ptr<image> dst(new image(width, height));

    if (src.width() == width && src.height() == height)
    {
        memcpy(dst->buffer(), src.buffer(), width * height * sizeof(color));
    }
    else
    {
        // リサイズ画像を取得する
        if (method == _T("ssp") || method == _T("nearest_neighbor"))
        {
            // ニアレストネイバー
            src.resize(*dst, nearest_neighbor_sampler());
        }
        else if (method == _T("fast") || method == _T("bilinear"))
        {
            // バイリニア
            src.resize(*dst, bilinear_sampler());
        }
        else if (method == _T("quality") || method == _T("bicubic"))
        {
            // バイキュービック
            src.resize(*dst, bicubic_sampler());
        }
        else if (method == _T("lanczos2"))
        {
            // Lanczos-2
            src.resize(*dst, lanczos2_sampler());
        }
        else if (method == _T("lanczos3"))
        {
            // Lanczos-3
            src.resize(*dst, lanczos3_sampler());
        }
        else if (method == _T("lanczos4"))
        {
            // Lanczos-4
            src.resize(*dst, lanczos4_sampler());
        }
        else
        {
            return SAORIRESULT_BAD_REQUEST;
        }
    }

    images.insert(std::make_pair(id, std::move(dst)));

    // イメージ ID を返す
    out.result = conv<string_t>(images.size());

    // 200 OK を返す
    return SAORIRESULT_OK;
}

// サイズ取得
DEFINE_SAORI_FUNCTION(size)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(1);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // イメージを取得する
    const image &img = *images.lower_bound(index)->second;

    // イメージ ID を返す
    out.result = conv<string_t>(index);

    // 追加情報として幅と高さを返す
    out.values.push_back(conv<string_t>(img.width()));
    out.values.push_back(conv<string_t>(img.height()));

    // 200 OK を返す
    return SAORIRESULT_OK;
}

DEFINE_SAORI_FUNCTION(rotate)
{
    return SAORIRESULT_INTERNAL_SERVER_ERROR;
}

DEFINE_SAORI_FUNCTION(opacity)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(2);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // 透明度を取得する
    int opacity = conv<int>(in.args[1]);

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // イメージを取得する
        image &img = *it->second;

        // opacity_function 処理を実行する
        img.transform(opacity_function(opacity));
    }

    // 200 OK を返す
    return SAORIRESULT_OK;
}

DEFINE_SAORI_FUNCTION(dup)
{
    // 引数の個数を確認
    VERIFY_ARGUMENT(1);

    // イメージのインデックスを取得
    int index = conv<int>(in.args[0]);

    // インデックスを検証
    VERIFY_IMAGE_INDEX(index);

    // イメージを取得する
    image &img = *images.lower_bound(index)->second;

    // 画像のサイズを取得
    int width = img.width();
    int height = img.height();

    // イメージ ID を生成する
    GENERATE_IMAGE_INDEX(id);

    // 新しく画像を作成
    std::unique_ptr<image> newimg(new image(width, height));

    // 画像をコピーする
    memcpy(newimg->buffer(), img.buffer(), width * height * sizeof(color));

    // リストに追加する
    images.insert(std::make_pair(id, std::move(newimg)));

    // イメージ ID を返す
    out.result = conv<string_t>(images.size());

    // 200 OK を返す
    return SAORIRESULT_OK;
}

bool saori::load()
{
    // SAORI 関数を登録する
    REGISTER_SAORI_FUNCTION(new);
    REGISTER_SAORI_FUNCTION(load);
    REGISTER_SAORI_FUNCTION(save);
    REGISTER_SAORI_FUNCTION(clear);
    REGISTER_SAORI_FUNCTION(draw);
    REGISTER_SAORI_FUNCTION(fill);
    REGISTER_SAORI_FUNCTION(pixel);
    REGISTER_SAORI_FUNCTION(repaint);
    REGISTER_SAORI_FUNCTION(tone);
    REGISTER_SAORI_FUNCTION(cut);
    REGISTER_SAORI_FUNCTION(resize);
    REGISTER_SAORI_FUNCTION(size);
    REGISTER_SAORI_FUNCTION(rotate);
    REGISTER_SAORI_FUNCTION(opacity);
    REGISTER_SAORI_FUNCTION(dup);
    return true;
}

bool saori::unload()
{
    return true;
}