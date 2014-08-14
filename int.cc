/*
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of dwgrep.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   dwgrep is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include <cassert>
#include <utility>
#include <iostream>

#include "int.hh"

namespace
{
  class overflow {};
}

void
mpz_class::swap (mpz_class &that)
{
  using std::swap;
  swap (m_value, that.m_value);
  swap (m_sign, that.m_sign);
}

std::ostream &
operator<< (std::ostream &o, mpz_class value)
{
  switch (value.m_sign)
    {
    case signedness::sign: return o << (int64_t)value.m_value;
    case signedness::unsign: return o << value.m_value;
    }
  __builtin_unreachable ();
}

bool
operator< (mpz_class v1, mpz_class v2)
{
  if (v1.m_sign == v2.m_sign)
    switch (v1.m_sign)
      {
      case signedness::unsign:
	return v1.m_value < v2.m_value;
      case signedness::sign:
	return (int64_t) v1.m_value < (int64_t) v2.m_value;
      }

  // If v1 is negative and v2 unsigned, v1 is certainly smaller.
  if (v1.m_sign == signedness::sign && (int64_t)v1.m_value < 0)
    return true;

  // And vice versa--if v2 is negative, v1 must be bigger.
  if (v2.m_sign == signedness::sign && (int64_t)v2.m_value < 0)
    return false;

  // Otherwise both are in the positive range and we can compare
  // directly.
  return v1.m_value < v2.m_value;
}

bool
operator== (mpz_class v1, mpz_class v2)
{
  return !(v1 < v2) && !(v2 < v1);
}

bool
operator<= (mpz_class v1, mpz_class v2)
{
  return v1 < v2 || !(v2 < v1);
}

bool
operator> (mpz_class v1, mpz_class v2)
{
  return !(v1 <= v2);
}

bool
operator>= (mpz_class v1, mpz_class v2)
{
  return !(v1 < v2);
}

bool
operator!= (mpz_class v1, mpz_class v2)
{
  return !(v1 == v2);
}

mpz_class
operator- (mpz_class v)
{
  switch (v.m_sign)
    {
    case signedness::sign:
      if ((int64_t) v.m_value == INT64_MIN)
	return mpz_class {(uint64_t) INT64_MAX + 1, signedness::unsign};
      return mpz_class {(uint64_t) -(int64_t) v.m_value, signedness::unsign};

    case signedness::unsign:
      if (v.m_value > (uint64_t) INT64_MAX + 1)
	throw overflow ();
      return mpz_class {-v.m_value, signedness::sign};
    }

  __builtin_unreachable ();
}

mpz_class
operator+ (mpz_class v1, mpz_class v2)
{
  if (v1.m_sign == v2.m_sign)
    switch (v1.m_sign)
      {
      case signedness::unsign:
	goto uns;
      case signedness::sign:
	{
	  int64_t a = (int64_t) v1.m_value;
	  int64_t b = (int64_t) v2.m_value;
	  if ((a <= 0 && b >= 0) || (b <= 0 && a >= 0))
	    // This will be unchanged at worst, but likely will move
	    // towards zero.  No overflow possible.
	    return mpz_class {a + b};

	  if (a >= 0 && b >= 0)
	    // These can be treated as unsigned.
	    goto uns;

	  if (a == INT64_MIN || b == INT64_MIN)
	    // At least one of them is minimum value and we know
	    // that the other is not zero, because we check for that
	    // up there.  Thus this has to be overflow.
	    throw overflow {};

	  uint64_t ua = -a;
	  uint64_t ub = -b;
	  uint64_t ur = ua + ub;
	  if (ur < ua || ur > (uint64_t) INT64_MAX + 1)
	    throw overflow ();
	  return mpz_class {-ur, signedness::sign};
	}
      }

  // If a is negative, a + b becomes b - -a.
  if (v1.m_sign == signedness::sign && (int64_t) v1.m_value < 0)
    return v2 - -v1;

  // If b is negative, a + b becomes a - -b.  And watch for overflow.
  if (v2.m_sign == signedness::sign && (int64_t) v2.m_value < 0)
    return v1 - -v2;

  // Otherwise even though one is signed, both are non-negative, and
  // we can just pretend both are unsigned.
 uns:
  auto result = v1.m_value + v2.m_value;
  if (result < v1.m_value)
    throw overflow ();
  return mpz_class {result, signedness::unsign};
}

mpz_class
operator- (mpz_class v1, mpz_class v2)
{
  // v1 >= && v2 >= 0
  if ((v1.m_sign == signedness::unsign || (int64_t) v1.m_value >= 0)
      && (v2.m_sign == signedness::unsign || (int64_t) v2.m_value >= 0))
    {
      if (v1.m_value > v2.m_value)
	return mpz_class {v1.m_value - v2.m_value};
      uint64_t r = v2.m_value - v1.m_value;
      if (r > (uint64_t) INT64_MAX + 1)
	throw overflow ();
      return mpz_class {-r, signedness::sign};
    }

  // v2 < 0 => v1 - v2 can be converted to v1 + -v2
  if (v2.m_sign == signedness::sign && (int64_t) v2.m_value < 0)
    return v1 + -v2;

  assert (!(v2 < 0));
  assert (v1 < 0);
  if (v2.m_value > v1.m_value - INT64_MIN)
    throw overflow ();
  return mpz_class {(uint64_t) ((int64_t) v1.m_value - (int64_t) v2.m_value),
		    signedness::sign};
}

mpz_class
operator* (mpz_class v1, mpz_class v2)
{
  // v1 < 0 && v2 < 0: a * b becomes -a * -b.
  if ((v1.m_sign == signedness::sign && (int64_t) v1.m_value < 0)
      && (v2.m_sign == signedness::sign && (int64_t) v2.m_value < 0))
    {
      v1 = -v1;
      v2 = -v2;
    }

  // v1 >= 0 && v2 >= 0
  if ((v1.m_sign == signedness::unsign || (int64_t) v1.m_value >= 0)
      && (v2.m_sign == signedness::unsign || (int64_t) v2.m_value >= 0))
    {
      uint64_t r = v1.m_value * v2.m_value;
      if (v1.m_value != 0 && r / v1.m_value != v2.m_value)
	throw overflow ();
      return mpz_class {r, signedness::unsign};
    }

  // v1 < 0
  if (v1.m_sign == signedness::sign && (int64_t) v1.m_value < 0)
    v1.swap (v2);

  // v2 < 0
  assert (v2.m_sign == signedness::sign);

  uint64_t a = (-v2).m_value;
  uint64_t r = a * v1.m_value;
  if (a != 0 && r / a != v1.m_value)
    throw overflow ();
  if (r > (uint64_t) INT64_MAX + 1)
    throw overflow ();
  return mpz_class {-r, signedness::sign};
}

#if 0

namespace
{
  template <class F>
  bool
  overflow_detected (F f)
  {
    try
      {
	f ();
	return false;
      }
    catch (overflow)
      {
	return true;
      }
  }
}

#define assert_overflows(F)			\
  assert (overflow_detected ([] () { F; }))

int
main(int argc, char *argv[])
{
  assert (mpz_class (UINT64_MAX) == mpz_class (UINT64_MAX));
  assert (mpz_class (UINT64_MAX) != mpz_class (-1));
  assert (mpz_class (-1) == mpz_class (-1));
  assert (mpz_class (0, signedness::sign) == mpz_class (0, signedness::unsign));
  assert (mpz_class (-2) < mpz_class (-1));
  assert (mpz_class (-2) < mpz_class (0U));
  assert (mpz_class (-2) < mpz_class (1U));

  assert (mpz_class (UINT64_MAX) == UINT64_MAX);
  assert (mpz_class (UINT64_MAX) != -1);
  assert (mpz_class (-1) == -1);
  assert (mpz_class (-2) < -1);
  assert (mpz_class (-2) < 0U);
  assert (mpz_class (-2) < 1U);
  assert (mpz_class (-2) < 0);


  assert (mpz_class (10U) + 15U == 25);
  assert (mpz_class (10U) + 5U == 15);
  assert (mpz_class (10U) + 15 == 25);
  assert (mpz_class (10U) + 5 == 15);
  assert (mpz_class (10U) + -15 == -5);
  assert (mpz_class (10U) + -5 == 5);

  assert (mpz_class (10) + 15U == 25);
  assert (mpz_class (10) + 5U == 15);
  assert (mpz_class (10) + 15 == 25);
  assert (mpz_class (10) + 5 == 15);
  assert (mpz_class (10) + -15 == -5);
  assert (mpz_class (10) + -5 == 5);

  assert (mpz_class (-10) + 15U == 5);
  assert (mpz_class (-10) + 5U == -5);
  assert (mpz_class (-10) + 15 == 5);
  assert (mpz_class (-10) + 5 == -5);
  assert (mpz_class (-10) + -15 == -25);
  assert (mpz_class (-10) + -5 == -15);


  assert (mpz_class (10U) - 15U == -5);
  assert (mpz_class (10U) - 5U == 5U);
  assert (mpz_class (10U) - 15 == -5);
  assert (mpz_class (10U) - 5 == 5U);
  assert (mpz_class (10U) - -15 == 25);
  assert (mpz_class (10U) - -5 == 15U);

  assert (mpz_class (10) - 15U == -5);
  assert (mpz_class (10) - 5U == 5U);
  assert (mpz_class (10) - 15 == -5);
  assert (mpz_class (10) - 5 == 5);
  assert (mpz_class (10) - -15 == 25);
  assert (mpz_class (10) - -5 == 15);

  assert (mpz_class (-10) - 15U == -25);
  assert (mpz_class (-10) - 5U == -15);
  assert (mpz_class (-10) - 15 == -25);
  assert (mpz_class (-10) - 5 == -15);
  assert (mpz_class (-10) - -15 == 5);
  assert (mpz_class (-10) - -5 == -5);


  assert (mpz_class (10U) * 15U == 150);
  assert (mpz_class (10U) * 15 == 150);
  assert (mpz_class (10U) * -15 == -150);
  assert (mpz_class (10U) * 0U == 0);
  assert (mpz_class (10U) * 0 == 0);
  assert (mpz_class (0U) * 10U == 0);
  assert (mpz_class (0) * 10U == 0);

  assert (mpz_class (10) * 15U == 150);
  assert (mpz_class (10) * 15 == 150);
  assert (mpz_class (10) * -15 == -150);
  assert (mpz_class (10) * 0U == 0);
  assert (mpz_class (10) * 0 == 0);
  assert (mpz_class (0U) * 10 == 0);
  assert (mpz_class (0) * 10 == 0);

  assert (mpz_class (-10) * 15U == -150);
  assert (mpz_class (-10) * 15 == -150);
  assert (mpz_class (-10) * -15 == 150);
  assert (mpz_class (-10) * 0U == 0);
  assert (mpz_class (-10) * 0 == 0);
  assert (mpz_class (-0U) * 10 == 0);
  assert (mpz_class (-0) * 10 == 0);


  // INT64_MIN is an even number, so the following should hold.
  assert (mpz_class (INT64_MIN / 2) + (INT64_MIN / 2) == INT64_MIN);

  assert (mpz_class (INT64_MAX) + INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (INT64_MAX) + (uint64_t) INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class ((uint64_t) INT64_MAX) + INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (INT64_MIN) + 1U == INT64_MIN + 1);
  assert (mpz_class (INT64_MIN) + 1 == INT64_MIN + 1);
  assert (mpz_class (1U) + INT64_MIN == INT64_MIN + 1);
  assert (mpz_class (1) + INT64_MIN == INT64_MIN + 1);

  assert (mpz_class (10U) - INT64_MIN == (uint64_t) INT64_MAX + 11);
  assert (mpz_class (INT64_MIN) - -10 == INT64_MIN + 10);

  assert (mpz_class (1) * INT64_MIN == INT64_MIN);
  assert (mpz_class (1U) * INT64_MIN == INT64_MIN);
  assert (mpz_class (-1) * INT64_MIN == (uint64_t) INT64_MAX + 1);
  assert (mpz_class (INT64_MIN) * 1 == INT64_MIN);
  assert (mpz_class (INT64_MIN) * 1U == INT64_MIN);
  assert (mpz_class (INT64_MIN) * -1 == (uint64_t) INT64_MAX + 1);
  assert (mpz_class (2) * INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (2U) * INT64_MAX == UINT64_MAX - 1);
  assert (mpz_class (INT64_MAX) * 2 == UINT64_MAX - 1);
  assert (mpz_class (INT64_MAX) * 2U == UINT64_MAX - 1);

  assert_overflows (mpz_class (UINT64_MAX) * -1);
  assert_overflows (mpz_class ((uint64_t) INT64_MAX + 2) * -1);

  return 0;
}
#endif
