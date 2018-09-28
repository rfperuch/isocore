#ifndef ISOLARIO_PLUS_JSON_HPP_
#define ISOLARIO_PLUS_JSON_HPP_

#include <isolario/plus/restrict.hpp>

extern "C" {
#include <isolario/json.h>
}

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <utility>

namespace isolario {

    enum class json_err_t {
        success       = JSON_SUCCESS,
        incomplete    = JSON_INCOMPLETE,
        bad_syntax    = JSON_BAD_SYNTAX
    };

    // simple JSON encoder
    class json_enc {
    public:
        explicit json_enc(std::size_t n = JSON_BUFSIZ) : jsonp(jsonalloc(n))
        {
            if (!jsonp) {
                throw std::bad_alloc();
            }
        }

        json_enc(const json_enc& o) : jsonp(jsonalloc(o.jsonp->len))
        {
            std::copy_n(o.jsonp->text, o.jsonp->len, jsonp->text);
        }

        json_enc(json_enc&& o) noexcept : jsonp(std::exchange(o.jsonp, nullptr)) { assert(jsonp); }

        ~json_enc() { std::free(jsonp); }

        std::size_t size() const noexcept { return jsonp->len; }
        std::size_t capacity() const noexcept { return jsonp->cap; }

        void clear() noexcept
        {
            assert(jsonp);
            jsonclear(jsonp);
        }

        void ensure(std::size_t n)
        {
            assert(jsonp);
            if (!jsonensure(&jsonp, n)) {
                throw std::bad_alloc();
            }
        }

        void shrink() noexcept
        {
            assert(jsonp);
            void *new_jsonp = std::realloc(jsonp, jsonp->len + 1);
            if (new_jsonp) {
                jsonp = static_cast<json_t*>(new_jsonp);
            }
        }

        const char* text() const noexcept
        {
            assert(jsonp);

            return jsonp->text;
        }

        void new_object()
        {
            assert(jsonp);

            newjsonobj(&jsonp);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void new_array()
        {
            assert(jsonp);

            newjsonarr(&jsonp);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void new_field(const char *name)
        {
            assert(jsonp);

            newjsonfield(&jsonp, name);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void new_value(const char *string)
        {
            assert(jsonp);

            newjsonvals(&jsonp, string);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void new_value(unsigned long uval)
        {
            assert(jsonp);

            newjsonvalu(&jsonp, uval);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void new_value(long val)
        {
            assert(jsonp);

            newjsonvald(&jsonp, val);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void new_value(double val)
        {
            assert(jsonp);

            newjsonvalf(&jsonp, val);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void new_value(bool val)
        {
            assert(jsonp);

            newjsonvalb(&jsonp, val);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void close_array()
        {
            assert(jsonp);

            closejsonarr(&jsonp);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }
        void close_object()
        {
            assert(jsonp);

            closejsonobj(&jsonp);
            if (jsonerror(jsonp)) {
                throw std::bad_alloc();
            }
        }

        static json_err_t pretty_print(const char *text, json_enc& dest)
        {
            assert(dest.jsonp);

            int err = jsonprettyp(&dest.jsonp, text);
            if (err == JSON_NOMEM) {
                throw std::bad_alloc();
            }
            return json_err_t(err);
        }
    private:
        json_t* jsonp;
    };

    class json_tok {
    public:
        json_tok() noexcept { clear(); }

        json_tok(const json_tok& other) noexcept = default;
        json_tok& operator=(const json_tok& other) noexcept = default;

        bool is_valid() const noexcept   { return ctok.type != JSON_NONE; }
        bool is_boolean() const noexcept { return ctok.type == JSON_BOOL; }
        bool is_number() const noexcept  { return ctok.type == JSON_NUM; }
        bool is_object() const noexcept  { return ctok.type == JSON_OBJ; }
        bool is_array() const noexcept   { return ctok.type == JSON_ARR; }
        bool is_string() const noexcept  { return ctok.type == JSON_STR; }

        explicit operator bool() const noexcept { return is_valid(); }

        std::string text() const noexcept { assert(is_valid()); return std::string(ctok.start, ctok.end - ctok.start); }

        bool boolean_value() const noexcept { assert(is_boolean()); return ctok.boolval; }
        long long_value() const noexcept    { assert(is_number()); return ctok.numval; }
        double num_value() const noexcept   { assert(is_number()); return ctok.numval; }
        std::size_t length() const noexcept { assert(is_object() || is_array()); return ctok.size; }

        void clear() noexcept { std::memset(&ctok, 0, sizeof(ctok)); }

    private:
        jsontok_t ctok;

        friend class json_dec;
    };

    inline void swap(json_tok& a, json_tok& b) noexcept
    {
        json_tok tmp = b;
        b = a;
        a = tmp;
    }

    class json_dec {
    public:
        explicit json_dec(const char* text = nullptr) noexcept : ptr(text), err(JSON_SUCCESS) {}

        json_dec(const json_dec&) = delete;
        json_dec& operator=(const json_dec&) = delete;

        void reset(const char* text) noexcept
        {
            ptr = text;
            err = JSON_SUCCESS;
            tok.clear();
        }

        bool next_token(json_tok& dest) noexcept
        {
            if (!ptr) {
                err = JSON_END;
                return false;
            }

            err = jsonparse(ptr, &tok.ctok);
            if (err != JSON_SUCCESS) {
                return false;
            }

            dest = tok;
            return true;
        }

        json_err_t status() const noexcept { return json_err_t(err); }

    private:
        const char* ptr;
        int err;
        json_tok tok;
    };

}

#endif
