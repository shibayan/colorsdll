/*
	saori.h
    Multi Platform SAORI Framework
*/

#pragma once

#include <string>
#include <vector>
#include <map>
#include <locale>
#include <sstream>
#include <memory>
#include <type_traits>
#include <functional>
#include <algorithm>

#include <boost/lexical_cast.hpp>

// プラットフォーム判定
#ifdef _WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif /* _DEBUG */

#define SAORIAPI extern "C" __declspec(dllexport)
#define SAORICALL __cdecl

// Windows では強制的に Unicode を使う
#define _SAORI_UNICODE

// メモリアロケータの抽象化
#define SAORI_ALLOC(size) GlobalAlloc(GMEM_FIXED, size)
#define SAORI_FREE(handle) GlobalFree(handle)

#else

#define SAORIAPI extern "C"
#define SAORICALL

// メモリアロケータの抽象化
#define SAORI_ALLOC(size) malloc(size)
#define SAORI_FREE(handle) free(handle)

#endif /* _WINDOWS */

// 内部を全て Unicode で処理するか判定
#ifdef _SAORI_UNICODE

#define _T(text) L##text

typedef wchar_t char_t;

#ifdef _MSC_VER

// VC は Unicode 系の関数名が特殊
#define wtoi _wtoi

#define tfopen_s _wfopen_s

#endif /* _MSC_VER */

#else

#define _T(text) text

typedef char char_t;

#define tfopen_s fopen_s

#endif /* _SAORI_UNICODE */

// SAORI バージョン文字列
#define SAORI_VERSIONSTRING_10 _T("SAORI/1.0")
#define SAORI_VERSIONSTRING SAORI_VERSIONSTRING_10

// SAORI ヘルパーマクロ
#define DEFINE_SAORI_FUNCTION(name) SAORIResult saori_function_##name(const saori_input &in, saori_output &out)
#define REGISTER_SAORI_FUNCTION(name) functions[_T(#name)] = saori_function_##name

#define VERIFY_ARGUMENT(min) if (in.args.size() < min) { return SAORIRESULT_BAD_REQUEST; }
#define VERIFY_ARGUMENT_RANGE(min, max) if (in.args.size() < min || in.args.size() > max) { return SAORIRESULT_BAD_REQUEST; }
#define CHECK_ARGUMENT(count) in.args.size() == count

// エクスポート関数定義
SAORIAPI int SAORICALL load(void *h, long len);
SAORIAPI int SAORICALL unload();
SAORIAPI void * SAORICALL request(void *h, long *len);

typedef std::basic_string<char_t> string_t;
typedef std::basic_istringstream<char_t> istringstream_t;
typedef std::basic_ostringstream<char_t> ostringstream_t;

typedef enum
{
	SAORIRESULT_OK = 200,
	SAORIRESULT_NO_CONTENT = 204,
	SAORIRESULT_BAD_REQUEST = 400,
	SAORIRESULT_INTERNAL_SERVER_ERROR = 500,
} SAORIResult;

typedef enum
{
	SAORICHARSET_SHIFT_JIS = 932,
	SAORICHARSET_UTF_8 = 65001,
	SAORICHARSET_EUC_JP = 20932,
	SAORICHARSET_ISO_2022_JP = 50220,
} SAORICharset;

class saori_input;
class saori_output;

// SAORI 関数
typedef std::function<SAORIResult (const saori_input &, saori_output &)> saori_function;

// SAORI リクエスト
class saori_input
{
public:
    saori_input(string_t req) { deserialize(req); }
	bool deserialize(string_t req);
public:
	SAORICharset charset;
	string_t command;
    string_t function;
	std::vector<string_t> args;
	std::map<string_t, string_t> opts;
};

// SAORI レスポンス
class saori_output
{
public:
	string_t serialize();
public:
	SAORICharset charset;
	SAORIResult result_code;
	string_t result;
	std::vector<string_t> values;
	std::map<string_t, string_t> opts;
};

// SAORI メイン
class saori
{
public:
	saori(const std::string &path)
	{
#ifdef _SAORI_UNICODE
		setlocale(LC_ALL, "Japanese");
		saori_path = to_unicode(SAORICHARSET_SHIFT_JIS, path);
#else
		saori_path = path;
#endif /* _SAORI_UNICODE */
	}
	// 実装すべき関数
	bool load();
	bool unload();
    // リクエスト関数
	std::string request(std::string req);
private:
	string_t saori_path;
    std::map<string_t, saori_function> functions;
public:
	static string_t from_result(SAORIResult result);
	static string_t from_charset(SAORICharset charset);
	static SAORICharset to_charset(const std::string &str);
#ifdef _SAORI_UNICODE
	static std::wstring to_unicode(SAORICharset charset, const std::string &str);
	static std::string from_unicode(SAORICharset charset, const std::wstring &wstr);
#endif /* _SAORI_UNICODE */
};

// 自動型変換 + 文字コード変換ヘルパー
namespace conv_helper
{
    template<class Target, class Source, bool IsConvertible>
    struct conv_op
    {
        Target operator()(const Source &src) const
        {
			return boost::lexical_cast<Target>(src);
        }
    };

    template<class Target, class Source>
    struct conv_op<Target, Source, true>
    {
        Target operator()(const Source &src) const
        {
            return static_cast<Target>(src);
        }
    };

    // char はそのままでは処理できないので対処
    template<class Source>
    struct conv_op<char, Source, false>
    {
        char operator()(const Source &src) const
        {
        	try
        	{
            	return static_cast<char>(boost::lexical_cast<int>(src));
            }
            catch (...)
            {
            	return 0;
            }
        }
    };

    // 文字コードがらみは特殊化で対処
    template<>
    struct conv_op<std::string, std::wstring, false>
    {
        std::string operator()(const std::wstring &src) const
        {
            return saori::from_unicode(SAORICHARSET_SHIFT_JIS, src);
        }
    };

    template<>
    struct conv_op<std::wstring, std::string, false>
    {
        std::wstring operator()(const std::string &src) const
        {
            return saori::to_unicode(SAORICHARSET_SHIFT_JIS, src);
        }
    };
}

template<class Target, class Source>
Target conv(const Source &src)
{
    return conv_helper::conv_op<Target, Source, std::is_convertible<Source, Target>::value>()(src);
}