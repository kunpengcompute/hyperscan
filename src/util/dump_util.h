/*
 * Copyright (c) 2016-2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DUMP_UTIL
#define DUMP_UTIL

#include "noncopyable.h"

#include <cstdio>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>

namespace ue2 {

#ifdef DUMP_SUPPORT
/** Bounds-checked, unaligned reads for the optional bytecode diagnostics. */
class DumpBuffer {
public:
    DumpBuffer(const void *data_in, size_t size_in)
        : data(static_cast<const unsigned char *>(data_in)), length(size_in) {}

    size_t size() const { return length; }

    bool contains(size_t offset, size_t bytes) const {
        return data && offset <= length && bytes <= length - offset;
    }

    bool containsArray(size_t offset, size_t count, size_t element_size) const {
        return element_size && contains(offset, 0)
            && count <= (length - offset) / element_size;
    }

    template<class T> bool read(size_t offset, T &value) const {
        if (!contains(offset, sizeof(T))) {
            return false;
        }
        std::memcpy(&value, data + offset, sizeof(T));
        return true;
    }

    DumpBuffer slice(size_t offset, size_t bytes) const {
        if (!contains(offset, bytes)) {
            return DumpBuffer(nullptr, 0);
        }
        return DumpBuffer(data + offset, bytes);
    }

private:
    const unsigned char *data;
    size_t length;
};
#endif

/**
 * Same as fopen(), but on error throws an exception rather than returning NULL.
 */
FILE *fopen_or_throw(const char *path, const char *mode);

/**
 * \brief Helper class: wraps C stdio FILE* handle and takes care of closing
 * the file on destruction.
 */
class StdioFile : noncopyable {
public:
    StdioFile(const std::string &filename, const char *mode)
        : handle(fopen_or_throw(filename.c_str(), mode), &fclose) {}

    // Implicit conversion to FILE* for use by stdio calls.
    operator FILE *() { return handle.get(); }

private:
    std::unique_ptr<FILE, decltype(&fclose)> handle;
};

} // namespace ue2

#endif
