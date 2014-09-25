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
value_seq::show (std::ostream &o, brevity brv) const
{
  o << "[";
  bool seen = false;
  for (auto const &v: *m_seq)
    {
      if (seen)
	o << ", ";
      seen = true;
      v->show (o, brevity::brief);
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

std::unique_ptr <value>
op_add_seq::operate (std::unique_ptr <value_seq> a,
		     std::unique_ptr <value_seq> b)
{
  auto seq = a->get_seq ();
  for (auto &v: *b->get_seq ())
    seq->push_back (std::move (v));
  return std::move (a);
}

std::unique_ptr <value>
op_length_seq::operate (std::unique_ptr <value_seq> a)
{
  constant t {a->get_seq ()->size (), &dec_constant_dom};
  return std::make_unique <value_cst> (t, 0);
}


namespace
{
  struct seq_elem_producer_base
  {
    std::shared_ptr <value_seq::seq_t> m_seq;
    size_t m_idx;

    seq_elem_producer_base (std::shared_ptr <value_seq::seq_t> seq)
      : m_seq {seq}
      , m_idx {0}
    {}
  };

  struct seq_elem_producer
    : public value_producer
    , public seq_elem_producer_base
  {
    using seq_elem_producer_base::seq_elem_producer_base;

    std::unique_ptr <value>
    next () override
    {
      if (m_idx < m_seq->size ())
	{
	  std::unique_ptr <value> v = (*m_seq)[m_idx]->clone ();
	  v->set_pos (m_idx++);
	  return v;
	}

      return nullptr;
    }
  };

  struct seq_relem_producer
    : public value_producer
    , public seq_elem_producer_base
  {
    using seq_elem_producer_base::seq_elem_producer_base;

    std::unique_ptr <value>
    next () override
    {
      if (m_idx < m_seq->size ())
	{
	  std::unique_ptr <value> v
	    = (*m_seq)[m_seq->size () - 1 - m_idx]->clone ();
	  v->set_pos (m_idx++);
	  return v;
	}

      return nullptr;
    }
  };
}

std::unique_ptr <value_producer>
op_elem_seq::operate (std::unique_ptr <value_seq> a)
{
  return std::make_unique <seq_elem_producer> (a->get_seq ());
}

std::unique_ptr <value_producer>
op_relem_seq::operate (std::unique_ptr <value_seq> a)
{
  return std::make_unique <seq_relem_producer> (a->get_seq ());
}

pred_result
pred_empty_seq::result (value_seq &a)
{
  return pred_result (a.get_seq ()->empty ());
}

pred_result
pred_find_seq::result (value_seq &haystack, value_seq &needle)
{
  auto const &hay = *haystack.get_seq ();
  auto const &need = *needle.get_seq ();
  return pred_result
    (std::search (hay.begin (), hay.end (),
		  need.begin (), need.end (),
		  [] (std::unique_ptr <value> const &a,
		      std::unique_ptr <value> const &b)
		  {
		    return a->cmp (*b) == cmp_result::equal;
		  }) != haystack.get_seq ()->end ());
}

pred_result
pred_starts_seq::result (value_seq &haystack, value_seq &needle)
{
  auto const &hay = *haystack.get_seq ();
  auto const &need = *needle.get_seq ();
  return pred_result
    (hay.size () >= need.size ()
     && std::equal (hay.begin (), std::next (hay.begin (), need.size ()),
		    need.begin (), need.end (),
		    [] (std::unique_ptr <value> const &a,
			std::unique_ptr <value> const &b)
		    {
		      return a->cmp (*b) == cmp_result::equal;
		    }));
}

pred_result
pred_ends_seq::result (value_seq &haystack, value_seq &needle)
{
  auto const &hay = *haystack.get_seq ();
  auto const &need = *needle.get_seq ();
  return pred_result
    (hay.size () >= need.size ()
     && std::equal (std::prev (hay.end (), need.size ()), hay.end (),
		    need.begin (), need.end (),
		    [] (std::unique_ptr <value> const &a,
			std::unique_ptr <value> const &b)
		    {
		      return a->cmp (*b) == cmp_result::equal;
		    }));
}
