/*
   Copyright (C) 2018 Petr Machata
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
#include <stdexcept>
#include <sstream>

#include "int.hh"

namespace
{
  std::string
  describe_error (char const *what, mpz_class a, mpz_class b, char op)
  {
    std::stringstream ss;
    ss << what << " occured when computing " << a << op << b;
    return ss.str ();
  }

  std::string
  describe_overflow (mpz_class a, mpz_class b, char op)
  {
    return describe_error ("overflow", a, b, op);
  }

  std::string
  describe_overflow (mpz_class a, char op)
  {
    std::stringstream ss;
    ss << "overflow occured when computing " << op << a;
    return ss.str ();
  }

  std::string
  describe_div_0 (mpz_class a, mpz_class b, char op)
  {
    return describe_error ("division by zero", a, b, op);
  }

  void
  int_error (std::string &&reason)
  {
    throw std::domain_error (std::move (reason));
  }
}

void
mpz_class::swap (mpz_class &that)
{
  using std::swap;
  swap (m_u, that.m_u);
  swap (m_sign, that.m_sign);
}

std::ostream &
operator<< (std::ostream &o, mpz_class value)
{
  if (value < 0)
    return o << '-' << -value;
  else
    return o << value.m_u;
}

bool
operator< (mpz_class v1, mpz_class v2)
{
  if (v1.m_sign == v2.m_sign)
    switch (v1.m_sign)
      {
      case signedness::unsign:
	return v1.m_u < v2.m_u;
      case signedness::sign:
	return v1.m_i < v2.m_i;
      }

  // If v1 is negative and v2 unsigned, v1 is certainly smaller.
  if (v1.m_sign == signedness::sign && v1.m_i < 0)
    return true;

  // And vice versa--if v2 is negative, v1 must be bigger.
  if (v2.m_sign == signedness::sign && v2.m_i < 0)
    return false;

  // Otherwise both are in the positive range and we can compare
  // directly.
  return v1.m_u < v2.m_u;
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
      if (v.m_i == INT64_MIN)
	return mpz_class {(uint64_t) INT64_MAX + 1, signedness::unsign};
      return mpz_class {(uint64_t) -v.m_i, signedness::unsign};

    case signedness::unsign:
      if (v.m_u > (uint64_t) INT64_MAX + 1)
	int_error (describe_overflow (v, '-'));
      return mpz_class {-v.m_u, signedness::sign};
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
	  int64_t a = v1.m_i;
	  int64_t b = v2.m_i;
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
	    int_error (describe_overflow (v1, v2, '+'));

	  uint64_t ua = -a;
	  uint64_t ub = -b;
	  uint64_t ur = ua + ub;
	  if (ur < ua || ur > (uint64_t) INT64_MAX + 1)
	    int_error (describe_overflow (v1, v2, '+'));
	  return mpz_class {-ur, signedness::sign};
	}
      }

  // If a is negative, a + b becomes b - -a.
  if (v1.m_sign == signedness::sign && v1.m_i < 0)
    return v2 - -v1;

  // If b is negative, a + b becomes a - -b.  And watch for overflow.
  if (v2.m_sign == signedness::sign && v2.m_i < 0)
    return v1 - -v2;

  // Otherwise even though one is signed, both are non-negative, and
  // we can just pretend both are unsigned.
 uns:
  auto result = v1.m_u + v2.m_u;
  if (result < v1.m_u)
    int_error (describe_overflow (v1, v2, '+'));
  return mpz_class {result, signedness::unsign};
}

mpz_class
operator- (mpz_class v1, mpz_class v2)
{
  // v1 >= && v2 >= 0
  if ((v1.m_sign == signedness::unsign || v1.m_i >= 0)
      && (v2.m_sign == signedness::unsign || v2.m_i >= 0))
    {
      if (v1.m_u > v2.m_u)
	return mpz_class {v1.m_u - v2.m_u};
      uint64_t r = v2.m_u - v1.m_u;
      if (r > (uint64_t) INT64_MAX + 1)
	int_error (describe_overflow (v1, v2, '-'));
      return mpz_class {-r, signedness::sign};
    }

  // v2 < 0 => v1 - v2 can be converted to v1 + -v2
  if (v2.m_sign == signedness::sign && v2.m_i < 0)
    return v1 + -v2;

  assert (!(v2 < 0));
  assert (v1 < 0);
  if (v2.m_u > v1.m_u - INT64_MIN)
    int_error (describe_overflow (v1, v2, '-'));
  return mpz_class {(uint64_t) (v1.m_i - v2.m_i),
		    signedness::sign};
}

mpz_class
operator* (mpz_class v1, mpz_class v2)
{
  // v1 < 0 && v2 < 0: a * b becomes -a * -b.
  if ((v1.m_sign == signedness::sign && v1.m_i < 0)
      && (v2.m_sign == signedness::sign && v2.m_i < 0))
    {
      v1 = -v1;
      v2 = -v2;
    }

  // v1 >= 0 && v2 >= 0
  if ((v1.m_sign == signedness::unsign || v1.m_i >= 0)
      && (v2.m_sign == signedness::unsign || v2.m_i >= 0))
    {
      uint64_t r = v1.m_u * v2.m_u;
      if (v1.m_u != 0 && r / v1.m_u != v2.m_u)
	int_error (describe_overflow (v1, v2, '*'));
      return mpz_class {r, signedness::unsign};
    }

  // v1 < 0
  if (v1.m_sign == signedness::sign && v1.m_i < 0)
    v1.swap (v2);

  uint64_t a = (-v2).m_u;
  uint64_t r = a * v1.m_u;
  if (a != 0 && r / a != v1.m_u)
    int_error (describe_overflow (v1, v2, '*'));
  if (r > (uint64_t) INT64_MAX + 1)
    int_error (describe_overflow (v1, v2, '*'));
  return mpz_class {-r, signedness::sign};
}

mpz_class
operator/ (mpz_class v1, mpz_class v2)
{
  if (v2.m_u == 0)
    int_error (describe_div_0 (v1, v2, '/'));

  bool neg = false;
  if (v1 < 0)
    {
      v1 = -v1;
      neg = true;
    }
  if (v2 < 0)
    {
      v2 = -v2;
      neg = ! neg;
    }

  if (neg)
    v1 = v1 + (v2 - 1);

  mpz_class ret {v1.m_u / v2.m_u, signedness::unsign};
  if (neg)
    ret = -ret;

  return ret;
}

mpz_class
operator% (mpz_class v1, mpz_class v2)
{
  if (v2.m_u == 0)
    int_error (describe_div_0 (v1, v2, '%'));

  mpz_class d = v1 / v2;
  return v1 - v2 * d;
}

mpz_class
operator~ (mpz_class v)
{
  // There are really three cases: negative numbers, small positive numbers (up
  // to the value of INT64_MAX) and large positive numbers. Negation of a large
  // positive number or a negative number ends up being a small positive number.
  // The only concern is what to do with negation of small positive numbers.
  //
  // We could take the signedness of the operand and decide based on
  // that--negation of signed small positive number would be a negative number,
  // whereas negation of unsigned small positive number would be a large
  // positive number.
  //
  // But that would betray number of bits that we operate with, and the whole
  // arithmetic package is built around the theory that you are not supposed to
  // care (mpz_class used to be an actual GNU MP arbitrary-precision class).
  //
  // So instead we simply pretend all negated numbers are signed. That's
  // actually a correct answer for negation of large positive numbers, negative
  // numbers, and signed small positive numbers, and for negation of unsigned
  // small positive numbers we force signed number.
  return mpz_class {~v.m_u, signedness::sign};
}

mpz_class
operator& (mpz_class v1, mpz_class v2)
{
  // It's not clear what signedness to use when a large positive number is and'd
  // with a negative number. The two numbers may be bit-for-bit equal and the
  // only thing that differs is their signedness. Python chooses to keep the
  // result signed if both operands are signed for and'ing, and if either
  // operand is signed when or'ing. Do likewise.
  auto sig = v1.m_sign == signedness::sign && v2.m_sign == signedness::sign
    ? signedness::sign : signedness::unsign;
  return mpz_class {v1.m_u & v2.m_u, sig};
}

mpz_class
operator| (mpz_class v1, mpz_class v2)
{
  auto sig = v1.m_sign == signedness::sign || v2.m_sign == signedness::sign
    ? signedness::sign : signedness::unsign;
  return mpz_class {v1.m_u | v2.m_u, sig};
}

mpz_class
operator^ (mpz_class v1, mpz_class v2)
{
  auto sig = v1.m_sign != v2.m_sign
    ? signedness::sign : signedness::unsign;
  return mpz_class {v1.m_u ^ v2.m_u, sig};
}
