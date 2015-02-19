/*
   Copyright (C) 2015 Red Hat, Inc.
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

#ifndef VALUE_ASET_H
#define VALUE_ASET_H

#include <memory>
#include "coverage.hh"
#include "value.hh"

// Set of addresses.
struct value_aset
  : public value
{
  coverage cov;

  static value_type const vtype;

  value_aset (coverage a_cov, size_t pos)
    : value {vtype, pos}
    , cov {std::move (a_cov)}
  {}

  value_aset (value_aset const &that) = default;

  coverage &get_coverage ()
  { return cov; }

  coverage const &get_coverage () const
  { return cov; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

#endif /* VALUE_ASET_H */
