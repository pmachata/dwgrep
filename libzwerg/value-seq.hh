/*
   Copyright (C) 2017 Petr Machata
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

#ifndef _VALUE_SEQ_H_
#define _VALUE_SEQ_H_

#include "value.hh"
#include "op.hh"
#include "overload.hh"
#include "value-cst.hh"

class value_seq
  : public value
{
public:
  typedef std::vector <std::unique_ptr <value> > seq_t;

private:
  std::shared_ptr <seq_t> m_seq;

public:
  static value_type const vtype;

  value_seq (seq_t &&seq, size_t pos)
    : value {vtype, pos}
    , m_seq {std::make_shared <seq_t> (std::move (seq))}
  {}

  value_seq (std::shared_ptr <seq_t> seqp, size_t pos)
    : value {vtype, pos}
    , m_seq {seqp}
  {}

  value_seq (value_seq const &that);

  std::shared_ptr <seq_t>
  get_seq () const
  {
    return m_seq;
  }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

struct op_add_seq
  : public op_once_overload <value_seq, value_seq, value_seq>
{
  using op_once_overload::op_once_overload;

  value_seq operate (std::unique_ptr <value_seq> a,
		     std::unique_ptr <value_seq> b) const override;

  static std::string docstring ();
};

struct op_length_seq
  : public op_once_overload <value_cst, value_seq>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_seq> a) const override;

  static std::string docstring ();
};

struct op_elem_seq
  : public op_yielding_overload <value, value_seq>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value>>
  operate (std::unique_ptr <value_seq> a) const override;

  static std::string docstring ();
};

struct op_relem_seq
  : public op_yielding_overload <value, value_seq>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value>>
  operate (std::unique_ptr <value_seq> a) const override;

  static std::string docstring ();
};

struct pred_empty_seq
  : public pred_overload <value_seq>
{
  using pred_overload::pred_overload;
  pred_result result (value_seq &a) const override;

  static std::string docstring ();
};

struct pred_find_seq
  : public pred_overload <value_seq, value_seq>
{
  using pred_overload::pred_overload;
  pred_result result (value_seq &haystack, value_seq &needle) const override;

  static std::string docstring ();
};

struct pred_starts_seq
  : public pred_overload <value_seq, value_seq>
{
  using pred_overload::pred_overload;
  pred_result result (value_seq &haystack, value_seq &needle) const override;

  static std::string docstring ();
};

struct pred_ends_seq
  : public pred_overload <value_seq, value_seq>
{
  using pred_overload::pred_overload;
  pred_result result (value_seq &haystack, value_seq &needle) const override;

  static std::string docstring ();
};

#endif /* _VALUE_SEQ_H_ */
