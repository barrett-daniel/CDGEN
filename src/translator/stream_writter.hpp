#pragma once

#include <sstream>
#include <string>
#include <cassert>

namespace stream_util {

    // wrapping the below in a namespace to prevent polluting the top-level namespace

    // just a mirror of std::format/wrapped of snprintf to handle variadic arguments and encode such in a string.
    template<typename... Args>
    static std::string format(const char* fmt, Args... args) {
        int size = std::snprintf(nullptr, 0, fmt, args...);
        assert(size > 0);

        std::string buf(static_cast<size_t>(size), '\0');

        // size+1 for the null terminator
        std::snprintf(buf.data(), size + 1, fmt, args...);

        return buf;
    }

    // pass through primitive types and unwrap std::strings as c-strings
    template<typename T>
    static T unwrap(T v) { return v; }
    static const char* unwrap(const std::string& s) { return s.c_str(); }
};

/**
 * Helper class to make writing to a stringstream a bit easier. Handles variadic arguments so writing to such
 * is similar to printf.
 */
class StreamWriter {
    public:
        StreamWriter() = default;

        void indent() {
            indent_buff += '\t';
        }

        void dedent() {
            assert(!indent_buff.empty());
            indent_buff.pop_back();
        }

        void write_line() {
            ss << std::endl;
        }

        template<typename... Args>
        void write_line(const char* fmt, Args... args) {
            if (sizeof...(args) > 0) {
                ss << indent_buff << stream_util::format(fmt, stream_util::unwrap(args)...) << std::endl;
                return;
            }

            ss << indent_buff << fmt << std::endl;
        }

        template<typename... Args>
        void write_line(const std::string& fmt, Args... args) {
            write_line(fmt.c_str(), stream_util::unwrap(args)...);
        }

        std::string get_code_str() const {
            return ss.str();
        }

    private:
        std::stringstream ss;
        std::string indent_buff;
};