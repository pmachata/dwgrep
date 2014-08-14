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

#ifndef _INT_H_
#define _INT_H_

#include <cstdint>
#include <iosfwd>

enum class signedness
  {
    unsign,
    sign,
  };

struct mpz_class
{
  uint64_t m_value;
  signedness m_sign;

  mpz_class ()
    : m_value {0}
    , m_sign {signedness::unsign}
  {}

  mpz_class (uint64_t value, signedness sign)
    : m_value {value}
    , m_sign {sign}
  {}

  mpz_class (mpz_class const &that) = default;

  mpz_class (uint64_t value)
    : mpz_class {value, signedness::unsign}
  {}

  mpz_class (uint32_t value)
    : mpz_class {static_cast <uint64_t> (value), signedness::unsign}
  {}

  mpz_class (int64_t value)
    : mpz_class {static_cast <uint64_t> (value), signedness::sign}
  {}

  mpz_class (int32_t value)
    : mpz_class {static_cast <uint64_t> (value), signedness::sign}
  {}

  void swap (mpz_class &that);
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

#endif /* _INT_H_ */
