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

// �S�Ẳ摜��ێ�����R���e�i
static image_multimap images;

#define GENERATE_IMAGE_INDEX(id) int id = static_cast<int>(images.size() + 1)
#define VERIFY_IMAGE_INDEX(index) if (index < 1 || static_cast<int>(images.size()) < index) { return SAORIRESULT_BAD_REQUEST; }

// �V�����摜���쐬����
DEFINE_SAORI_FUNCTION(new)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(2);

    // �ǉ��p�����[�^���擾����
    int width = conv<int>(in.args[0]);
    int height = conv<int>(in.args[1]);

    // �C���[�W ID �𐶐�����
    GENERATE_IMAGE_INDEX(id);

    // �V�����摜���쐬
    std::unique_ptr<image> img(new image(width, height));

    // ���X�g�ɒǉ�����
    images.insert(std::make_pair(id, std::move(img)));

    // �C���[�W ID ��Ԃ�
    out.result = conv<string_t>(id);

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �摜�t�@�C����ǂݍ���
DEFINE_SAORI_FUNCTION(load)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(1);

    // �C���[�W ID �𐶐�����
    GENERATE_IMAGE_INDEX(id);

    for (auto it = in.args.cbegin(); it != in.args.cend(); ++it)
    {
        // �V�����摜���쐬
        std::unique_ptr<image> img(new image());

        // �t�@�C����ǂݍ���
        if (!png_load_image(*it, *img))
        {
            return SAORIRESULT_BAD_REQUEST;
        }

        // ���X�g�ɒǉ�����
        images.insert(std::make_pair(id, std::move(img)));
    }

    // �C���[�W ID ��Ԃ�
    out.result = conv<string_t>(id);

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �摜�t�@�C���Ƃ��ĕۑ�����
DEFINE_SAORI_FUNCTION(save)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(2);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    int id = 1;

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // �ۑ�����摜���擾
        image &img = *it->second;

        // �t�@�C���ɏ�������
        if (!png_save_image(in.args[id], img))
        {
            return SAORIRESULT_BAD_REQUEST;
        }

        id += 1;
    }

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �ێ����Ă���摜��S�Ĕj������
DEFINE_SAORI_FUNCTION(clear)
{
    // �C���[�W�����ׂĊJ������
    images.clear();

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �摜��`�悷��
DEFINE_SAORI_FUNCTION(draw)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(4);

    // �C���[�W�̃C���f�b�N�X���擾����
    int base_index = conv<int>(in.args[0]);
    int elem_index = conv<int>(in.args[1]);

    // �C���f�b�N�X���m�F����
    VERIFY_IMAGE_INDEX(base_index);
    VERIFY_IMAGE_INDEX(elem_index);

    // �C���[�W���擾����
    image &base = *images.lower_bound(base_index)->second;
    image &elem = *images.lower_bound(elem_index)->second;

    // �ǉ��p�����[�^���擾����
    int x = conv<int>(in.args[2]);
    int y = conv<int>(in.args[3]);
    int opacity = CHECK_ARGUMENT(5) ? conv<int>(in.args[4]) : 100;

    // �`�悷��
    if (!draw_image(base, elem, x, y, opacity))
    {
        return SAORIRESULT_BAD_REQUEST;
    }

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

DEFINE_SAORI_FUNCTION(fill)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(2);

    // �C���[�W�̃C���f�b�N�X���擾����
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X���m�F����
    VERIFY_IMAGE_INDEX(index);

    // �ǉ��p�����[�^���擾����
    color fill_color(conv<color::value_type>(in.args[1]));

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // �C���[�W���擾����
        image &img = *it->second;

        // �h��Ԃ�
        fill_image(img, fill_color);
    }

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �s�N�Z�����擾����
DEFINE_SAORI_FUNCTION(pixel)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(3);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �C���[�W���擾����
    image &img = *images.lower_bound(index)->second;

    // �p�����[�^���擾����
    int x = conv<int>(in.args[1]);
    int y = conv<int>(in.args[2]);

    // �����̐��ɂ���ċ������ς��
    if (CHECK_ARGUMENT(3))
    {
        // �w�肳�ꂽ���W�̃s�N�Z���l���擾���A�Ԃ�
        out.result = conv<string_t>(img.pixel(x, y).to_rgb());
    }
    else
    {
        // �w�肳�ꂽ���W�̃s�N�Z���l��ύX����
        img.pixel(color(conv<color::value_type>(in.args[3])), x, y);
    }

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// ����̐F��V�����F�œh��Ȃ���
DEFINE_SAORI_FUNCTION(repaint)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(3);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �ϊ��O�A�ϊ���̐F���擾����
    color before(conv<color::value_type>(in.args[1]));
    color after(conv<color::value_type>(in.args[2]));

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // �C���[�W���擾����
        image &img = *it->second;

        // repaint_function ���������s����
        img.transform(repaint_function(before, after));
    }

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �F������ύX����
DEFINE_SAORI_FUNCTION(tone)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(4);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �ǉ��p�����[�^���擾����
    int red = conv<int>(in.args[1]);
    int green = conv<int>(in.args[2]);
    int blue = conv<int>(in.args[3]);

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // �C���[�W���擾����
        image &img = *it->second;

        // tone_function �������s��
        img.transform(tone_function(red, green, blue));
    }

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �摜���ꕔ�������؂�o��
DEFINE_SAORI_FUNCTION(cut)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(5);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �C���[�W���擾����
    image &src = *images.lower_bound(index)->second;

    // �ǉ��p�����[�^���擾
    int x = conv<int>(in.args[1]);
    int y = conv<int>(in.args[2]);
    int width = conv<int>(in.args[3]);
    int height = conv<int>(in.args[4]);

    // �C���[�W ID �𐶐�����
    GENERATE_IMAGE_INDEX(id);

    // �V�����C���[�W���쐬����
    std::unique_ptr<image> dst(new image());

    // �����摜���擾����
    if (!src.sub_image(*dst, x, y, width, height))
    {
        return SAORIRESULT_BAD_REQUEST;
    }

    // ���X�g�ɒǉ�����
    images.insert(std::make_pair(id, std::move(dst)));

    // �C���[�W ID ��Ԃ�
    out.result = conv<string_t>(images.size());

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// ���T�C�Y
DEFINE_SAORI_FUNCTION(resize)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(3);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �C���[�W���擾����
    image &src = *images.lower_bound(index)->second;

    int width;
    int height;

    // �ǉ��p�����[�^���擾
    const string_t &method = in.args[1];

    // �����̐��ɂ���ċ������ς��
    if (CHECK_ARGUMENT(3))
    {
        // �X�P�[�����w�肳�ꂽ
        double scale = conv<double>(in.args[2]);

        // �X�P�[�������ɃT�C�Y������
        width = static_cast<int>(src.width() * scale / 100.0);
        height = static_cast<int>(src.height() * scale / 100.0);
    }
    else
    {
        // ���T�C�Y��̃T�C�Y���w�肳��Ă���
        width = conv<int>(in.args[2]);
        height = conv<int>(in.args[3]);

        // �Е������Œ�̏ꍇ�́A�䗦��ۂ����܂܃��T�C�Y
        if (width == 0)
        {
            width = static_cast<int>(src.width() * (static_cast<double>(height) / src.height()));
        }
        else if (height == 0)
        {
            height = static_cast<int>(src.height() * (static_cast<double>(width) / src.width()));
        }
    }

    // �T�C�Y���m�F����
    if (width <= 0 || height <= 0)
    {
        return SAORIRESULT_BAD_REQUEST;
    }

    // �C���[�W ID �𐶐�����
    GENERATE_IMAGE_INDEX(id);

    // �V�����C���[�W���쐬���A���X�g�ɒǉ�����
    std::unique_ptr<image> dst(new image(width, height));

    if (src.width() == width && src.height() == height)
    {
        memcpy(dst->buffer(), src.buffer(), width * height * sizeof(color));
    }
    else
    {
        // ���T�C�Y�摜���擾����
        if (method == _T("ssp") || method == _T("nearest_neighbor"))
        {
            // �j�A���X�g�l�C�o�[
            src.resize(*dst, nearest_neighbor_sampler());
        }
        else if (method == _T("fast") || method == _T("bilinear"))
        {
            // �o�C���j�A
            src.resize(*dst, bilinear_sampler());
        }
        else if (method == _T("quality") || method == _T("bicubic"))
        {
            // �o�C�L���[�r�b�N
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

    // �C���[�W ID ��Ԃ�
    out.result = conv<string_t>(images.size());

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

// �T�C�Y�擾
DEFINE_SAORI_FUNCTION(size)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(1);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �C���[�W���擾����
    const image &img = *images.lower_bound(index)->second;

    // �C���[�W ID ��Ԃ�
    out.result = conv<string_t>(index);

    // �ǉ����Ƃ��ĕ��ƍ�����Ԃ�
    out.values.push_back(conv<string_t>(img.width()));
    out.values.push_back(conv<string_t>(img.height()));

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

DEFINE_SAORI_FUNCTION(rotate)
{
    return SAORIRESULT_INTERNAL_SERVER_ERROR;
}

DEFINE_SAORI_FUNCTION(opacity)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(2);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �����x���擾����
    int opacity = conv<int>(in.args[1]);

    auto range = images.equal_range(index);

    for (auto it = range.first; it != range.second; ++it)
    {
        // �C���[�W���擾����
        image &img = *it->second;

        // opacity_function ���������s����
        img.transform(opacity_function(opacity));
    }

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

DEFINE_SAORI_FUNCTION(dup)
{
    // �����̌����m�F
    VERIFY_ARGUMENT(1);

    // �C���[�W�̃C���f�b�N�X���擾
    int index = conv<int>(in.args[0]);

    // �C���f�b�N�X������
    VERIFY_IMAGE_INDEX(index);

    // �C���[�W���擾����
    image &img = *images.lower_bound(index)->second;

    // �摜�̃T�C�Y���擾
    int width = img.width();
    int height = img.height();

    // �C���[�W ID �𐶐�����
    GENERATE_IMAGE_INDEX(id);

    // �V�����摜���쐬
    std::unique_ptr<image> newimg(new image(width, height));

    // �摜���R�s�[����
    memcpy(newimg->buffer(), img.buffer(), width * height * sizeof(color));

    // ���X�g�ɒǉ�����
    images.insert(std::make_pair(id, std::move(newimg)));

    // �C���[�W ID ��Ԃ�
    out.result = conv<string_t>(images.size());

    // 200 OK ��Ԃ�
    return SAORIRESULT_OK;
}

bool saori::load()
{
    // SAORI �֐���o�^����
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