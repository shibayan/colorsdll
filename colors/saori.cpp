/*
    saori.cpp
    Multi Platform SAORI Framework
*/

#include "saori.h"

// 唯一の SAORI インスタンス
static std::unique_ptr<saori> instance;

bool saori_input::deserialize(string_t req)
{
    istringstream_t stream(req);
    string_t line;

    std::getline(stream, line);
    string_t::size_type proto_pos = line.find(_T(" SAORI/1."));
    if (proto_pos == string_t::npos)
    {
        return false;
    }

    command = line.substr(0, proto_pos);

    while (std::getline(stream, line))
    {
        string_t::size_type pos = line.find(_T(": "));
        if (pos != string_t::npos)
        {
            string_t key = line.substr(0, pos);
            string_t value = line.substr(pos + 2, line.size() - pos - 3);

            if (key.compare(0, 8, _T("Argument")) == 0)
            {

#ifdef _SAORI_UNICODE
                unsigned int order = wtoi(key.c_str() + 8);
#else
                unsigned int order = atoi(key.c_str() + 8);
#endif /* _SAORI_UNICODE */

                if (order == 0 && key[8] == _T('0'))
                {
                    function = value;
                }
                else if (order > 0)
                {
                    if (args.size() < order)
                    {
                        args.resize(order);
                    }
                    args[order - 1] = value;
                }
            }
            else
            {
                if (pos > 0)
                {
                    opts[key] = value;
                }
            }
        }
    }
    return true;
}

string_t saori_output::serialize()
{
    string_t rc_str = saori::from_result(result_code);

    ostringstream_t res;

    res << SAORI_VERSIONSTRING _T(" ") << result_code << _T(" ") << rc_str << _T("\r\n");

    res << _T("Charset: ") << saori::from_charset(charset) << _T("\r\n");

    if (!result.empty())
    {
        res << _T("Result: ") << result << _T("\r\n");
    }

    if (!values.empty())
    {
        for (std::vector<string_t>::size_type i = 0; i < values.size(); ++i)
        {
            res << _T("Value") << i << _T(": ");

            string_t value = values[i];
            string_t::size_type pos = 0;

            while ((pos = value.find_first_of(_T("\r\n"), pos)) != string_t::npos)
            {
                value.replace(pos, 2, _T("\1"));
            }

            res << value << _T("\r\n");
        }
    }

    if (!opts.empty())
    {
        for (std::map<string_t, string_t>::const_iterator it = opts.begin(); it != opts.end(); ++it)
        {
            res << it->first << _T(": ") << it->second << _T("\r\n");
        }
    }

    res << _T("\r\n");

    return res.str();
}

std::string saori::request(std::string req)
{
    SAORICharset charset = SAORICHARSET_SHIFT_JIS;

#ifdef _SAORI_UNICODE
    std::string temp = req;
    std::transform(temp.begin(), temp.end(), temp.begin(), tolower);

    std::string::size_type char_pos = temp.find("\ncharset: ");

    if (char_pos != std::string::npos)
    {
        char_pos += 10;
        charset = to_charset(temp.substr(char_pos, temp.find("\r\n", char_pos) - char_pos));
    }

    string_t req_t = to_unicode(charset, req);
#else
    string_t req_t = req;
#endif /* _SAORI_UNICODE */

    saori_input in(req_t);
    in.charset = charset;
    in.opts[_T("SecurityLevel")] = _T("Local");

    saori_output out;
    out.charset = in.charset;
    out.result_code = SAORIRESULT_BAD_REQUEST;

    if (in.command == _T("GET Version"))
    {
        out.result_code = SAORIRESULT_OK;
    }
    else if (in.command == _T("EXECUTE"))
    {
        std::map<string_t, saori_function>::const_iterator it = functions.lower_bound(in.function);
        if (it != functions.end())
        {
            try
            {
                out.result_code = it->second(in, out);
            }
            catch (...)
            {
                out.result_code = SAORIRESULT_INTERNAL_SERVER_ERROR;
            }
        }
    }

    string_t res_t = out.serialize();

#ifdef _SAORI_UNICODE
    std::string res = from_unicode(out.charset, res_t);
#else
    std::string res = res_t;
#endif /* _SAORI_UNICODE */

    return res;
}

string_t saori::from_result(SAORIResult result)
{
    switch (result)
    {
        case SAORIRESULT_OK:
            return _T("OK");
        case SAORIRESULT_NO_CONTENT:
            return _T("No Content");
        case SAORIRESULT_BAD_REQUEST:
            return _T("Bad Request");
        case SAORIRESULT_INTERNAL_SERVER_ERROR:
            return _T("Internal Server Error");
    }
    return _T("unknown error");
}

string_t saori::from_charset(SAORICharset charset)
{
    switch (charset)
    {
        case SAORICHARSET_SHIFT_JIS:
            return _T("Shift_JIS");
        case SAORICHARSET_ISO_2022_JP:
            return _T("ISO-2022-JP");
        case SAORICHARSET_EUC_JP:
            return _T("EUC-JP");
        case SAORICHARSET_UTF_8:
            return _T("UTF-8");
    }
    return _T("unknown charset");
}

SAORICharset saori::to_charset(const std::string &str)
{
    if (str == "shift_jis" || str == "x-sjis")
    {
        return SAORICHARSET_SHIFT_JIS;
    }
    else if (str == "iso-2022-jp")
    {
        return SAORICHARSET_ISO_2022_JP;
    }
    else if (str == "euc-jp" || str == "x-euc-jp")
    {
        return SAORICHARSET_EUC_JP;
    }
    else if (str == "utf-8")
    {
        return SAORICHARSET_UTF_8;
    }
    return SAORICHARSET_SHIFT_JIS;
}

#ifdef _SAORI_UNICODE

std::wstring saori::to_unicode(SAORICharset charset, const std::string &str)
{
#ifdef _WINDOWS
    int length = MultiByteToWideChar(charset, 0, str.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wstr(length + 1);
    length = MultiByteToWideChar(charset, 0, str.c_str(), -1, &wstr[0], length + 1);
    return std::wstring(wstr.begin(), wstr.begin() + length);
#else
    // not implemented
    return std::wstring();
#endif /* _WINDOWS */
}

std::string saori::from_unicode(SAORICharset charset, const std::wstring &wstr)
{
#ifdef _WINDOWS
    int length = WideCharToMultiByte(charset, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::vector<char> str(length + 1);
    length = WideCharToMultiByte(charset, 0, wstr.c_str(), -1, &str[0], length + 1, NULL, NULL);
    return std::string(str.begin(), str.begin() + length);
#else
    // not implemented
    return std::string();
#endif /* _WINDOWS */
}

#endif /* _SAORI_UNICODE */

#ifdef _WINDOWS

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    // boost::scoped_ptr<T> を使ってるので何もしなくてもよい
    return TRUE;
}

#endif /* _WINDOWS */

SAORIAPI int SAORICALL load(void *h, long len)
{
    // インスタンスが既に作成されているか調べる
    if (instance)
    {
        // 作成されていればアンロード
        unload();
    }

    // ポインタが有効か調べる
    if (!h)
    {
        // NULL なら何もせずにリターン
        return 0;
    }

    // SAORI インスタンスを作成
    instance.reset(new saori(std::string(reinterpret_cast<char *>(h), len)));

    // メモリを開放
    SAORI_FREE(h);

    return instance->load();
}

SAORIAPI int SAORICALL unload()
{
    // インスタンスを作成済みか調べる
    if (!instance)
    {
        // 作成されていないので何もせずにリターン
        return 0;
    }

    // アンロードする
    int ret = instance->unload();

    // インスタンスを開放
    instance.reset();

    // 戻り値を返す
    return ret;
}

SAORIAPI void * SAORICALL request(void *h, long *len)
{
    std::string res = instance->request(std::string(reinterpret_cast<char *>(h), *len));

    SAORI_FREE(h);

    *len = static_cast<long>(res.size());
    h = SAORI_ALLOC(*len + 1);
    if (h)
    {
        memcpy(h, res.c_str(), *len + 1);
    }
    return h;
}