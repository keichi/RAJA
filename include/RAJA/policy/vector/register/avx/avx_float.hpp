/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file defining a SIMD register abstraction.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-19, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifdef __AVX__

#ifndef RAJA_policy_vector_register_avx_float_HPP
#define RAJA_policy_vector_register_avx_float_HPP

#include "RAJA/config.hpp"
#include "RAJA/util/macros.hpp"
#include "RAJA/pattern/vector.hpp"

// Include SIMD intrinsics header file
#include <immintrin.h>
#include <cmath>


namespace RAJA
{


  template<camp::idx_t N>
  class Register<avx_register, float, N> :
    public internal::RegisterBase<Register<avx_register, float, N>>
  {
    static_assert(N >= 1, "Vector must have at least 1 lane");
    static_assert(N <= 8, "AVX can only have 8 lanes of floats");

    public:
      using register_policy = avx_register;
      using self_type = Register<avx_register, float, N>;
      using element_type = float;
      using register_type = __m256;


    private:
      register_type m_value;

      RAJA_INLINE
      __m256i createMask() const {
        // Generate a mask
        return  _mm256_set_epi32(
            N >= 8 ? -1 : 0,
            N >= 7 ? -1 : 0,
            N >= 6 ? -1 : 0,
            N >= 5 ? -1 : 0,
            N >= 4 ? -1 : 0,
            N >= 3 ? -1 : 0,
            N >= 2 ? -1 : 0,
            -1);
      }

    public:

      /*!
       * @brief Default constructor, zeros register contents
       */
      RAJA_INLINE
      Register() : m_value(_mm256_setzero_ps()) {
      }

      /*!
       * @brief Copy constructor from underlying simd register
       */
      RAJA_INLINE
      constexpr
      explicit Register(register_type const &c) : m_value(c) {}


      /*!
       * @brief Copy constructor
       */
      RAJA_INLINE
      constexpr
      Register(self_type const &c) : m_value(c.m_value) {}


      /*!
       * @brief Construct from scalar.
       * Sets all elements to same value (broadcast).
       */
      RAJA_INLINE
      Register(element_type const &c) : m_value(_mm256_set1_ps(c)) {}


      /*!
       * @brief Strided load constructor, when scalars are located in memory
       * locations ptr, ptr+stride, ptr+2*stride, etc.
       *
       *
       * Note: this could be done with "gather" instructions if they are
       * available. (like in avx2, but not in avx)
       */
      RAJA_INLINE
      self_type &load(element_type const *ptr, camp::idx_t stride = 1){
        // Packed data is either a full, or masked load
        if(stride == 1){
          if(N == 8)
          {
            m_value = _mm256_loadu_ps(ptr);
          }
          else {
            m_value = _mm256_maskload_ps(ptr, createMask());
          }
        }
        // AVX has no gather instruction, so do it manually
        else{
          for(camp::idx_t i = 0;i < N;++ i){
            m_value[i] = ptr[i*stride];
          }
        }

        return *this;
      }



      /*!
       * @brief Strided store operation, where scalars are stored in memory
       * locations ptr, ptr+stride, ptr+2*stride, etc.
       *
       *
       * Note: this could be done with "scatter" instructions if they are
       * available.
       */
      RAJA_INLINE
      self_type const &store(element_type *ptr, camp::idx_t stride = 1) const{
        // Is this a packed store?
        if(stride == 1){
          // Is it full-width?
          if(N == 8){
            _mm256_storeu_ps(ptr, m_value);
          }
          // Need to do a masked store
          else{
            _mm256_maskstore_ps(ptr, createMask(), m_value);
          }

        }

        // Scatter operation:  AVX2 doesn't have a scatter, so it's manual
        else{
          for(camp::idx_t i = 0;i < N;++ i){
            ptr[i*stride] = m_value[i];
          }
        }
        return *this;
      }

      /*!
       * @brief Get scalar value from vector register
       * @param i Offset of scalar to get
       * @return Returns scalar value at i
       */
      template<typename IDX>
      constexpr
      RAJA_INLINE
      element_type get(IDX i) const
      {return m_value[i];}


      /*!
       * @brief Set scalar value in vector register
       * @param i Offset of scalar to set
       * @param value Value of scalar to set
       */
      template<typename IDX>
      RAJA_INLINE
      self_type &set(IDX i, element_type value)
      {
        m_value[i] = value;
        return *this;
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type &broadcast(element_type const &value){
        m_value =  _mm256_set1_ps(value);
        return *this;
      }


      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type &copy(self_type const &src){
        m_value = src.m_value;
        return *this;
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type add(self_type const &b) const {
        return self_type(_mm256_add_ps(m_value, b.m_value));
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type subtract(self_type const &b) const {
        return self_type(_mm256_sub_ps(m_value, b.m_value));
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type multiply(self_type const &b) const {
        return self_type(_mm256_mul_ps(m_value, b.m_value));
      }

      RAJA_HOST_DEVICE
      RAJA_INLINE
      self_type divide(self_type const &b) const {
        return self_type(_mm256_div_ps(m_value, b.m_value));
      }


      /*!
       * @brief Sum the elements of this vector
       * @return Sum of the values of the vectors scalar elements
       */
      RAJA_INLINE
      element_type sum() const
      {
        // Some simple cases
        if(N == 1){
          return m_value[0];
        }
        if(N == 2){
          return m_value[0]+m_value[1];
        }

        // swap odd-even pairs and add
        auto sh1 = _mm256_permute_ps(m_value, 0xB1);
        auto red1 = _mm256_add_ps(m_value, sh1);

        if(N == 3 || N == 4){
          return red1[0] + red1[2];
        }

        // swap odd-even quads and add
        auto sh2 = _mm256_permute_ps(red1, 0x4E);
        auto red2 = _mm256_add_ps(red1, sh2);

        return red2[0] + red2[4];
      }


      /*!
       * @brief Returns the largest element
       * @return The largest scalar element in the register
       */
      RAJA_INLINE
      element_type max() const
      {
        // Some simple cases
        if(N == 1){
          return m_value[0];
        }
        if(N == 2){
          return std::max<element_type>(m_value[0], m_value[1]);
        }

        // swap odd-even pairs and add
        auto sh1 = _mm256_permute_ps(m_value, 0xB1);

        if(N == 7){
          // blend out the 8th lane of the permute
          sh1 = _mm256_blend_ps(sh1, m_value, 0x40);
        }

        auto red1 = _mm256_max_ps(m_value, sh1);

        // Some more simple shortcuts
        if(N == 3){
          return std::max<element_type>(red1[0], m_value[2]);
        }


        // swap odd-even quads and add
        auto sh2 = _mm256_permute_ps(red1, 0x4E);
        auto red2 = _mm256_max_ps(red1, sh2);

        if(N == 4){
          return red2[0];
        }
        if(N == 5){
          return std::max<element_type>(red2[0], m_value[4]);
        }
        if(N == 6){
          return std::max<element_type>(red2[0], red1[4]);
        }

        // 7 or 8 lanes
        return std::max<element_type>(red2[0], red2[4]);
      }

      /*!
       * @brief Returns element-wise largest values
       * @return Vector of the element-wise max values
       */
      RAJA_INLINE
      self_type vmax(self_type a) const
      {
        return self_type(_mm256_max_ps(m_value, a.m_value));
      }

      /*!
       * @brief Returns the largest element
       * @return The largest scalar element in the register
       */
      RAJA_INLINE
      element_type min() const
      {
        // Some simple cases
        if(N == 1){
          return m_value[0];
        }
        if(N == 2){
          return std::min<element_type>(m_value[0], m_value[1]);
        }

        // swap odd-even pairs and add
        auto sh1 = _mm256_permute_ps(m_value, 0xB1);

        if(N == 7){
          // blend out the 8th lane of the permute
          sh1 = _mm256_blend_ps(sh1, m_value, 0x40);
        }

        auto red1 = _mm256_min_ps(m_value, sh1);

        // Some more simple shortcuts
        if(N == 3){
          return std::min<element_type>(red1[0], m_value[2]);
        }


        // swap odd-even quads and add
        auto sh2 = _mm256_permute_ps(red1, 0x4E);
        auto red2 = _mm256_min_ps(red1, sh2);

        if(N == 4){
          return red2[0];
        }
        if(N == 5){
          return std::min<element_type>(red2[0], m_value[4]);
        }
        if(N == 6){
          return std::min<element_type>(red2[0], red1[4]);
        }

        // 7 or 8 lanes
        return std::min<element_type>(red2[0], red2[4]);
      }

      /*!
       * @brief Returns element-wise largest values
       * @return Vector of the element-wise max values
       */
      RAJA_INLINE
      self_type vmin(self_type a) const
      {
        return self_type(_mm256_min_ps(m_value, a.m_value));
      }
  };



}  // namespace RAJA


#endif

#endif //__AVX2__
