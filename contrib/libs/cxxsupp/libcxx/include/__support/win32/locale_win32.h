// -*- C++ -*-
//===-----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP_SUPPORT_WIN32_LOCALE_WIN32_H
#define _LIBCPP_SUPPORT_WIN32_LOCALE_WIN32_H

#include <__config>
#include <__nullptr>
#include <locale.h> // _locale_t
#include <stdio.h>

#define _X_ALL LC_ALL
#define _X_COLLATE LC_COLLATE
#define _X_CTYPE LC_CTYPE
#define _X_MONETARY LC_MONETARY
#define _X_NUMERIC LC_NUMERIC
#define _X_TIME LC_TIME
#define _X_MAX LC_MAX
#define _X_MESSAGES 6
#define _NCAT (_X_MESSAGES + 1)

#define _CATMASK(n) ((1 << (n)) >> 1)
#define _M_COLLATE _CATMASK(_X_COLLATE)
#define _M_CTYPE _CATMASK(_X_CTYPE)
#define _M_MONETARY _CATMASK(_X_MONETARY)
#define _M_NUMERIC _CATMASK(_X_NUMERIC)
#define _M_TIME _CATMASK(_X_TIME)
#define _M_MESSAGES _CATMASK(_X_MESSAGES)
#define _M_ALL (_CATMASK(_NCAT) - 1)

#define LC_COLLATE_MASK _M_COLLATE
#define LC_CTYPE_MASK _M_CTYPE
#define LC_MONETARY_MASK _M_MONETARY
#define LC_NUMERIC_MASK _M_NUMERIC
#define LC_TIME_MASK _M_TIME
#define LC_MESSAGES_MASK _M_MESSAGES
#define LC_MESSAGES _X_MESSAGES
#define LC_ALL_MASK _M_ALL

class __lconv_storage {
public:
    __lconv_storage(const lconv *__lc_input) {
        __lc = *__lc_input;

        __decimal_point = __lc_input->decimal_point;
        __thousands_sep = __lc_input->thousands_sep;
        __grouping = __lc_input->grouping;
        __int_curr_symbol = __lc_input->int_curr_symbol;
        __currency_symbol = __lc_input->currency_symbol;
        __mon_decimal_point = __lc_input->mon_decimal_point;
        __mon_thousands_sep = __lc_input->mon_thousands_sep;
        __mon_grouping = __lc_input->mon_grouping;
        __positive_sign = __lc_input->positive_sign;
        __negative_sign = __lc_input->negative_sign;

        __lc.decimal_point = const_cast<char *>(__decimal_point.c_str());
        __lc.thousands_sep = const_cast<char *>(__thousands_sep.c_str());
        __lc.grouping = const_cast<char *>(__grouping.c_str());
        __lc.int_curr_symbol = const_cast<char *>(__int_curr_symbol.c_str());
        __lc.currency_symbol = const_cast<char *>(__currency_symbol.c_str());
        __lc.mon_decimal_point = const_cast<char *>(__mon_decimal_point.c_str());
        __lc.mon_thousands_sep = const_cast<char *>(__mon_thousands_sep.c_str());
        __lc.mon_grouping = const_cast<char *>(__mon_grouping.c_str());
        __lc.positive_sign = const_cast<char *>(__positive_sign.c_str());
        __lc.negative_sign = const_cast<char *>(__negative_sign.c_str());
    }

    lconv *__get() {
        return &__lc;
    }
private:
    lconv __lc;
    std::string __decimal_point;
    std::string __thousands_sep;
    std::string __grouping;
    std::string __int_curr_symbol;
    std::string __currency_symbol;
    std::string __mon_decimal_point;
    std::string __mon_thousands_sep;
    std::string __mon_grouping;
    std::string __positive_sign;
    std::string __negative_sign;
};

class locale_t {
public:
    locale_t()
        : __locale(nullptr), __locale_str(nullptr), __lc(nullptr) {}
    locale_t(std::nullptr_t)
        : __locale(nullptr), __locale_str(nullptr), __lc(nullptr) {}
    locale_t(_locale_t __xlocale, const char* __xlocale_str)
        : __locale(__xlocale), __locale_str(__xlocale_str), __lc(nullptr) {}
    locale_t(const locale_t &__l)
        : __locale(__l.__locale), __locale_str(__l.__locale_str), __lc(nullptr) {}

    ~locale_t() {
        delete __lc;
    }

    locale_t &operator =(const locale_t &__l) {
        __locale = __l.__locale;
        __locale_str = __l.__locale_str;
        // __lc not copied
        return *this;
    }

    friend bool operator==(const locale_t& __left, const locale_t& __right) {
        return __left.__locale == __right.__locale;
    }

    friend bool operator==(const locale_t& __left, int __right) {
        return __left.__locale == nullptr && __right == 0;
    }

    friend bool operator==(const locale_t& __left, long long __right) {
        return __left.__locale == nullptr && __right == 0;
    }

    friend bool operator==(const locale_t& __left, std::nullptr_t) {
        return __left.__locale == nullptr;
    }

    friend bool operator==(int __left, const locale_t& __right) {
        return __left == 0 && nullptr == __right.__locale;
    }

    friend bool operator==(std::nullptr_t, const locale_t& __right) {
        return nullptr == __right.__locale;
    }

    friend bool operator!=(const locale_t& __left, const locale_t& __right) {
        return !(__left == __right);
    }

    friend bool operator!=(const locale_t& __left, int __right) {
        return !(__left == __right);
    }

    friend bool operator!=(const locale_t& __left, long long __right) {
        return !(__left == __right);
    }

    friend bool operator!=(const locale_t& __left, std::nullptr_t __right) {
        return !(__left == __right);
    }

    friend bool operator!=(int __left, const locale_t& __right) {
        return !(__left == __right);
    }

    friend bool operator!=(std::nullptr_t __left, const locale_t& __right) {
        return !(__left == __right);
    }

    operator bool() const {
        return __locale != nullptr;
    }

    const char* __get_locale() const { return __locale_str; }

    operator _locale_t() const {
        return __locale;
    }

    lconv *__store_lconv(const lconv *__input_lc) {
        delete __lc;
        __lc = new __lconv_storage(__input_lc);
        return __lc->__get();
    }
private:
    _locale_t __locale;
    const char* __locale_str;
    __lconv_storage *__lc = nullptr;
};

// Locale management functions
#define freelocale _free_locale
// FIXME: base currently unused. Needs manual work to construct the new locale
locale_t newlocale( int mask, const char * locale, locale_t base );
// uselocale can't be implemented on Windows because Windows allows partial modification
// of thread-local locale and so _get_current_locale() returns a copy while uselocale does
// not create any copies.
// We can still implement raii even without uselocale though.


lconv *localeconv_l( locale_t &__loc );
size_t mbrlen_l( const char *__restrict __s, size_t __n,
                 mbstate_t *__restrict __ps, locale_t __loc);
size_t mbsrtowcs_l( wchar_t *__restrict __dst, const char **__restrict __src,
                    size_t __len, mbstate_t *__restrict __ps, locale_t __loc );
size_t wcrtomb_l( char *__restrict s, wchar_t __wc, mbstate_t *__restrict __ps,
                  locale_t __loc);
size_t mbrtowc_l( wchar_t *__restrict __pwc, const char *__restrict __s,
                  size_t __n, mbstate_t *__restrict __ps, locale_t __loc);
size_t mbsnrtowcs_l( wchar_t *__restrict __dst, const char **__restrict __src,
                     size_t __nms, size_t __len, mbstate_t *__restrict __ps, locale_t __loc);
size_t wcsnrtombs_l( char *__restrict __dst, const wchar_t **__restrict __src,
                     size_t __nwc, size_t __len, mbstate_t *__restrict __ps, locale_t __loc);
wint_t btowc_l( int __c, locale_t __loc );
int wctob_l( wint_t __c, locale_t __loc );

decltype(MB_CUR_MAX) MB_CUR_MAX_L( locale_t __l );

// the *_l functions are prefixed on Windows, only available for msvcr80+, VS2005+
#define mbtowc_l _mbtowc_l
#define strtoll_l _strtoi64_l
#define strtoull_l _strtoui64_l
#define strtod_l _strtod_l
#if defined(_LIBCPP_MSVCRT)
#define strtof_l _strtof_l
#define strtold_l _strtod_l
#else
_LIBCPP_FUNC_VIS float strtof_l(const char*, char**, locale_t);
_LIBCPP_FUNC_VIS long double strtold_l(const char*, char**, locale_t);
#endif
#define isupper_l _isupper_l
#define islower_l _islower_l
#define isblank_l _isblank_l
#define isdigit_l _isdigit_l
#define isxdigit_l _isxdigit_l
#define strcoll_l _strcoll_l
#define strxfrm_l _strxfrm_l
#define wcscoll_l _wcscoll_l
#define wcsxfrm_l _wcsxfrm_l
#define toupper_l _toupper_l
#define tolower_l _tolower_l
#define iswspace_l _iswspace_l
#define iswprint_l _iswprint_l
#define iswcntrl_l _iswcntrl_l
#define iswupper_l _iswupper_l
#define iswlower_l _iswlower_l
#define iswblank_l _iswblank_l
#define iswalpha_l _iswalpha_l
#define iswdigit_l _iswdigit_l
#define iswpunct_l _iswpunct_l
#define iswxdigit_l _iswxdigit_l
#define towupper_l _towupper_l
#define towlower_l _towlower_l
#if defined(__MINGW32__) && __MSVCRT_VERSION__ < 0x0800
_LIBCPP_FUNC_VIS size_t strftime_l(char *ret, size_t n, const char *format,
                                   const struct tm *tm, locale_t loc);
#else
#define strftime_l _strftime_l
#endif
#define sscanf_l( __s, __l, __f, ...) _sscanf_l( __s, __f, __l, ##__VA_ARGS__ )
#define sprintf_l( __s, __l, __f, ... ) _sprintf_l( __s, __f, __l, ##__VA_ARGS__ )
#define vsprintf_l( __s, __l, __f, ... ) _vsprintf_l( __s, __f, __l, ##__VA_ARGS__ )
#define vsnprintf_l( __s, __n, __l, __f, ... ) _vsnprintf_l( __s, __n, __f, __l, ##__VA_ARGS__ )
#define snprintf_l(__s, __n, __l, __f, ...) _snprintf_l( __s, __n, __f, __l, ##__VA_ARGS__ )
_LIBCPP_FUNC_VIS int asprintf_l( char **__ret, locale_t __loc, const char *__format, ... );
_LIBCPP_FUNC_VIS int vasprintf_l( char **__ret, locale_t __loc, const char *__format, va_list __ap );

#endif // _LIBCPP_SUPPORT_WIN32_LOCALE_WIN32_H
