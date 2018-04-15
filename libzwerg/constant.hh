/*
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

#ifndef _CONSTANT_H_
#define _CONSTANT_H_

#include <cassert>
#include <iosfwd>
#include <cstdint>

#include "int.hh"

class constant;

enum class brevity
  {
    full,
    brief,
  };

typedef class zw_cdom constant_dom;
class zw_cdom
{
public:
  virtual ~zw_cdom () {}
  virtual void show (mpz_class const &c, std::ostream &o,
		     brevity brv) const = 0;
  virtual char const *name () const = 0;

  // Whether this domain is considered safe for integer arithmetic.
  // (E.g. named constants aren't.)
  virtual bool safe_arith () const { return false; }

  // When doing arithmetic, whether this domain is considered plain
  // and should be given up for the other one.  (E.g. hex domain
  // wouldn't.)
  virtual bool plain () const { return false; }

  // Some domains form a hierarchy of sub-classes.  An example would
  // be arch-specific domains for ELF symbols.  E.g. STT_ARM_TFUNC and
  // STT_SPARC_REGISTER both have value of 13, so they have to be
  // represented by different domains.  For reasons of practicality,
  // both of these domains cover the common cases as well.  That means
  // there could be two, say, STT_FUNC's, each coming from a different
  // domain.  It is still desirable to have them compare equal.
  //
  // most_enclosing returns either itself, or a domain that best fits
  // the constant in question.  E.g.
  //    domain$STT_ARM->most_enclosing (STT_FUNC)
  // would return domain$STT, because there's nothing ARM-specific on
  // that constant.
  virtual constant_dom const *most_enclosing (mpz_class const &c) const
  { return this; }

  virtual char const *docstring () const
  { return ""; }
};

struct numeric_constant_dom_t
  : public constant_dom
{
  char const *m_name;

public:
  explicit numeric_constant_dom_t (char const *name)
    : m_name {name}
  {}

  void show (mpz_class const &v, std::ostream &o, brevity brv) const override;
  bool safe_arith () const override { return true; }
  bool plain () const override { return true; }

  char const *
  name () const override
  {
    return m_name;
  }
};

// Domains for unnamed constants.
extern constant_dom const &dec_constant_dom;
extern constant_dom const &hex_constant_dom;
extern constant_dom const &oct_constant_dom;
extern constant_dom const &bin_constant_dom;

// A domain for boolean-formatted unsigned values.
extern constant_dom const &bool_constant_dom;

extern constant_dom const &column_number_dom;
extern constant_dom const &line_number_dom;

class constant
{
  mpz_class m_value;
  constant_dom const *m_dom;
  brevity m_brv;

public:
  constant ()
    : constant (0, nullptr)
  {}

  template <class T>
  constant (T const &value, constant_dom const *dom,
	    brevity brv = brevity::full)
    : m_value (value)
    , m_dom {dom}
    , m_brv {brv}
  {}

  constant (constant const &copy) = default;

  constant_dom const *dom () const
  {
    return m_dom;
  }

  mpz_class const &value () const
  {
    return m_value;
  }

  bool operator< (constant that) const;
  bool operator> (constant that) const;
  bool operator<= (constant that) const;
  bool operator>= (constant that) const;
  bool operator== (constant that) const;
  bool operator!= (constant that) const;

  friend std::ostream &operator<< (std::ostream &, constant);
};

std::ostream &operator<< (std::ostream &o, constant cst);

void check_arith (constant const &cst_a, constant const &cst_b);
void check_arith (constant const &cst_a);

inline const char *
abbreviate (char const *name, size_t prefix_len, brevity brv)
{
  if (name != nullptr)
    return name + (brv == brevity::full ? 0 : prefix_len);
  else
    return nullptr;
}

#endif /* _CONSTANT_H_ */
