//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-19, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

///
/// Source file containing tests for basic simd/simt vector operations
///

#include "RAJA/RAJA.hpp"
#include "RAJA_gtest.hpp"

#include "./tensor-helper.hpp"

using RegisterTestTypes = ::testing::Types<
#ifdef RAJA_ENABLE_CUDA
    RAJA::Register<double, RAJA::cuda_warp_register>,
#endif
//    RAJA::VectorRegister<double, RAJA::avx2_register, 8>,
//    RAJA::VectorRegister<double, RAJA::avx_register, 5>,
//    RAJA::VectorRegister<double, RAJA::avx_register, 6>,
//    RAJA::VectorRegister<double, RAJA::avx_register, 7>,
//    RAJA::VectorRegister<double, RAJA::avx_register, 8>,
//    RAJA::VectorRegister<double, RAJA::avx_register, 9>
//    RAJA::VectorRegister<float>,
//    RAJA::VectorRegister<int>,
//    RAJA::VectorRegister<long>,

#ifdef __AVX__
//    RAJA::Register<double, RAJA::avx_register>,
//    RAJA::Register<float, RAJA::avx_register>,
//    RAJA::Register<int, RAJA::avx_register>,
//    RAJA::Register<long, RAJA::avx_register>,
#endif

#ifdef __AVX2__
    RAJA::Register<double, RAJA::avx2_register>,
//    RAJA::Register<float, RAJA::avx2_register>,
//    RAJA::Register<int, RAJA::avx2_register>,
//    RAJA::Register<long, RAJA::avx2_register>,
#endif

#ifdef __AVX512__
//    RAJA::Register<double, RAJA::avx512_register>,
//    RAJA::Register<float, RAJA::avx512_register>,
//    RAJA::Register<int, RAJA::avx512_register>,
//    RAJA::Register<long, RAJA::avx512_register>,
#endif

//    // scalar_register is supported on all platforms
//    RAJA::Register<double, RAJA::scalar_register>,
//    RAJA::Register<float, RAJA::scalar_register>,
//    RAJA::Register<int, RAJA::scalar_register>,
    RAJA::Register<long, RAJA::scalar_register>
  >;


template <typename RegisterType>
class RegisterTest : public ::testing::Test
{
public:

  RegisterTest() = default;
  virtual ~RegisterTest() = default;

  virtual void SetUp()
  {

  }

  virtual void TearDown()
  {

  }
};
TYPED_TEST_SUITE_P(RegisterTest);

#if 0
/*
 * We are using NO_OPT_RAND for input values so the compiler cannot do fancy
 * things, like constexpr out all of the intrinsics.
 */

GPU_TYPED_TEST_P(RegisterTest, GetSet)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem];
  register_t x;
  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
  }

  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(x.get(i), A[i]);
    ASSERT_SCALAR_EQ(x.get(i), A[i]);
  }

  // test copy construction
  register_t cc(x);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(cc.get(i), A[i]);
  }

  // test explicit copy
  register_t ce(0);
  ce.copy(x);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(ce.get(i), A[i]);
  }

  // test assignment
  register_t ca(0);
  ca = cc;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(ca.get(i), A[i]);
  }

  // test scalar construction (broadcast)
  register_t bc((element_t)5);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(bc.get(i), 5.0);
  }

  // test scalar assignment (broadcast)
  register_t ba((element_t)0);
  ba = (element_t)13.0;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(ba.get(i), 13.0);
  }

  // test explicit broadcast
  register_t be((element_t)0);
  be.broadcast((element_t)13.0);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(be.get(i), 13.0);
  }
}


TYPED_TEST_P(RegisterTest, Load)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem*2];
  for(size_t i = 0;i < num_elem*2; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
  }


  // load stride-1 from pointer
  register_t x;
  x.load_packed(A);

  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(x.get(i), A[i]);
  }


  // load n stride-1 from pointer
  if(num_elem > 1){
    x.load_packed_n(A, num_elem-1);

    // check first n-1 values
    for(size_t i = 0;i+1 < num_elem; ++ i){
      ASSERT_SCALAR_EQ(x.get(i), A[i]);
    }

    // last value should be cleared to zero
    ASSERT_SCALAR_EQ(x.get(num_elem-1), 0);
  }

  // load stride-2 from pointer
  register_t y;
  y.load_strided(A, 2);

  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(y.get(i), A[i*2]);
  }

  // load n stride-2 from pointer
  if(num_elem > 1){
    y.load_strided_n(A, 2, num_elem-1);

    // check first n-1 values
    for(size_t i = 0;i+1 < num_elem; ++ i){
      ASSERT_SCALAR_EQ(y.get(i), A[i*2]);
    }

    // last value should be cleared to zero
    ASSERT_SCALAR_EQ(y.get(num_elem-1), 0);
  }
}

TYPED_TEST_P(RegisterTest, Gather)
{

  using register_t = TypeParam;

  using int_vector_type = typename register_t::int_vector_type;

  using element_t = typename register_t::element_type;
  static constexpr camp::idx_t num_elem = register_t::s_num_elem;

  element_t A[num_elem*num_elem];
  for(camp::idx_t i = 0;i < num_elem*num_elem;++ i){
    A[i] = 3*i+13;
//    printf("A[%d]=%d\n", (int)i, (int)A[i]);
  }

  // create an index vector to point at sub elements of A
  int_vector_type idx;
  for(camp::idx_t i = 0;i < num_elem;++ i){
    int j = num_elem-1-i;
    idx.set(j*j, i);
//    printf("idx[%d]=%d\n", (int)i, (int)(j*j));
  }

  // Gather elements from A into a register using the idx offsets
  register_t x;
  x.gather(&A[0], idx);

  // check
  for(camp::idx_t i = 0;i < num_elem;++ i){
    int j = num_elem-1-i;
//    printf("i=%d, j=%d, A[%d]=%d, x.get(i)=%d\n",
//        (int)i, (int)j, (int)(j*j), (int)A[j*j], (int)x.get(i));
    ASSERT_SCALAR_EQ(A[j*j], x.get(i));
  }


  // Gather all but one elements from A into a register using the idx offsets
  register_t y;
  y.gather_n(&A[0], idx, num_elem-1);

  // check
  for(camp::idx_t i = 0;i < num_elem-1;++ i){
    int j = num_elem-1-i;
    ASSERT_SCALAR_EQ(A[j*j], y.get(i));
  }
  ASSERT_SCALAR_EQ(0, y.get(num_elem-1));


}

TYPED_TEST_P(RegisterTest, Scatter)
{

  using register_t = TypeParam;

  using int_vector_type = typename register_t::int_vector_type;

  using element_t = typename register_t::element_type;
  static constexpr camp::idx_t num_elem = register_t::s_num_elem;

  element_t A[num_elem*num_elem];
  for(camp::idx_t i = 0;i < num_elem*num_elem;++ i){
    A[i] = 0;
  }

  // create an index vector to point at sub elements of A
  int_vector_type idx;
  for(camp::idx_t i = 0;i < num_elem;++ i){
    int j = num_elem-1-i;
    idx.set(j*j, i);
  }

  // Create a vector of values
  register_t x;
  for(camp::idx_t i = 0;i < num_elem;++ i){
    x.set(i+1, i);
  }

  // Scatter the values of x into A[] using idx as the offsets
  x.scatter(&A[0], idx);

  // check
  for(camp::idx_t i = 0;i < num_elem*num_elem;++ i){
//    printf("A[%d]=%d\n", (int)i, (int)A[i]);
    // if the index i is in idx, check that A contains the right value
    for(camp::idx_t j = 0;j < num_elem;++ j){
      if(idx.get(j) == i){
        // check
        ASSERT_SCALAR_EQ(A[i], element_t(j+1));
        // and set to zero (for the next assert, and to clear for next test)
        A[i] = 0;
      }
    }
    // otherwise A should contain zero
    ASSERT_SCALAR_EQ(A[i], element_t(0));
  }


  // Scatter all but one of the values of x into A[] using idx as the offsets
  x.scatter_n(&A[0], idx, num_elem-1);

  // check
  for(camp::idx_t i = 0;i < num_elem*num_elem;++ i){
//    printf("A[%d]=%d\n", (int)i, (int)A[i]);
    // if the index i is in idx, check that A contains the right value
    for(camp::idx_t j = 0;j < num_elem-1;++ j){
      if(idx.get(j) == i){
        // check
        ASSERT_SCALAR_EQ(A[i], element_t(j+1));
        // and set to zero (for the next assert, and to clear for next test)
        A[i] = 0;
      }
    }
    // otherwise A should contain zero
    ASSERT_SCALAR_EQ(A[i], element_t(0));
  }


}


TYPED_TEST_P(RegisterTest, Add)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem];
  register_t x, y;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    B[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
    y.set(B[i], i);
  }

  // operator +
  register_t op_add = x+y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_add.get(i), A[i] + B[i]);
  }

  // operator +=
  register_t op_pluseq = x;
  op_pluseq += y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_pluseq.get(i), A[i] + B[i]);
  }

  // function add
  register_t func_add = x.add(y);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(func_add.get(i), A[i] + B[i]);
  }

  // operator + scalar
  register_t op_add_s1 = x + element_t(1);
  register_t op_add_s2 = element_t(1) + x;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_add_s1.get(i), A[i] + element_t(1));
    ASSERT_SCALAR_EQ(op_add_s2.get(i), element_t(1) + A[i]);
  }

  // operator += scalar
  register_t op_pluseq_s = x;
  op_pluseq_s += element_t(1);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_pluseq_s.get(i), A[i] + element_t(1));
  }

}





TYPED_TEST_P(RegisterTest, Subtract)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem];
  register_t x;
  register_t y;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    B[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
    y.set(B[i], i);
  }

  // operator -
  register_t op_sub = x-y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_sub.get(i), A[i] - B[i]);
  }

  // operator -=
  register_t op_subeq = x;
  op_subeq -= y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_subeq.get(i), A[i] - B[i]);
  }

  // function subtract
  register_t func_sub = x.subtract(y);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(func_sub.get(i), A[i] - B[i]);
  }

  // operator - scalar
  register_t op_sub_s1 = x - element_t(1);
  register_t op_sub_s2 = element_t(1) - x;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_sub_s1.get(i), A[i] - element_t(1));
    ASSERT_SCALAR_EQ(op_sub_s2.get(i), element_t(1) - A[i]);
  }

  // operator -= scalar
  register_t op_subeq_s = x;
  op_subeq_s -= element_t(1);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_subeq_s.get(i), A[i] - element_t(1));
  }
}

TYPED_TEST_P(RegisterTest, Multiply)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem];
  register_t x, y;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    B[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
    y.set(B[i], i);
  }

  // operator *
  register_t op_mul = x*y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_mul.get(i), (A[i] * B[i]));
  }

  // operator *=
  register_t op_muleq = x;
  op_muleq *= y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_muleq.get(i), A[i] * B[i]);
  }

  // function multiply
  register_t func_mul = x.multiply(y);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(func_mul.get(i), A[i] * B[i]);
  }

  // operator * scalar
  register_t op_mul_s1 = x * element_t(2);
  register_t op_mul_s2 = element_t(2) * x;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_mul_s1.get(i), A[i] * element_t(2));
    ASSERT_SCALAR_EQ(op_mul_s2.get(i), element_t(2) * A[i]);
  }

  // operator *= scalar
  register_t op_muleq_s = x;
  op_muleq_s *= element_t(2);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_muleq_s.get(i), A[i] * element_t(2));
  }
}

TYPED_TEST_P(RegisterTest, Divide)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem];
  register_t x, y;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0)+1.0;
    B[i] = (element_t)(NO_OPT_RAND*1000.0)+1.0;
    x.set(A[i], i);
    y.set(B[i], i);
  }

  // operator /
  register_t op_div = x/y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_div.get(i), A[i] / B[i]);
  }

  // operator /=
  register_t op_diveq = x;
  op_diveq /= y;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_diveq.get(i), A[i] / B[i]);
  }

  // function divide
  register_t func_div = x.divide(y);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(func_div.get(i), A[i] / B[i]);
  }


  // operator / scalar
  register_t op_div_s1 = x / element_t(2);
  register_t op_div_s2 = element_t(2) / x;
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_div_s1.get(i), A[i] / element_t(2));
    ASSERT_SCALAR_EQ(op_div_s2.get(i), element_t(2) / A[i]);
  }

  // operator /= scalar
  register_t op_diveq_s = x;
  op_diveq_s /= element_t(2);
  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(op_diveq_s.get(i), A[i] / element_t(2));
  }
}

TYPED_TEST_P(RegisterTest, DotProduct)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem];
  register_t x, y;

  element_t expected = 0.0;
  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    B[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
    y.set(B[i], i);
    expected += A[i]*B[i];
  }

  ASSERT_SCALAR_EQ(x.dot(y), expected);

}

TYPED_TEST_P(RegisterTest, FMA)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem], C[num_elem], expected[num_elem];
  register_t x, y, z, result;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    B[i] = (element_t)(NO_OPT_RAND*1000.0);
    C[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
    y.set(B[i], i);
    z.set(C[i], i);
    expected[i] = A[i]*B[i]+C[i];
  }

  result = x.multiply_add(y,z);

  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(result.get(i), expected[i]);
  }

}


TYPED_TEST_P(RegisterTest, FMS)
{

  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem], C[num_elem], expected[num_elem];
  register_t x, y, z, result;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    B[i] = (element_t)(NO_OPT_RAND*1000.0);
    C[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
    y.set(B[i], i);
    z.set(C[i], i);
    expected[i] = A[i]*B[i]-C[i];
  }

  result = x.multiply_subtract(y,z);

  for(size_t i = 0;i < num_elem; ++ i){
    ASSERT_SCALAR_EQ(result.get(i), expected[i]);
  }

}

TYPED_TEST_P(RegisterTest, Max)
{
  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  for(int iter = 0;iter < 100;++ iter){
    element_t A[num_elem], B[num_elem];
    register_t x, y;

    for(size_t i = 0;i < num_elem; ++ i){
      A[i] = -(element_t)(NO_OPT_RAND*1000.0);
      B[i] = -(element_t)(NO_OPT_RAND*1000.0);
      x.set(A[i], i);
      y.set(B[i], i);
    }

    // Check vector reduction
    element_t expected = A[0];
    for(size_t i = 1;i < num_elem;++ i){
      expected = expected > A[i] ? expected : A[i];
    }

//    printf("X=%s", x.to_string().c_str());

    ASSERT_SCALAR_EQ(x.max(), expected);


    // Check element-wise
    register_t z = x.vmax(y);
    for(size_t i = 1;i < num_elem;++ i){
      ASSERT_SCALAR_EQ(z.get(i), std::max<element_t>(A[i], B[i]));
    }


  }
}

TYPED_TEST_P(RegisterTest, Min)
{
  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  for(int iter = 0;iter < 100;++ iter){
    element_t A[num_elem], B[num_elem];
    register_t x, y;

    for(size_t i = 0;i < num_elem; ++ i){
      A[i] = (element_t)(NO_OPT_RAND*1000.0);
      B[i] = (element_t)(NO_OPT_RAND*1000.0);
      x.set(A[i], i);
      y.set(B[i], i);
    }

    // Check vector reduction
    element_t expected = A[0];
    for(size_t i = 1;i < num_elem;++ i){
      expected = expected < A[i] ? expected : A[i];
    }

//    printf("X=%s", x.to_string().c_str());


    ASSERT_SCALAR_EQ(x.min(), expected);

    // Check element-wise
    register_t z = x.vmin(y);
    for(size_t i = 1;i < num_elem;++ i){
      ASSERT_SCALAR_EQ(z.get(i), std::min<element_t>(A[i], B[i]));
    }

  }
}



TYPED_TEST_P(RegisterTest, SegmentedSum)
{
  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem],  R[num_elem];
  register_t x;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
  }

  printf("x: %s", x.to_string().c_str());

  // run segmented dot products for all segments allowed by the vector
  for(int segbits = 0;(1<<segbits) <= num_elem;++ segbits){

    register_t s = x.segmented_sum_outer(segbits,0);


    // Compute expected values
    for(size_t i = 0;i < num_elem; ++ i){
      R[i] = 0;
    }
    for(size_t i = 0;i < num_elem; ++ i){
      R[i>>segbits] += A[i];
    }

    printf("sum: segbits=%d, %s", segbits, s.to_string().c_str());

    for(size_t i = 0;i < num_elem; ++ i){
      ASSERT_SCALAR_EQ(R[i], s.get(i));
    }

  }

}

TYPED_TEST_P(RegisterTest, SegmentedDotProduct)
{
  using register_t = TypeParam;

  using element_t = typename register_t::element_type;
  static constexpr size_t num_elem = register_t::s_num_elem;

  element_t A[num_elem], B[num_elem], R[num_elem];
  register_t x, y;

  for(size_t i = 0;i < num_elem; ++ i){
    A[i] = (element_t)(NO_OPT_RAND*1000.0);
    B[i] = (element_t)(NO_OPT_RAND*1000.0);
    x.set(A[i], i);
    y.set(B[i], i);
  }

  printf("x: %s", x.to_string().c_str());
  printf("y: %s", y.to_string().c_str());


  // run segmented dot products for all segments allowed by the vector
  for(int segbits = 0;(1<<segbits) <= num_elem;++ segbits){

    int num_output_segments = 1<<segbits;

    for(int output_segment = 0;output_segment < num_output_segments;++output_segment){

      int offset = output_segment * num_elem/(1<<segbits);

      register_t dp = x.segmented_dot(segbits, output_segment, y);


      // Compute expected values
      for(size_t i = 0;i < num_elem; ++ i){
        R[i] = 0;
      }
      for(size_t i = 0;i < num_elem; ++ i){
        R[(i>>segbits) + offset] += A[i]*B[i];
      }

      printf("xdoty: segbits=%d, oseg=%d, %s", segbits, output_segment, dp.to_string().c_str());

      for(size_t i = 0;i < num_elem; ++ i){
        printf("i=%d, R=%lf, dp=%lf\n", (int)i, (double)R[i], (double)dp.get(i));
        ASSERT_SCALAR_EQ(R[i], dp.get(i));
      }

    } // output_segment

  } // segbits

}
#endif


GPU_TYPED_TEST_P(RegisterTest, SegmentedSumInner)
{
  using register_type = TypeParam;
  using element_type = typename register_type::element_type;
  using policy_type = typename register_type::register_policy;

  static constexpr camp::idx_t num_elem = register_type::s_num_elem;

  // Allocate

  std::vector<element_type> input0_vec(num_elem);
  element_type *input0_hptr = input0_vec.data();
  element_type *input0_dptr = tensor_malloc<policy_type, element_type>(num_elem);

  std::vector<element_type> output0_vec(num_elem);
  element_type *output0_hptr = output0_vec.data();
  element_type *output0_dptr = tensor_malloc<policy_type, element_type>(num_elem);


  // Initialize input data
//  printf("input: ");
  for(camp::idx_t i = 0;i < num_elem; ++ i){
    input0_hptr[i] = (element_type)(i+1); //+NO_OPT_RAND);
//    printf("%lf ", (double)input0_hptr[i]);
  }
//  printf("\n");
  tensor_copy_to_device<policy_type>(input0_dptr, input0_vec);



  // run segmented dot products for all segments allowed by the vector
  for(int segbits = 0;(1<<segbits) <= num_elem;++ segbits){

    int num_segments = 1<<segbits;

    for(int output_segment = 0;output_segment < num_segments;++ output_segment){
//      printf("segbits=%d, output_segment=%d\n", (int)segbits, (int)output_segment);

      // Execute segmented broadcast
      tensor_do<policy_type>([=] RAJA_HOST_DEVICE (){

        register_type x;
        x.load_packed(input0_dptr);

        register_type y = x.segmented_sum_inner(segbits, output_segment);

        y.store_packed(output0_dptr);

      });

      // Move result to host
      tensor_copy_to_host<policy_type>(output0_vec, output0_dptr);


      // Check result

      // Compute expected values
      element_type expected[num_elem];
      for(camp::idx_t i = 0;i < num_elem; ++ i){
        expected[i] = 0;
      }

      int output_offset = output_segment * num_elem>>segbits;

      // sum each value into appropriate segment lane
      for(camp::idx_t i = 0;i < num_elem; ++ i){

        auto off = (i >> segbits)+output_offset;

        expected[off] += input0_hptr[i];
      }


      printf("Expected: ");
      for(camp::idx_t i = 0;i < num_elem; ++ i){
        printf("%lf ", (double)expected[i]);
      }
      printf("\nResult:   ");
      for(camp::idx_t i = 0;i < num_elem; ++ i){
        printf("%lf ", (double)output0_hptr[i]);
      }
      printf("\n");

      for(camp::idx_t i = 0;i < num_elem; ++ i){

        ASSERT_SCALAR_EQ(expected[i], output0_hptr[i]);
      }

    } // segment

  } // segbits


  // Cleanup
  tensor_free<policy_type>(input0_dptr);
  tensor_free<policy_type>(output0_dptr);
}


GPU_TYPED_TEST_P(RegisterTest, SegmentedBroadcastInner)
{
  using register_type = TypeParam;
  using element_type = typename register_type::element_type;
  using policy_type = typename register_type::register_policy;

  static constexpr camp::idx_t num_elem = register_type::s_num_elem;

  // Allocate

  std::vector<element_type> input0_vec(num_elem);
  element_type *input0_hptr = input0_vec.data();
  element_type *input0_dptr = tensor_malloc<policy_type, element_type>(num_elem);

  std::vector<element_type> output0_vec(num_elem);
  element_type *output0_hptr = output0_vec.data();
  element_type *output0_dptr = tensor_malloc<policy_type, element_type>(num_elem);


  // Initialize input data
//  printf("input: ");
  for(camp::idx_t i = 0;i < num_elem; ++ i){
    input0_hptr[i] = (element_type)(i+1); //+NO_OPT_RAND);
//    printf("%lf ", (double)input0_hptr[i]);
  }
//  printf("\n");
  tensor_copy_to_device<policy_type>(input0_dptr, input0_vec);



  // run segmented dot products for all segments allowed by the vector
  for(int segbits = 0;(1<<segbits) <= num_elem;++ segbits){

    int num_segments = num_elem>>segbits;

    for(int input_segment = 0;input_segment < num_segments;++ input_segment){
//      printf("segbits=%d, input_segment=%d\n", (int)segbits, (int)input_segment);

      // Execute segmented broadcast
      tensor_do<policy_type>([=] RAJA_HOST_DEVICE (){

        register_type x;
        x.load_packed(input0_dptr);

        register_type y = x.segmented_broadcast_inner(segbits, input_segment);

        y.store_packed(output0_dptr);

      });

      // Move result to host
      tensor_copy_to_host<policy_type>(output0_vec, output0_dptr);


      // Check result

      // Compute expected values
      element_type expected[num_elem];

      camp::idx_t mask = (1<<segbits)-1;
      camp::idx_t offset = input_segment << segbits;

      // default implementation is dumb, just sum each value into
      // appropriate segment lane
//      printf("Expected: ");
      for(camp::idx_t i = 0;i < num_elem; ++ i){

        auto off = (i&mask) + offset;

        expected[i] = input0_hptr[off];

//        printf("%d ", (int)off);
        //printf("%lf ", (double)expected[i]);
      }
//      printf("\n");


//      printf("Result:   ");
//      for(camp::idx_t i = 0;i < num_elem; ++ i){
//        printf("%lf ", (double)output0_hptr[i]);
//      }
//      printf("\n");

      for(camp::idx_t i = 0;i < num_elem; ++ i){

        ASSERT_SCALAR_EQ(expected[i], output0_hptr[i]);
      }

    } // segment

  } // segbits


  // Cleanup
  tensor_free<policy_type>(input0_dptr);
  tensor_free<policy_type>(output0_dptr);
}

GPU_TYPED_TEST_P(RegisterTest, SegmentedBroadcastOuter)
{
  using register_type = TypeParam;
  using element_type = typename register_type::element_type;
  using policy_type = typename register_type::register_policy;

  static constexpr camp::idx_t num_elem = register_type::s_num_elem;

  // Allocate

  std::vector<element_type> input0_vec(num_elem);
  element_type *input0_hptr = input0_vec.data();
  element_type *input0_dptr = tensor_malloc<policy_type, element_type>(num_elem);

  std::vector<element_type> output0_vec(num_elem);
  element_type *output0_hptr = output0_vec.data();
  element_type *output0_dptr = tensor_malloc<policy_type, element_type>(num_elem);


  // Initialize input data
//  printf("input: ");
  for(camp::idx_t i = 0;i < num_elem; ++ i){
    input0_hptr[i] = (element_type)(i+1+NO_OPT_RAND);
//    printf("%lf ", (double)input0_hptr[i]);
  }
//  printf("\n");
  tensor_copy_to_device<policy_type>(input0_dptr, input0_vec);



  // run segmented dot products for all segments allowed by the vector
  for(int segbits = 0;(1<<segbits) <= num_elem;++ segbits){

    int num_segments = (1<<segbits);

    for(int input_segment = 0;input_segment < num_segments;++ input_segment){

      // Execute segmented broadcast
      tensor_do<policy_type>([=] RAJA_HOST_DEVICE (){

        register_type x;
        x.load_packed(input0_dptr);

        register_type y = x.segmented_broadcast_outer(segbits, input_segment);

        y.store_packed(output0_dptr);

      });

      // Move result to host
      tensor_copy_to_host<policy_type>(output0_vec, output0_dptr);


      // Check result

      // Compute expected values
//      printf("explode: segbits=%d, input_segment=%d\n", segbits, input_segment);
//      printf("  expected:  ");

      element_type expected[num_elem];
      for(camp::idx_t i = 0;i < num_elem; ++ i){
        int seg = i>>segbits;

        int off = (num_elem>>segbits)*input_segment + seg;

        expected[i] = input0_hptr[off];
//        printf("%lf ", (double)expected[i]);
      }
//      printf("\n");


      for(camp::idx_t i = 0;i < num_elem; ++ i){
        ASSERT_SCALAR_EQ(expected[i], output0_hptr[i]);
      }

    } // segment

  } // segbits


  // Cleanup
  tensor_free<policy_type>(input0_dptr);
  tensor_free<policy_type>(output0_dptr);
}






REGISTER_TYPED_TEST_SUITE_P(RegisterTest,
//                             GetSet,
//                             Load,
//                             Gather,
//                             Scatter,
//                             Add,
//                             Subtract,
//                             Multiply,
//                             Divide,
//                             DotProduct,
//                             FMA,
//                             FMS,
//                             Max,
//                             Min,

    SegmentedSumInner,
//    SegmentedSumOuter,
//                             SegmentedDotProduct,
    SegmentedBroadcastInner,
    SegmentedBroadcastOuter);

INSTANTIATE_TYPED_TEST_SUITE_P(SIMD, RegisterTest, RegisterTestTypes);




