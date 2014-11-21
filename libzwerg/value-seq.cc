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

value_seq
op_add_seq::operate (std::unique_ptr <value_seq> a,
		     std::unique_ptr <value_seq> b)
{
  auto seq = a->get_seq ();
  for (auto &v: *b->get_seq ())
    seq->push_back (std::move (v));
  return {seq, 0};
}

std::string
op_add_seq::docstring ()
{
  return R"docstring(

Concatenate two sequences and yield the resulting sequence::

	$ dwgrep '[1, 2, 3] [4, 5, 6] add'
	[1, 2, 3, 4, 5, 6]

Using sub-expression capture may be a more flexible alternative to
using ``add``::

	$ dwgrep '[1, 2, 3] [4, 5, 6] [7, 8, 9] [|A B C| (A, B, C) elem]'
	[1, 2, 3, 4, 5, 6, 7, 8, 9]

)docstring";
}

value_cst
op_length_seq::operate (std::unique_ptr <value_seq> a)
{
  return {constant {a->get_seq ()->size (), &dec_constant_dom}, 0};
}

std::string
op_length_seq::docstring ()
{
  return
R"docstring(

Yield number of elements of sequence on TOS.

E.g. the following tests whether the DIE's whose all attributes report
the same form as their abbreviations suggest, comprises all DIE's.
This test comes from dwgrep's test suite::

	[entry ([abbrev attribute label] == [attribute label])] length
	== [entry] length

(Note that this isn't anything that should be universally true, though
it typically will, and it is for the particular file that this test is
run on.  Attributes for which their abbreviation suggests
``DW_FORM_indirect`` will themselves have a different form.)

)docstring";
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
    : public value_producer <value>
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
    : public value_producer <value>
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

std::unique_ptr <value_producer <value>>
op_elem_seq::operate (std::unique_ptr <value_seq> a)
{
  return std::make_unique <seq_elem_producer> (a->get_seq ());
}

namespace
{
  char const *const elem_seq_docstring =
R"docstring(

For each element in the input sequence, which is popped, yield a stack
with that element pushed on top.

To zip contents of two top lists near TOS, do::

	$ dwgrep '[1, 2, 3] ["a", "b", "c"]
	          (|A B| A elem B elem) ?((|A B| A pos == B pos)) [|A B| A, B]'
	[1, a]
	[2, b]
	[3, c]

The first parenthesis enumerates all combinations of elements.  The
second then allows only those that correspond to each other
position-wise.  At that point we get three stacks, each with two
values.  The last bracket then packs the values on stacks back to
sequences, thus we get three stacks, each with a two-element sequence
on top.

The expression could be simplified a bit on expense of clarity::

	[1, 2, 3] ["a", "b", "c"]
	(|A B| A elem B elem (pos == drop pos)) [|A B| A, B]

``relem`` operates in the same fashion as ``elem``, but backwards.

)docstring";
}

std::string
op_elem_seq::docstring ()
{
  return elem_seq_docstring;
}

std::unique_ptr <value_producer <value>>
op_relem_seq::operate (std::unique_ptr <value_seq> a)
{
  return std::make_unique <seq_relem_producer> (a->get_seq ());
}

std::string
op_relem_seq::docstring ()
{
  return elem_seq_docstring;
}

pred_result
pred_empty_seq::result (value_seq &a)
{
  return pred_result (a.get_seq ()->empty ());
}

std::string
pred_empty_seq::docstring ()
{
return R"docstring(

Asserts that a sequence on TOS is empty.  This predicate holds for
both empty sequence literals as well as sequences that just happen to
be empty::

	$ dwgrep '[] ?empty'
	[]

	$ dwgrep 'dwgrep '[(1, 2, 3) (== 0)] ?empty'
	[]

)docstring";
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

std::string
pred_find_seq::docstring ()
{
return R"docstring(

(A B ?find) asserts that the sequence A contains sub-sequence B
(e.g. ``[hay stack] ?([needle] ?find)``).

To determine whether a sequence contains a particular element, you
would use the following construct::

	[that sequence] (elem == something)

E.g.::

	[child @AT_name] ?(elem == "foo")
	[child] ?(elem @AT_name == "foo")

To filter only those elements that match, you could do the\
following::

	[child] [|L| L elem ?(@AT_name == "foo")]

The above is suitable for a function that takes a list on input
and wants to filter it.  It is of course preferable to write this
sort of thing directly, if possible::

	[child ?(@AT_name == "foo")]

)docstring";
}

pred_result
pred_starts_seq::result (value_seq &haystack, value_seq &needle)
{
  auto const &hay = *haystack.get_seq ();
  auto const &need = *needle.get_seq ();
  return pred_result
    (hay.size () >= need.size ()
     && std::equal (hay.begin (), std::next (hay.begin (), need.size ()),
		    need.begin (),
		    [] (std::unique_ptr <value> const &a,
			std::unique_ptr <value> const &b)
		    {
		      return a->cmp (*b) == cmp_result::equal;
		    }));
}

std::string
pred_starts_seq::docstring ()
{
  return R"docstring(

``(A B ?starts)`` asserts that the sequence A starts with sub-sequence
B (e.g. ``[hay stack] ?([needle] ?starts)``).

)docstring";
}

pred_result
pred_ends_seq::result (value_seq &haystack, value_seq &needle)
{
  auto const &hay = *haystack.get_seq ();
  auto const &need = *needle.get_seq ();
  return pred_result
    (hay.size () >= need.size ()
     && std::equal (std::prev (hay.end (), need.size ()), hay.end (),
		    need.begin (),
		    [] (std::unique_ptr <value> const &a,
			std::unique_ptr <value> const &b)
		    {
		      return a->cmp (*b) == cmp_result::equal;
		    }));
}

std::string
pred_ends_seq::docstring ()
{
  return R"docstring(

``(A B ?ends)`` asserts that the sequence A ends with sub-sequence
B (e.g. ``[hay stack] ?([needle] ?ends)``).

)docstring";
}
