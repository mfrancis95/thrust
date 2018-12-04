/******************************************************************************
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

// TODO: Move into system::cuda

#pragma once

#include <thrust/detail/config.h>
#include <thrust/detail/cpp11_required.h>

#if THRUST_CPP_DIALECT >= 2011

#if THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_NVCC

#include <thrust/system/cuda/config.h>

#include <thrust/system/cuda/detail/async/customization.h>
#include <thrust/system/cuda/detail/async/transform.h>
#include <thrust/system/cuda/detail/cross_system.h>
#include <thrust/system/cuda/future.h>
#include <thrust/iterator/iterator_traits.h>
#include <thrust/type_traits/logical_metafunctions.h>
#include <thrust/detail/static_assert.h>
#include <thrust/type_traits/is_trivially_relocatable.h>
#include <thrust/type_traits/is_contiguous_iterator.h>
#include <thrust/distance.h>

#include <type_traits>

THRUST_BEGIN_NS

namespace system { namespace cuda { namespace detail
{

// Non-ContiguousIterator output iterator
// TriviallyRelocatable value type
// Device to host, host to device
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt, typename Size
>
THRUST_RUNTIME_FUNCTION
auto async_copy_n(
  FromPolicy& from_exec
, ToPolicy&   to_exec
, ForwardIt   first
, Size        n
, OutputIt    output
) ->
  typename std::enable_if<
    conjunction<
      negation<is_contiguous_iterator<OutputIt>>
    , is_trivially_relocatable_to<
        typename iterator_traits<ForwardIt>::value_type
      , typename iterator_traits<OutputIt>::value_type
      >
    , disjunction<
        decltype(is_host_to_device_copy(from_exec, to_exec))
      , decltype(is_device_to_host_copy(from_exec, to_exec))
      >
    >::value
  , unique_eager_future<
      OutputIt
    , typename thrust::detail::allocator_traits<
        decltype(get_async_universal_host_pinned_allocator(
          select_device_system(from_exec, to_exec)
        ))
      >::template rebind_traits<OutputIt>::pointer
    >
  >::type
{
  THRUST_STATIC_ASSERT_MSG(
    (thrust::detail::depend_on_instantiation<ForwardIt, false>::value)
  , "copying to non-ContiguousIterators in another system from the cuda system "
    "is not currently supported"
  );

  return {};
}

// Workaround for an NVCC bug; when two SFINAE-enabled overloads are only
// distinguishable by a part of a SFINAE condition that is in a `decltype`,
// NVCC thinks they are the same overload and emits an error.
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt
>
struct is_buffered_trivially_relocatable_host_to_device_copy
  : thrust::integral_constant<
      bool
    ,    !is_contiguous_iterator<ForwardIt>::value
      && !is_contiguous_iterator<OutputIt>::value
      && !is_trivially_relocatable_to<
            typename iterator_traits<ForwardIt>::value_type
          , typename iterator_traits<OutputIt>::value_type
          >::value
      && decltype(
           is_host_to_device_copy(
             std::declval<FromPolicy const&>()
           , std::declval<ToPolicy const&>()
           )
         )::value
    >
{};

// Non-ContiguousIterator input iterator, ContiguousIterator output iterator
// TriviallyRelocatable value type
// Host to device
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt, typename Size
>
THRUST_RUNTIME_FUNCTION
auto async_copy_n(
  FromPolicy&                                       from_exec
, thrust::system::cuda::execution_policy<ToPolicy>& to_exec
, ForwardIt                                         first
, Size                                              n
, OutputIt                                          output
) ->
  typename std::enable_if<
    is_buffered_trivially_relocatable_host_to_device_copy<
      FromPolicy
    , thrust::system::cuda::execution_policy<ToPolicy>
    , ForwardIt, OutputIt
    >::value
  , unique_eager_future<
      OutputIt
    , typename thrust::detail::allocator_traits<
        decltype(get_async_universal_host_pinned_allocator(
          select_device_system(from_exec, to_exec)
        ))
      >::template rebind_traits<OutputIt>::pointer
    >
  >::type
{
  // TODO: Use .after for refactoring

  // TODO: Buffer host-side, memcpy

  THRUST_STATIC_ASSERT_MSG(
    (thrust::detail::depend_on_instantiation<ForwardIt, false>::value)
  , "unimplemented"
  );

  return {};
}

// Workaround for an NVCC bug; when two SFINAE-enabled overloads are only
// distinguishable by a part of a SFINAE condition that is in a `decltype`,
// NVCC thinks they are the same overload and emits an error.
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt
>
struct is_buffered_trivially_relocatable_device_to_host_copy
  : thrust::integral_constant<
      bool
    ,    !is_contiguous_iterator<ForwardIt>::value
      && !is_contiguous_iterator<OutputIt>::value
      && !is_trivially_relocatable_to<
            typename iterator_traits<ForwardIt>::value_type
          , typename iterator_traits<OutputIt>::value_type
          >::value
      && decltype(
           is_device_to_host_copy(
             std::declval<FromPolicy const&>()
           , std::declval<ToPolicy const&>()
           )
         )::value
    >
{};

// Non-ContiguousIterator input iterator, ContiguousIterator output iterator
// TriviallyRelocatable value type
// Device to host
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt, typename Size
>
THRUST_RUNTIME_FUNCTION
auto async_copy_n(
  thrust::system::cuda::execution_policy<FromPolicy>& from_exec
, ToPolicy&                                           to_exec
, ForwardIt                                           first
, Size                                                n
, OutputIt                                            output
) ->
  typename std::enable_if<
    is_buffered_trivially_relocatable_device_to_host_copy<
      thrust::system::cuda::execution_policy<FromPolicy>
    , ToPolicy
    , ForwardIt, OutputIt
    >::value
  , unique_eager_future<
      OutputIt
    , typename thrust::detail::allocator_traits<
        decltype(get_async_universal_host_pinned_allocator(
          select_device_system(from_exec, to_exec)
        ))
      >::template rebind_traits<OutputIt>::pointer
    >
  >::type
{
  THRUST_STATIC_ASSERT_MSG(
    (thrust::detail::depend_on_instantiation<ForwardIt, false>::value)
  , "copying from non-ContiguousIterators in the cuda system to other systems "
    "is not currently supported"
  );

  // TODO: Buffer device-side, memcpy, static_assert for now

  return {};
}

template <typename InputType, typename OutputType>
void async_copy_n_compile_failure_non_trivially_relocatable_elements()
{
  THRUST_STATIC_ASSERT_MSG(
    (thrust::is_trivially_relocatable_to<OutputType, InputType>::value)
  , "only sequences of TriviallyRelocatable elements can be copied to and from "
    "the cuda system; specialize `thrust::proclaim_trivially_relocatable<T>` to "
    "indicate that a type can be copied by bitwise (e.g. by `memcpy`)"
  );
}

// Non-TriviallyRelocatable value type
// Host to device, device to host
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt, typename Size
>
THRUST_RUNTIME_FUNCTION
auto async_copy_n(
  FromPolicy& from_exec
, ToPolicy&   to_exec
, ForwardIt   first
, Size        n
, OutputIt    output
) ->
  typename std::enable_if<
    conjunction<
      negation<
        is_trivially_relocatable_to<
          typename iterator_traits<ForwardIt>::value_type
        , typename iterator_traits<OutputIt>::value_type
        >
      >
    , disjunction<
        decltype(is_host_to_device_copy(from_exec, to_exec))
      , decltype(is_device_to_host_copy(from_exec, to_exec))
      >
    >::value
  , unique_eager_future<
      OutputIt
    , typename thrust::detail::allocator_traits<
        decltype(get_async_universal_host_pinned_allocator(
          select_device_system(from_exec, to_exec)
        ))
      >::template rebind_traits<OutputIt>::pointer
    >
  >::type
{
  // TODO: We could do more here with cudaHostRegister.

  async_copy_n_compile_failure_non_trivially_relocatable_elements<
    typename thrust::iterator_traits<ForwardIt>::value_type
  , typename std::add_lvalue_reference<
      typename thrust::iterator_traits<OutputIt>::value_type
    >::type
  >();

  return {};
}

// Non-ContiguousIterator input or output iterator, or non-TriviallyRelocatable value type
// Device to device
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt, typename Size
>
THRUST_RUNTIME_FUNCTION
auto async_copy_n(
  thrust::system::cuda::execution_policy<FromPolicy>& from_exec
, thrust::system::cuda::execution_policy<ToPolicy>&   to_exec
, ForwardIt                                           first
, Size                                                n
, OutputIt                                            output
) ->
  typename std::enable_if<
    conjunction<
      negation<
        thrust::is_trivially_relocatable_sequence_copy<ForwardIt, OutputIt>
      >
    , decltype(is_device_to_device_copy(from_exec, to_exec))
    >::value
  , unique_eager_future<
      OutputIt
    , typename thrust::detail::allocator_traits<
        decltype(get_async_universal_host_pinned_allocator(
          select_device_system(from_exec, to_exec)
        ))
      >::template rebind_traits<OutputIt>::pointer
    >
  >::type
{
  using T = typename thrust::iterator_traits<ForwardIt>::value_type;

  return async_transform_n(
    select_device_system(from_exec, to_exec)
  , first, n, output, thrust::identity<T>()
  );
}

// ContiguousIterator input and output iterators
// TriviallyCopyable elements
// Host to device, device to host, device to device
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename OutputIt, typename Size
>
THRUST_RUNTIME_FUNCTION
auto async_copy_n(
  FromPolicy& from_exec
, ToPolicy&   to_exec
, ForwardIt   first
, Size        n
, OutputIt    output
) ->
  typename std::enable_if<
    thrust::is_trivially_relocatable_sequence_copy<ForwardIt, OutputIt>::value 
  , unique_eager_future<
      OutputIt
    , typename thrust::detail::allocator_traits<
        decltype(get_async_universal_host_pinned_allocator(
          select_device_system(from_exec, to_exec)
        ))
      >::template rebind_traits<OutputIt>::pointer
    >
  >::type
{
  using T = typename thrust::iterator_traits<ForwardIt>::value_type;

  auto const uhp_alloc = get_async_universal_host_pinned_allocator(
    select_device_system(from_exec, to_exec)
  );

  using return_type = OutputIt;

  using return_pointer =
    typename thrust::detail::allocator_traits<decltype(uhp_alloc)>::
      template rebind_traits<return_type>::pointer;

  unique_eager_future_promise_pair<return_type, return_pointer> fp;

  // Create result storage.

  auto content = allocate_unique<OutputIt>(uhp_alloc, std::next(output, n));

  // Set up stream with dependencies.

  cudaStream_t const user_raw_stream = thrust::cuda_cub::stream(
    select_device_system(from_exec, to_exec)
  );

  if (thrust::cuda_cub::default_stream() != user_raw_stream)
  {
    fp = depend_on<return_type, return_pointer>(
      [] (decltype(content) const& c)
      { return c.get(); }
    , std::tuple_cat(
        std::make_tuple(
          std::move(content)
        , unique_stream(nonowning, user_raw_stream)
        )
      , extract_dependencies(
          std::move(select_device_system(from_exec, to_exec))
        )
      )
    );
  }
  else
  {
    fp = depend_on<return_type, return_pointer>(
      [] (decltype(content) const& c)
      { return c.get(); }
    , std::tuple_cat(
        std::make_tuple(
          std::move(content)
        )
      , extract_dependencies(
          std::move(select_device_system(from_exec, to_exec))
        )
      )
    );
  }

  // Run copy.

  thrust::cuda_cub::throw_on_error(
    cudaMemcpyAsync(
      thrust::raw_pointer_cast(&*output)
    , thrust::raw_pointer_cast(&*first)
    , sizeof(T) * n
    , direction_of_copy(from_exec, to_exec)
    , fp.future.stream()
    )
  , "after copy launch"
  );

  return std::move(fp.future);
}

}}} // namespace system::cuda::detail

namespace cuda_cub
{

// ADL entry point.
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename Sentinel, typename OutputIt
>
THRUST_RUNTIME_FUNCTION
auto async_copy(
  thrust::cuda::execution_policy<FromPolicy>&         from_exec
, thrust::cpp::execution_policy<ToPolicy>&            to_exec
, ForwardIt                                           first
, Sentinel                                            last
, OutputIt                                            output
)
THRUST_DECLTYPE_RETURNS(
  thrust::system::cuda::detail::async_copy_n(
    from_exec, to_exec, first, thrust::distance(first, last), output
  )
)

// ADL entry point.
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename Sentinel, typename OutputIt
>
THRUST_RUNTIME_FUNCTION
auto async_copy(
  thrust::cpp::execution_policy<FromPolicy>&          from_exec
, thrust::system::cuda::execution_policy<ToPolicy>&   to_exec
, ForwardIt                                           first
, Sentinel                                            last
, OutputIt                                            output
)
THRUST_DECLTYPE_RETURNS(
  thrust::system::cuda::detail::async_copy_n(
    from_exec, to_exec, first, thrust::distance(first, last), output
  )
)

// ADL entry point.
template <
  typename FromPolicy, typename ToPolicy
, typename ForwardIt, typename Sentinel, typename OutputIt
>
THRUST_RUNTIME_FUNCTION
auto async_copy(
  thrust::system::cuda::execution_policy<FromPolicy>& from_exec
, thrust::system::cuda::execution_policy<ToPolicy>&   to_exec
, ForwardIt                                           first
, Sentinel                                            last
, OutputIt                                            output
)
THRUST_DECLTYPE_RETURNS(
  thrust::system::cuda::detail::async_copy_n(
    from_exec, to_exec, first, thrust::distance(first, last), output
  )
)

} // cuda_cub

THRUST_END_NS

#endif // THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_NVCC

#endif // THRUST_CPP_DIALECT >= 2011
