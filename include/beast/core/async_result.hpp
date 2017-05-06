//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_ASYNC_COMPLETION_HPP
#define BEAST_ASYNC_COMPLETION_HPP

#include <beast/config.hpp>
#include <beast/core/handler_concepts.hpp>
#include <beast/core/detail/type_traits.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>
#include <type_traits>
#include <utility>

//------------------------------------------------------------------------------

namespace beast {

/** An interface for customising the behaviour of an initiating function.

    The async_result class is used for determining:
    
    @li the concrete completion handler type to be called at the end of the
    asynchronous operation;
    
    @li the initiating function return type; and
    
    @li how the return value of the initiating function is obtained.
    
    The trait allows the handler and return types to be determined at the point
    where the specific completion handler signature is known.

    This template takes advantage of specializations of both
    `boost::asio_async_result` and `boost::asio::handler_type` for user-defined
    completion token types. The primary template assumes that the
    @b CompletionToken is the completion handler.
*/
template<class CompletionToken, class Signature>
class async_result
{
    static_assert(! std::is_reference<CompletionToken>::value, "");

public:
    async_result(async_result const&) = delete;
    async_result& operator=(async_result const&) = delete;

    using completion_handler_type =
        typename boost::asio::handler_type<
            CompletionToken, Signature>::type;

    using return_type =
        typename boost::asio::async_result<
            completion_handler_type>::type;

    explicit
    async_result(completion_handler_type& h)
        : impl_(h)
    {
    }

    return_type
    get()
    {
        return impl_.get();
    }

private:
    boost::asio::async_result<
        completion_handler_type> impl_;
};

/** Helper for customizing the return type of asynchronous initiation functions.

    This class template is used to transform caller-provided completion
    handlers in calls to asynchronous initiation functions. The transformation
    allows customization of the return type of the initiating function, and the
    function signature of the final handler.

    @tparam CompletionHandler A completion handler, or a user defined type
    with specializations for customizing the return type (for example,
    `boost::asio::use_future` or `boost::asio::yield_context`).

    @tparam Signature The callable signature of the final completion handler.

    Example:
    @code
    ...
    template<class CompletionHandler>
    typename async_completion<CompletionHandler,
        void(error_code)>::result_type
    async_initfn(..., CompletionHandler&& handler)
    {
        async_completion<CompletionHandler,
            void(error_code)> init{handler};
        ...
        return init.result.get();
    }
    @endcode

    @note See <a href="http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3896.pdf">
        Library Foundations For Asynchronous Operations</a>
*/
template<class CompletionHandler, class Signature>
struct async_completion
{
    /** The type of the final handler called by the asynchronous initiation function.

        Objects of this type will be callable with the specified signature.
    */
    using handler_type =
        typename boost::asio::handler_type<
            CompletionHandler, Signature>::type;

    /// The type of the value returned by the asynchronous initiation function.
    using result_type = typename
        boost::asio::async_result<handler_type>::type;

    /** Construct the helper.

        @param token The completion handler. Copies will be made as
        required. If `CompletionHandler` is movable, it may also be moved.
    */
    async_completion(typename std::remove_reference<CompletionHandler>::type& token)
        : handler(std::forward<CompletionHandler>(token))
        , result(handler)
    {
        static_assert(is_CompletionHandler<handler_type, Signature>::value,
            "Handler requirements not met");
    }

    /// The final completion handler, callable with the specified signature.
    handler_type handler;

    /// The return value of the asynchronous initiation function.
    boost::asio::async_result<handler_type> result;
};

#define BEAST_INITFN_RESULT_TYPE(ct, sig) \
    typename async_completion<ct, sig>::result_type
    //typename async_result< \
        //typename std::decay<ct>::type, sig>::type

} // beast

#endif
