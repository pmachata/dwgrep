/*
   Copyright (C) 2018 Petr Machata
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#ifndef _INT_H_
#define _INT_H_

#include <cassert>
#include <cstdint>
#include <iosfwd>

enum class signedness
  {
    unsign,
    sign,
  };

struct mpz_class
{
  union
  {
    uint64_t m_u;
    int64_t m_i;
  };

  signedness m_sign;

  mpz_class ()
    : m_u {0}
    , m_sign {signedness::unsign}
  {}

  mpz_class (uint64_t value, signedness sign)
    : m_u {value}
    , m_sign {sign}
  {}

  mpz_class (mpz_class const &that) = default;

  mpz_class (unsigned long long int value)
    : mpz_class {static_cast <uint64_t> (value), signedness::unsign}
  {}

  mpz_class (unsigned long int value)
    : mpz_class {static_cast <uint64_t> (value), signedness::unsign}
  {}

  mpz_class (unsigned int value)
    : mpz_class {static_cast <uint64_t> (value), signedness::unsign}
  {}

  mpz_class (long long int value)
    : mpz_class {static_cast <uint64_t> (value), signedness::sign}
  {}

  mpz_class (long int value)
    : mpz_class {static_cast <uint64_t> (value), signedness::sign}
  {}

  mpz_class (int value)
    : mpz_class {static_cast <uint64_t> (value), signedness::sign}
  {}

  uint64_t uval () const;
  int64_t sval () const;

  void swap (mpz_class &that);

  bool is_signed () const
  { return m_sign == signedness::sign; }

  bool is_unsigned () const
  { return m_sign == signedness::unsign; }
};

std::ostream &operator<< (std::ostream &o, mpz_class value);

bool operator< (mpz_class v1, mpz_class v2);
bool operator> (mpz_class v1, mpz_class v2);
bool operator<= (mpz_class v1, mpz_class v2);
bool operator>= (mpz_class v1, mpz_class v2);
bool operator== (mpz_class v1, mpz_class v2);
bool operator!= (mpz_class v1, mpz_class v2);

mpz_class operator- (mpz_class v);
mpz_class operator- (mpz_class v1, mpz_class v2);
mpz_class operator+ (mpz_class v1, mpz_class v2);
mpz_class operator* (mpz_class v1, mpz_class v2);
mpz_class operator/ (mpz_class v1, mpz_class v2);
mpz_class operator% (mpz_class v1, mpz_class v2);

mpz_class operator~ (mpz_class v);

inline uint64_t
mpz_class::uval () const
{
  assert (*this >= 0);
  return m_u;
}

inline int64_t
mpz_class::sval () const
{
  assert (m_sign == signedness::sign
	  || m_u <= INT64_MAX);
  return m_i;
}

#endif /* _INT_H_ */
