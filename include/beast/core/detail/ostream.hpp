//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_DETAIL_OSTREAM_HPP
#define BEAST_DETAIL_OSTREAM_HPP

#include <ostream>
#include <streambuf>
#include <utility>

namespace beast {
namespace detail {

template<
    class DynamicBuffer,
    class CharT,
    class Traits
>
class ostream_buffer
    : public std::basic_streambuf<CharT, Traits>
{
    using int_type = typename
        std::basic_streambuf<CharT, Traits>::int_type;

    using traits_type = typename
        std::basic_streambuf<CharT, Traits>::traits_type;

    static std::size_t constexpr max_size = 512;

    DynamicBuffer& buf_;

public:
    ostream_buffer(
        ostream_buffer&&) = default;

    ~ostream_buffer() noexcept;

    explicit
    ostream_buffer(
        DynamicBuffer& buf);

    int_type
    overflow(int_type ch) override;

    int
    sync() override;

private:
    void
    prepare();

    void
    flush(int extra = 0);
};

template<class DynamicBuffer, class CharT, class Traits>
ostream_buffer<
    DynamicBuffer, CharT, Traits>::
~ostream_buffer() noexcept
{
    sync();
}

template<class DynamicBuffer, class CharT, class Traits>
ostream_buffer<
    DynamicBuffer, CharT, Traits>::
ostream_buffer(DynamicBuffer& buf)
    : buf_(buf)
{
    prepare();
}

template<class DynamicBuffer, class CharT, class Traits>
auto
ostream_buffer<
    DynamicBuffer, CharT, Traits>::
overflow(int_type ch) ->
    int_type
{
    if(ch != traits_type::eof())
    {
        Traits::assign(*this->pptr(), ch);
        flush(1);
        prepare();
        return ch;
    }
    flush();
    return traits_type::eof();
}

template<class DynamicBuffer, class CharT, class Traits>
int
ostream_buffer<
    DynamicBuffer, CharT, Traits>::
sync()
{
    flush();
    prepare();
    return 0;
}

template<class DynamicBuffer, class CharT, class Traits>
void
ostream_buffer<
    DynamicBuffer, CharT, Traits>::
prepare()
{
    using boost::asio::buffer_cast;
    using boost::asio::buffer_size;
    auto mbs = buf_.prepare(
        read_size_helper(buf_, max_size));
    auto const mb = *mbs.begin();
    auto const p = buffer_cast<CharT*>(mb);
    this->setp(p,
        p + buffer_size(mb) / sizeof(CharT) - 1);
}

template<class DynamicBuffer, class CharT, class Traits>
void
ostream_buffer<
    DynamicBuffer, CharT, Traits>::
flush(int extra)
{
    buf_.commit(
        (this->pptr() - this->pbase() + extra) *
            sizeof(CharT));
}

template<
    class DynamicBuffer, class CharT, class Traits>
class ostream_helper
    : public std::basic_ostream<CharT, Traits>

{
    ostream_buffer<
        DynamicBuffer, CharT, Traits> osb_;

public:
    explicit ostream_helper(
        DynamicBuffer& buf);

    ostream_helper(
        ostream_helper&& other);
};

template<
    class DynamicBuffer, class CharT, class Traits>
ostream_helper<
    DynamicBuffer, CharT, Traits>::
ostream_helper(DynamicBuffer& buf)
    : std::basic_ostream<CharT, Traits>(
        &this->osb_)
    , osb_(buf)
{
}

template<
    class DynamicBuffer, class CharT, class Traits>
ostream_helper<
    DynamicBuffer, CharT, Traits>::
ostream_helper(
        ostream_helper&& other)
    : std::basic_ostream<CharT, Traits>(&osb_)
    , osb_(std::move(other.osb_))
{
}

} // detail
} // beast

#endif
