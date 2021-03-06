//
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Copyright © 2021 ANSSI. All Rights Reserved.
//
// Author(s): fabienfl
//

#pragma once

#include <ostream>

#include <filesystem>

#include "Command/WolfLauncher/Console/Stream/LazyFileStreambuf.h"
#include "FileDisposition.h"

namespace Orc {
namespace Command {

template <typename CharT>
class LazyFileStream : public std::basic_ostream<CharT>
{
public:
    LazyFileStream(size_t bufferSize)
        : std::basic_ostream<CharT>(&m_streambuf)
        , m_streambuf(bufferSize)
    {
    }

    void Open(const std::filesystem::path& path, FileDisposition disposition, std::error_code& ec)
    {
        return m_streambuf.Open(path, disposition, ec);
    }

    void Close() { m_streambuf.Close(); }

    const std::optional<std::filesystem::path>& Path() const { return m_streambuf.Path(); }

private:
    LazyFileStreambuf<CharT> m_streambuf;
};

extern template class LazyFileStream<char>;
extern template class LazyFileStream<wchar_t>;

}  // namespace Command
}  // namespace Orc
