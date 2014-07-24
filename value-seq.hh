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

#ifndef _VALUE_SEQ_H_
#define _VALUE_SEQ_H_

#include "value.hh"
#include "op.hh"

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

  void show (std::ostream &o, bool full) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

struct op_add_seq
  : public stub_op
{
  using stub_op::stub_op;
  valfile::uptr next () override;
};

struct op_length_seq
  : public stub_op
{
  using stub_op::stub_op;
  valfile::uptr next () override;
};

struct op_elem_seq
  : public inner_op
{
  struct state;
  std::unique_ptr <state> m_state;

  op_elem_seq (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	       std::shared_ptr <scope> scope);
  ~op_elem_seq ();

  valfile::uptr next () override;
  std::string name () const override;
  void reset () override;
};

struct pred_empty_seq
  : public stub_pred
{
  using stub_pred::stub_pred;
  pred_result result (valfile &vf) override;
};

struct pred_find_seq
  : public stub_pred
{
  using stub_pred::stub_pred;
  pred_result result (valfile &vf) override;
};

#endif /* _VALUE_SEQ_H_ */
