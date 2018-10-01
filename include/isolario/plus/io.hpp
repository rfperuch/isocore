//
// Copyright (c) 2018, Enrico Gregori, Alessandro Improta, Luca Sani, Institute
// of Informatics and Telematics of the Italian National Research Council
// (IIT-CNR). All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE IIT-CNR BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef ISOLARIO_PLUS_IO_HPP_
#define ISOLARIO_PLUS_IO_HPP_

extern "C" {
#include <isolario/io.h>
}

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdio>
#include <utility>

namespace isolario
{
    enum class io_access { read = 'r',
                           write = 'w' };

    struct io_params {
        std::size_t bufsiz;  ///< Reading/Writing buffer size.
        io_access access;    ///< File access mode.

        constexpr io_params() noexcept
            : bufsiz(0), access(io_access::read) {}
    };

    enum class z_format { rfc1951 = 'd',
                          rfc1950 = 'z',
                          rfc1952 = 'g' };

    struct z_params : io_params {
        z_format format;  ///< Z compression format.
        int compression;  ///< Only meaningful for \a io_access::write, compression level [1-9]
        int window_bits;  ///< Only meaningful for \a io_access::write, sensible range: [8, 15]
        inline static constexpr const char *extension = ".gz";

        constexpr z_params() noexcept
            : io_params(), format(z_format::rfc1952), compression(-1), window_bits(15)
        {
        }
    };

    struct bz2_params : io_params {
        int compression;  ///< Only meaningful for \a io_access::write, compression level [1-9]
        int verbosity;    ///< Verbosity level [0-9]
        int work_factor;  ///< Only meaningful for \a io_access::write, BZ2 work factor
        bool small_flag;  ///< Only meaningful for \a io_access::read, BZ2 small flag
        inline static constexpr const char *extension = ".bz2";

        constexpr bz2_params() noexcept
            : io_params(), compression(9), verbosity(0), work_factor(0), small_flag(false)
        {
        }
    };

    enum class lzma_checksum { none = '-',
                               crc32 = 'c',
                               crc64 = 'C',
                               sha256 = 's' };

    struct xz_params : io_params {
        int compression;             ///< Only meaningful for \a io_access::write, compression level.
        lzma_checksum checksum;      ///< Only meaningful for \a io_access::write, checksum algorithm used for data integrity.
        std::uint64_t memory_usage;  ///< Only meaningful for \a io_access::read, decoding process memory usage.
        bool extreme_preset;         ///< Only meaningful for \a io_access::write, extreme preset for data compression.
        inline static constexpr const char *extension = ".xz";

        constexpr xz_params() noexcept
            : io_params(), compression(6), checksum(lzma_checksum::crc64), memory_usage(~0ull), extreme_preset(false)
        {
        }
    };

    struct lz4_params : io_params {
        int compression;
        inline static constexpr const char *extension = ".lz4";

        constexpr lz4_params() noexcept
            : io_params(), compression(0) {}
    };

    class io_rw
    {
      public:
        using native_type = io_rw_t *;

        io_rw() noexcept
            : ptr(nullptr) {}

        io_rw(const io_rw &) = delete;

        io_rw(io_rw &&rw) noexcept
            : ptr(std::exchange(rw.ptr, nullptr)),
              buf(rw.buf)
        {
            if (ptr == &rw.buf) {
                // ptr pointed to the local rw buffer, remap to this->buf
                ptr = &buf;
            }
        }

        io_rw &operator=(const io_rw &) = delete;

        io_rw &operator=(io_rw &&rw) noexcept
        {
            ptr = std::exchange(rw.ptr, nullptr);
            buf = rw.buf;
            if (ptr == &rw.buf) {
                ptr = &buf;
            }
            return *this;
        }

        void fd_open(int fd) noexcept
        {
            close();

            io_fd_init(&buf, fd);
            ptr = &buf;
        }

        bool fd_open(const char *name, io_access access = io_access::read)
        {
            close();

            int fd = do_open(name, access);
            if (fd < 0) {
                return false;
            }

            io_fd_init(&buf, fd);
            ptr = &buf;
            return true;
        }

        void file_open(std::FILE *file) noexcept
        {
            close();

            io_file_init(&buf, file);
            ptr = &buf;
        }

        bool file_open(const char *name, io_access access = io_access::read) noexcept
        {
            close();

            std::FILE *file = do_fopen(name, access);
            if (!file) {
                return false;
            }

            io_file_init(&buf, file);
            ptr = &buf;
            return true;
        }

        bool z_open(int fd, const z_params &params = z_params()) noexcept
        {
            close();

            return do_z_open(fd, params);
        }

        bool z_open(const char *name, const z_params &params = z_params()) noexcept
        {
            close();

            int fd = do_open(name, params.access);
            if (fd < 0) {
                return false;
            }

            return do_z_open(fd, params);
        }

        bool bz2_open(int fd, const bz2_params &params = bz2_params())
        {
            close();

            return do_bz2_open(fd, params);
        }

        bool bz2_open(const char *name, const bz2_params &params = bz2_params()) noexcept
        {
            close();

            int fd = do_open(name, params.access);
            if (fd < 0) {
                return false;
            }

            return do_bz2_open(fd, params);
        }

        bool xz_open(int fd, const xz_params &params = xz_params()) noexcept
        {
            close();

            return do_xz_open(fd, params);
        }

        bool xz_open(const char *name, const xz_params &params = xz_params()) noexcept
        {
            close();

            int fd = do_open(name, params.access);
            if (fd < 0) {
                return false;
            }

            return do_xz_open(fd, params);
        }

        bool lz4_open(int fd, const lz4_params &params = lz4_params()) noexcept
        {
            close();

            return do_lz4_open(fd, params);
        }

        bool lz4_open(const char *name, const lz4_params &params = lz4_params()) noexcept
        {
            close();

            int fd = do_open(name, params.access);
            if (fd < 0) {
                return false;
            }

            return do_lz4_open(fd, params);
        }

        std::size_t read(void *dest, std::size_t n) noexcept
        {
            if (!is_open()) {
                return 0;
            }

            return ptr->read(ptr, dest, n);
        }

        std::size_t write(const void *src, std::size_t n) noexcept
        {
            if (!is_open()) {
                return 0;
            }

            return ptr->write(ptr, src, n);
        }

        bool has_error() const noexcept { return is_open() && ptr->error(ptr) != 0; }

        bool is_open() const noexcept { return ptr != nullptr; }

        native_type native() noexcept { return ptr; }

        explicit operator bool() const noexcept { return is_open() && ptr->error(ptr) == 0; }

        bool close() noexcept
        {
            bool ok = true;
            if (is_open()) {
                if (ptr->close(ptr) != 0) {
                    ok = false;
                }
                ptr = nullptr;
            }
            return ok;
        }

        ~io_rw() { close(); }

      private:
        int do_open(const char *name, io_access access) noexcept
        {
            switch (access) {
                case io_access::read:
                    return open(name, O_RDONLY);
                case io_access::write:
                    return open(name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
                default:
                    return -1;
            }
        }

        std::FILE *do_fopen(const char *name, io_access access) noexcept
        {
            switch (access) {
                case io_access::read:
                    return std::fopen(name, "rb");
                case io_access::write:
                    return std::fopen(name, "wb");
                default:
                    return nullptr;
            }
        }

        bool do_z_open(int fd, const z_params &params) noexcept
        {
            char mode[16];

            char *p = mode;
            *p++ = char(params.access);

            switch (params.access) {
                case io_access::write:
                    *p++ = '*';
                    *p++ = 'b';
                    *p++ = '*';
                    [[fallthrough]];
                case io_access::read:
                    *p++ = char(params.format);
                    *p = '\0';
                    break;
                default:
                    return false;
            }

            // NOTE: arguments are ignored for io_access::read
            ptr = io_zopen(fd, params.bufsiz, mode, params.compression, params.window_bits);
            return ptr != nullptr;
        }

        bool do_bz2_open(int fd, const bz2_params &params) noexcept
        {
            char mode[16];

            char *p = mode;
            *p++ = char(params.access);

            switch (params.access) {
                case io_access::read:
                    if (params.small_flag) {
                        *p++ = '-';
                    }
                    *p++ = 'v';
                    *p++ = '*';
                    *p = '\0';
                    ptr = io_bz2open(fd, params.bufsiz, mode, params.verbosity);
                    break;
                case io_access::write:
                    *p++ = '*';
                    *p++ = '+';
                    *p++ = '*';
                    *p++ = 'v';
                    *p++ = '*';
                    *p = '\0';
                    ptr = io_bz2open(fd, params.bufsiz, mode, params.compression, params.work_factor, params.verbosity);
                    break;
                default:
                    return false;
            }

            return ptr != nullptr;
        }

        bool do_xz_open(int fd, const xz_params &params) noexcept
        {
            char mode[32];

            char *p = mode;
            *p++ = char(params.access);
            switch (params.access) {
                case io_access::write:
                    *p++ = '*';
                    if (params.extreme_preset) {
                        *p++ = 'e';
                    }

                    *p++ = char(params.checksum);
                    *p++ = '\0';

                    ptr = io_xzopen(fd, params.bufsiz, mode, params.compression);
                    break;
                case io_access::read:
                    *p++ = char(params.checksum);
                    *p++ = 'm';
                    *p++ = '*';
                    *p = '\0';

                    ptr = io_xzopen(fd, params.bufsiz, mode, params.memory_usage);
                    break;
                default:
                    return false;
            }

            return ptr != nullptr;
        }

        bool do_lz4_open(int fd, const lz4_params &params) noexcept
        {
            char mode[16];

            char *p = mode;
            *p++ = char(params.access);
            switch (params.access) {
                case io_access::write:
                    *p++ = '*';
                    [[fallthrough]];
                case io_access::read:
                    *p = '\0';
                    break;
                default:
                    return false;
            }

            ptr = io_lz4open(fd, params.bufsiz, mode, params.compression);
            return ptr != nullptr;
        }

        io_rw_t *ptr;
        io_rw_t buf;
    };

}  // namespace isolario

#endif
