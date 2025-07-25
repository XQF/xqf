/* XQF - Quake server browser and launcher
What is not explicitly stated to be covered by another license is
distributed under the CC0 1.0 license.
See: https://creativecommons.org/public-domain/cc0/ */

/* XQF CC0 Source Code.
Copyright (C) 2025 XQF Team - https://xqf.github.io
No rights reserved. */

#if defined(USE_RELATIVE_PREFIX)
#include <algorithm>
#include <memory>
#include <string>

#include <cstring>
#include <unistd.h>

#include "system.h"
#endif

#if defined(USE_RELATIVE_PREFIX)
char xqf_PACKAGE_DATA_DIR[ XQF_MAX_PATH ] = {};
char xqf_LOCALEDIR[ XQF_MAX_PATH ] = {};
#else
const char* xqf_PACKAGE_DATA_DIR = PACKAGE_DATA_DIR;
const char* xqf_LOCALEDIR = LOCALEDIR;
#endif

#if defined(USE_RELATIVE_PREFIX)
namespace FS {
	std::string DefaultLibPath();
}

void setDefaultDirs()
{
#ifdef _WIN32
	std::string sep = "\\";
#else
	std::string sep = "/";
#endif

	std::string exe_dir = FS::DefaultLibPath();
#if defined(USE_FHS)
	std::string share_dir = exe_dir + sep + ".." + sep + "share";
#else
	std::string share_dir = exe_dir + sep + "share";
#endif
	std::string data_dir = share_dir + sep + "xqf";
	std::string locale_dir = share_dir + sep + "locale";

	strncpy(xqf_PACKAGE_DATA_DIR, data_dir.c_str(), std::min(data_dir.size(), size_t(XQF_MAX_PATH)));
	strncpy(xqf_LOCALEDIR, locale_dir.c_str(), std::min(locale_dir.size(), size_t(XQF_MAX_PATH)));
}

// The following is copied from https://github.com/DaemonEngine/Daemon

/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2025, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifdef _WIN32
namespace Str
{
// Copied from Daemon/src/common/String.h
template<typename T> class BasicStringRef {
	public:
		const T* begin() const
		{
			return ptr;
		}
		const T* end() const
		{
			return ptr + len;
		}
	private:
		const T* ptr;
		size_t len;
	};
}

// Copied from Daemon/src/common/String.cpp
std::string UTF16To8(Str::BasicStringRef<wchar_t> str)
{
	std::string out;
	auto it = str.begin();
	auto end = str.end();

	while (it != end) {
		uint16_t ch = *it++;
		if (ch >= UNICODE_SURROGATE_HEAD_START && ch <= UNICODE_SURROGATE_HEAD_END) {
			if (it == end)
				out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
			else {
				uint16_t tail = *it++;
				if (tail >= UNICODE_SURROGATE_TAIL_START && tail <= UNICODE_SURROGATE_TAIL_END)
					UTF8_Append(out, ((ch & UNICODE_SURROGATE_MASK) << 10) | (tail & UNICODE_SURROGATE_MASK));
				else
					out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
			}
		} else if (ch >= UNICODE_SURROGATE_TAIL_START && ch <= UNICODE_SURROGATE_TAIL_END)
			out.append(UNICODE_REPLACEMENT_CHAR_UTF8);
		else
			UTF8_Append(out, ch);
	}
	return out;
}
}
#endif

namespace FS
{
// Copied from Daemon/src/common/FileSystem.cpp
std::string DefaultLibPath()
{
#ifdef _WIN32
	wchar_t buffer[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return "";

	wchar_t* p = wcsrchr(buffer, L'\\');
	if (!p)
		return empty_string;
	*p = L'\0';

	return Str::UTF16To8(buffer);
#elif defined(__linux__) || defined(__FreeBSD__)
	ssize_t len = 64;
	while (true) {
		std::unique_ptr<char[]> out(new char[len]);
#if defined(__linux__)
		const char* proc_file = "/proc/self/exe";
#elif defined(__FreeBSD__)
		const char* proc_file = "/proc/curproc/file";
#endif
		ssize_t result = readlink(proc_file, out.get(), len);
		if (result == -1)
			return "";
		if (result < len) {
			out[result] = '\0';
			char* p = strrchr(out.get(), '/');
			if (!p)
				return "";
			*p = '\0';
			return out.get();
		}
		len *= 2;
	}
#elif defined(__APPLE__)
	uint32_t bufsize = 0;
	_NSGetExecutablePath(nullptr, &bufsize);

	std::unique_ptr<char[]> out(new char[bufsize]);
	_NSGetExecutablePath(out.get(), &bufsize);

	char* p = strrchr(out.get(), '/');
	if (!p)
		return "";
	*p = '\0';

	return out.get();
#endif
}
}
#endif // USE_RELATIVE_PREFIX
