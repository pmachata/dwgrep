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

#include <memory>
#include "make_unique.hh"
#include <iostream>
#include <algorithm>

#include "value-seq.hh"
#include "overload.hh"
#include "value-cst.hh"

value_type const value_seq::vtype = value_type::alloc ("T_SEQ");

namespace
{
  value_seq::seq_t
  clone_seq (value_seq::seq_t const &seq)
  {
    value_seq::seq_t seq2;
    for (auto const &v: seq)
      seq2.emplace_back (std::move (v->clone ()));
    return seq2;
  }
}

value_seq::value_seq (value_seq const &that)
  : value {that}
  , m_seq {std::make_shared <seq_t> (clone_seq (*that.m_seq))}
{}

void
value_seq::show (std::ostream &o, bool full) const
{
  o << "[";
  bool seen = false;
  for (auto const &v: *m_seq)
    {
      if (seen)
	o << ", ";
      seen = true;
      v->show (o, false);
    }
  o << "]";
}

std::unique_ptr <value>
value_seq::clone () const
{
  return std::make_unique <value_seq> (*this);
}

namespace
{
  template <class Callable>
  cmp_result
  compare_sequences (value_seq::seq_t const &sa, value_seq::seq_t const &sb,
		     Callable cmp)
  {
    cmp_result ret = cmp_result::fail;
    auto mm = std::mismatch (sa.begin (), sa.end (), sb.begin (),
			     [&ret, cmp] (std::unique_ptr <value> const &a,
					  std::unique_ptr <value> const &b)
			     {
			       ret = cmp (a, b);
			       assert (ret != cmp_result::fail);
			       return ret == cmp_result::equal;
			     });

    if (mm.first != sa.end ())
      {
	assert (mm.second != sb.end ());
	assert (ret != cmp_result::fail);
	return ret;
      }

    return cmp_result::equal;
  }
}

cmp_result
value_seq::cmp (value const &that) const
{
  if (auto v = value::as <value_seq> (&that))
    {
      cmp_result ret = compare (m_seq->size (), v->m_seq->size ());
      if (ret != cmp_result::equal)
	return ret;

      ret = compare_sequences (*m_seq, *v->m_seq,
			       [] (std::unique_ptr <value> const &a,
				   std::unique_ptr <value> const &b)
			       {
				 return compare (a->get_type (),
						 b->get_type ());
			       });
      if (ret != cmp_result::equal)
	return ret;

      return compare_sequences (*m_seq, *v->m_seq,
				[] (std::unique_ptr <value> const &a,
				    std::unique_ptr <value> const &b)
				{ return a->cmp (*b); });
    }
  else
    return cmp_result::fail;
}

valfile::uptr
op_add_seq::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_seq> ();
      auto wp = vf->pop_as <value_seq> ();

      value_seq::seq_t res;
      for (auto const &x: *wp->get_seq ())
	res.emplace_back (x->clone ());
      for (auto const &x: *vp->get_seq ())
	res.emplace_back (x->clone ());

      vf->push (std::make_unique <value_seq> (std::move (res), 0));
      return vf;
    }

  return nullptr;
}

valfile::uptr
op_length_seq::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_seq> ();
      constant t {vp->get_seq ()->size (), &dec_constant_dom};
      vf->push (std::make_unique <value_cst> (t, 0));
      return vf;
    }

  return nullptr;
}


struct op_elem_seq::state
{
  valfile::uptr m_base;
  std::shared_ptr <value_seq::seq_t> m_seq;
  size_t m_idx;

  state (valfile::uptr base, std::shared_ptr <value_seq::seq_t> seq)
    : m_base {std::move (base)}
    , m_seq {seq}
    , m_idx {0}
  {}

  valfile::uptr
  next ()
  {
    if (m_idx < m_seq->size ())
      {
	std::unique_ptr <value> v = (*m_seq)[m_idx]->clone ();
	v->set_pos (m_idx);
	m_idx++;
	valfile::uptr vf = std::make_unique <valfile> (*m_base);
	vf->push (std::move (v));
	return vf;
      }

    return nullptr;
  }
};

op_elem_seq::op_elem_seq (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope)
  : inner_op {upstream}
{}

op_elem_seq::~op_elem_seq ()
{}

valfile::uptr
op_elem_seq::next ()
{
  while (true)
    {
      if (m_state == nullptr)
	{
	  if (auto vf = m_upstream->next ())
	    {
	      auto vp = vf->pop_as <value_seq> ();
	      m_state = std::make_unique <state> (std::move (vf),
						  vp->get_seq ());
	    }
	  else
	    return nullptr;
	}

      if (auto vf = m_state->next ())
	return vf;

      m_state = nullptr;
    }
}

std::string
op_elem_seq::name () const
{
  return "elem_seq";
}

void
op_elem_seq::reset ()
{
  m_state = nullptr;
  inner_op::reset ();
}

pred_result
pred_empty_seq::result (valfile &vf)
{
  auto vp = vf.top_as <value_seq> ();
  return pred_result (vp->get_seq ()->empty ());
}

pred_result
pred_find_seq::result (valfile &vf)
{
  auto needle = vf.get_as <value_seq> (0)->get_seq ();
  auto haystack = vf.get_as <value_seq> (1)->get_seq ();
  return pred_result (std::search (haystack->begin (), haystack->end (),
				   needle->begin (), needle->end (),
				   [] (std::unique_ptr <value> const &a,
				       std::unique_ptr <value> const &b)
				   {
				     return a->cmp (*b) == cmp_result::equal;
				   }) != haystack->end ());
}
