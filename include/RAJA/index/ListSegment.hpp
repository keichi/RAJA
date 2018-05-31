/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file defining list segment classes.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-18, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-689114
//
// All rights reserved.
//
// This file is part of RAJA.
//
// For details about use and distribution, please read RAJA/LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_ListSegment_HPP
#define RAJA_ListSegment_HPP

#include "RAJA/config.hpp"
#include "RAJA/internal/Span.hpp"
#include "RAJA/util/concepts.hpp"
#include "RAJA/util/defines.hpp"
#include "RAJA/util/types.hpp"

#if defined(RAJA_ENABLE_CUDA)
#include "RAJA/policy/cuda/raja_cudaerrchk.hpp"
#else
#define cudaErrchk(...)
#endif

#include <memory>
#include <type_traits>
#include <utility>

namespace RAJA
{

/*!
 ******************************************************************************
 *
 * \brief  Class representing an arbitrary collection of indices.
 *
 *         Length indicates number of indices in index array.
 *         Traversal executes as:
 *            for (i = 0; i < getLength(); ++i) {
 *               expression using m_indx[i] as array index.
 *            }
 *
 ******************************************************************************
 */
template <typename T>
class TypedListSegment
{

#if defined(RAJA_ENABLE_CUDA)
  static constexpr bool Has_CUDA = true;
#else
  static constexpr bool Has_CUDA = false;
#endif

  //! tag for trivial per-element copy
  struct TrivialCopy {
  };
  //! tag for memcpy-style copy
  struct BlockCopy {
  };

  //! alias for GPU memory tag
  using GPU_memory = std::integral_constant<bool, true>;
  //! alias for CPU memory tag
  using CPU_memory = std::integral_constant<bool, false>;

  //! specialization for deallocation of GPU_memory
  void deallocate(GPU_memory) { cudaErrchk(cudaFree(m_data)); }

  //! specialization for allocation of GPU_memory
  void allocate(GPU_memory)
  {
    cudaErrchk(cudaMallocManaged((void**)&m_data,
                                 m_size * sizeof(value_type),
                                 cudaMemAttachGlobal));
  }

  //! specialization for deallocation of CPU_memory
  void deallocate(CPU_memory) { delete[] m_data; }

  //! specialization for allocation of CPU_memory
  void allocate(CPU_memory) { m_data = new T[m_size]; }

#ifdef RAJA_ENABLE_CUDA
  //! copy data from container using BlockCopy
  template <typename Container>
  void copy(Container&& src, BlockCopy)
  {
    cudaErrchk(cudaMemcpy(
        m_data, &(*src.begin()), m_size * sizeof(T), cudaMemcpyDefault));
  }
#endif

  //! copy data from container using TrivialCopy
  template <typename Container>
  void copy(Container&& source, TrivialCopy)
  {
    auto dest = m_data;
    auto src = source.begin();
    auto const end = source.end();
    while (src != end) {
      *dest = *src;
      ++dest;
      ++src;
    }
  }

  // internal helper to allocate data and populate with data from a container
  template <bool GPU, typename Container>
  void allocate_and_copy(Container&& src)
  {
    allocate(std::integral_constant<bool, GPU>());
    static constexpr bool use_cuda =
        GPU && std::is_pointer<decltype(src.begin())>::value
        && std::is_same<type_traits::IterableValue<Container>,
                        value_type>::value;
    using TagType =
        typename std::conditional<use_cuda, BlockCopy, TrivialCopy>::type;
    copy(src, TagType());
  }

public:
  //! value type for storage
  using value_type = T;

  //! iterator type for storage (will be a pointer)
  using iterator = T*;

  //! prevent compiler from providing a default constructor
  TypedListSegment() = delete;

  ///
  /// \brief Construct list segment from given array with specified length.
  ///
  /// By default the ctor on the host performs deep copy of array elements.
  /// By default the ctor on the device does not perform a copy of array of elements. 
  ///
  RAJA_HOST_DEVICE TypedListSegment(const value_type* values,
                                    Index_type length) 
    : m_data{const_cast<value_type*>(values)}, m_size{length}, m_owned{Unowned}
  {
#if !defined(RAJA_DEVICE_CODE)
    // Deep copy may only take place on the host. 
    initIndexData(values, length, Owned);
#endif
  }

  ///
  /// \brief Construct list segment from given array with specified length.
  /// If 'Unowned' is passed as last argument, the constructed object
  /// does not own the segment data and will hold a pointer to given data.
  /// In this case, caller must manage object lifetimes properly.
  /// This constructor is host only since ownership may only be specified 
  /// on the host.
  TypedListSegment(const value_type* values,
                   Index_type length,
                   IndexOwnership owned)
  {
    // future TODO -- change to initializer list somehow
    initIndexData(values, length, owned);
  }

  ///
  /// Construct list segment from arbitrary object holding
  /// indices using a deep copy of given data.
  ///
  /// The object must provide methods: begin(), end(), size().
  ///
  template <typename Container>
  explicit TypedListSegment(const Container& container)
      : m_data(nullptr), m_size(container.size()), m_owned(Unowned)
  {
    if (m_size <= 0) return;
    allocate_and_copy<Has_CUDA>(container);
    m_owned = Owned;
  }

  ///
  /// Copy-constructor for list segment.
  ///
  RAJA_HOST_DEVICE
  TypedListSegment(const TypedListSegment& other)
    : m_data{other.m_data}, m_size{other.m_size}, m_owned{Unowned}
  {
#if !defined(RAJA_DEVICE_CODE)    
    // Deep copy may only take place on the host. 
    initIndexData(other.m_data, other.m_size, other.m_owned);
#endif
  }

  ///
  /// Assignment operetor for list segment.
  ///  
  RAJA_HOST_DEVICE RAJA_INLINE TypedListSegment& operator=(TypedListSegment const & other)
  {        
#if !defined(RAJA_DEVICE_CODE)
    // Deep copy may only take place on the host. 
    if (this != &other) {
      initIndexData(other.m_data, other.m_size, other.m_owned);
    }
#else
    m_data = other.m_data;
    m_size = other.m_size;    
    m_owned = Unowned;
#endif
    return *this;
  }

  ///
  /// Move-constructor for list segment.
  ///
  RAJA_HOST_DEVICE
  TypedListSegment(TypedListSegment&& rhs)
      : m_data(rhs.m_data), m_size(rhs.m_size), m_owned(rhs.m_owned)
  {
    // make the rhs non-owning so it's destructor won't have any side effects
    rhs.m_owned = Unowned;
  }

  ///
  /// Move operator for list segment.
  ///  
  RAJA_HOST_DEVICE RAJA_INLINE TypedListSegment& operator=(TypedListSegment&& other)
  {        

    m_data = other.m_data;
    m_size = other.m_size;    
    m_owned = other.m_owned;

    // make the rhs non-owning so it's destructor won't have any side effects
    other.m_owned = Unowned;

    return *this;
  }

  ///
  /// Destroy segment including its contents
  ///
  RAJA_HOST_DEVICE
  ~TypedListSegment()
  {
#if !defined(RAJA_DEVICE_CODE)
    // Deallocation may only take place on the host
    if (m_data == nullptr || m_owned != Owned) return;
    deallocate(std::integral_constant<bool, Has_CUDA>());
#endif
  }


  ///
  /// Swap function for copy-and-swap idiom.
  ///
  RAJA_HOST_DEVICE void swap(TypedListSegment& other)
  {
    camp::safe_swap(m_data, other.m_data);
    camp::safe_swap(m_size, other.m_size);
    camp::safe_swap(m_owned, other.m_owned);
  }

  //! accessor to get the end iterator for a TypedListSegment
  RAJA_HOST_DEVICE iterator end() const { return m_data + m_size; }

  //! accessor to get the begin iterator for a TypedListSegment
  RAJA_HOST_DEVICE iterator begin() const { return m_data; }
  //! accessor to retrieve the total number of elements in a TypedListSegment
  RAJA_HOST_DEVICE Index_type size() const { return m_size; }

  //! Create a slice of this instance as a new instance but with no ownership
  //! of the data it points to.
  /*!
   * \return A new instance spanning *begin() + begin to *begin() + begin +
   * length
   * 
   * Current status: Tiling may not be distributing data correctly
   */
  RAJA_HOST_DEVICE RAJA_INLINE TypedListSegment slice(Index_type begin,
                                                      Index_type length) const
  {
    Index_type end = begin+length > m_size ? (m_size-begin) : length;
#if !defined(RAJA_DEVICE_CODE)
    return TypedListSegment(&m_data[begin], end, Unowned);
#else
    return TypedListSegment(&m_data[begin], end);
#endif
  }

  //! get ownership of the data (Owned/Unowned)
  RAJA_HOST_DEVICE IndexOwnership getIndexOwnership() const { return m_owned; }

  //! checks a pointer and size (Span) for equality to all elements in the
  //! TypedListSegment
  RAJA_HOST_DEVICE bool indicesEqual(const value_type* container,
                                     Index_type len) const
  {
    if (container == m_data) return len == m_size;
    if (len != m_size || container == nullptr || m_data == nullptr)
      return false;
    for (Index_type i = 0; i < m_size; ++i)
      if (m_data[i] != container[i]) return false;
    return true;
  }

  ///
  /// Equality operator returns true if segments are equal; else false.
  ///
  RAJA_HOST_DEVICE bool operator==(const TypedListSegment& other) const
  {
    return (indicesEqual(other.m_data, other.m_size));
  }

  ///
  /// Inequality operator returns true if segments are not equal, else false.
  ///
  RAJA_HOST_DEVICE bool operator!=(const TypedListSegment& other) const
  {
    return (!(*this == other));
  }

private:
  //
  // Initialize segment data properly based on whether object
  // owns the index data.
  //  
  void initIndexData(const value_type* container,
                     Index_type len,
                     IndexOwnership container_own)
  {
    // empty
    if (len <= 0 || container == nullptr) {
      m_data = nullptr;
      m_size = 0;
      m_owned = Unowned;
      return;
    }
    // some size -- initialize accordingly
    m_size = len;
    m_owned = container_own;
    if (m_owned == Owned) {
      allocate_and_copy<Has_CUDA>(RAJA::impl::make_span(container, len));
      return;
    }
    // Uh-oh. Using evil const_cast....
    m_data = const_cast<value_type*>(container);
  }

  //! buffer storage for list data
  value_type* RAJA_RESTRICT m_data;
  //! size of list segment
  Index_type m_size;
  //! ownership flag to guide data copying/management
  IndexOwnership m_owned;
};

//! alias for A TypedListSegment with storage type @Index_type
using ListSegment = TypedListSegment<Index_type>;

}  // closing brace for RAJA namespace

namespace std
{

/*!
 *  Specialization of std::swap for TypedListSegment
 */
template <typename T>
RAJA_INLINE void swap(RAJA::TypedListSegment<T>& a,
                      RAJA::TypedListSegment<T>& b)
{
  a.swap(b);
}
}

#endif  // closing endif for header file include guard
