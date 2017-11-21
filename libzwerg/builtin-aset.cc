/*
   Copyright (C) 2017 Petr Machata
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

#include "builtin-aset.hh"
#include "dwcst.hh"

namespace
{
  struct elem_aset_producer
    : public value_producer <value_cst>
  {
    coverage cov;
    size_t m_idx;	// position among ranges
    uint64_t m_ai;	// iteration through a range
    size_t m_i;		// produced value counter
    bool m_forward;

    elem_aset_producer (coverage a_cov, bool forward)
      : cov {a_cov}
      , m_idx {0}
      , m_ai {0}
      , m_i {0}
      , m_forward {forward}
    {}

    std::unique_ptr <value_cst>
    next () override
    {
      if (m_idx >= cov.size ())
	return nullptr;

      auto idx = [&] () {
	return m_forward ? m_idx : cov.size () - 1 - m_idx;
      };

      if (m_ai >= cov.at (idx ()).length)
	{
	  m_idx++;
	  if (m_idx >= cov.size ())
	    return nullptr;
	  assert (cov.at (idx ()).length > 0);
	  m_ai = 0;
	}

      uint64_t ai = m_forward ? m_ai : cov.at (idx ()).length - 1 - m_ai;
      uint64_t addr = cov.at (idx ()).start + ai;
      m_ai++;

      return std::make_unique <value_cst>
	(constant {addr, &dw_address_dom ()}, m_i++);
    }
  };

  const char elem_aset_docstring[] =
R"docstring(

Take an address set on TOS and yield all its addresses.  Be warned
that this may be a very expensive operation.  It is common that
address sets cover the whole address range, which for a 64-bit
architecture is a whole lot of addresses.

Example::

	$ dwgrep ./tests/testfile_const_type -e 'entry (name == "main") address'
	[0x80482f0, 0x80482f3)

	$ dwgrep ./tests/testfile_const_type -e 'entry (name == "main") address elem'
	0x80482f0
	0x80482f1
	0x80482f2

``relem`` behaves similarly, but yields addresses in reverse order.

)docstring";
}

std::unique_ptr <value_producer <value_cst>>
op_elem_aset::operate (std::unique_ptr <value_aset> val) const
{
  return std::make_unique <elem_aset_producer>
    (val->get_coverage (), true);
}

std::string
op_elem_aset::docstring ()
{
  return elem_aset_docstring;
}

std::unique_ptr <value_producer <value_cst>>
op_relem_aset::operate (std::unique_ptr <value_aset> val) const
{
  return std::make_unique <elem_aset_producer>
    (val->get_coverage (), false);
}

std::string
op_relem_aset::docstring ()
{
  return elem_aset_docstring;
}

std::unique_ptr <value_cst>
op_low_aset::operate (std::unique_ptr <value_aset> a) const
{
  auto &cov = a->get_coverage ();
  if (cov.empty ())
    return nullptr;

  return std::make_unique <value_cst>
    (constant {cov.at (0).start, &dw_address_dom ()}, 0);
}

std::string
op_low_aset::docstring ()
{
  return
R"docstring(

Takes an address set on TOS and yields lowest address set in this set.
Doesn't yield at all if the set is empty::

	$ dwgrep ./tests/aranges.o -e 'entry @AT_location'
	0x10000..0x10010:[0:reg5]
	0x10010..0x1001a:[0:fbreg<-24>]

	$ dwgrep ./tests/aranges.o -e 'entry @AT_location address low'
	0x10000
	0x10010

)docstring";
}


std::unique_ptr <value_cst>
op_high_aset::operate (std::unique_ptr <value_aset> a) const
{
  auto &cov = a->get_coverage ();
  if (cov.empty ())
    return nullptr;

  auto range = cov.at (cov.size () - 1);

  return std::make_unique <value_cst>
    (constant {range.start + range.length, &dw_address_dom ()}, 0);
}

std::string
op_high_aset::docstring ()
{
  return
R"docstring(

Takes an address set on TOS and yields highest address of this set.
Note that that address doesn't actually belong to the set.  Doesn't
yield at all if the set is empty.

)docstring";
}


namespace
{
  mpz_class
  addressify (constant c)
  {
    if (! c.dom ()->safe_arith ())
      std::cerr << "Warning: the constant " << c
		<< " doesn't seem to be suitable for use in address sets.\n";

    auto v = c.value ();

    if (v < 0)
      {
	std::cerr
	  << "Warning: Negative values are not allowed in address sets.\n";
	v = 0;
      }

    return v;
  }
}

value_aset
op_aset_cst_cst::operate (std::unique_ptr <value_cst> a,
			  std::unique_ptr <value_cst> b) const
{
  auto av = addressify (a->get_constant ());
  auto bv = addressify (b->get_constant ());
  if (av > bv)
    std::swap (av, bv);

  coverage cov;
  cov.add (av.uval (), (bv - av).uval ());
  return value_aset (cov, 0);
}

std::string
op_aset_cst_cst::docstring ()
{
  return
R"docstring(

Takes two constants on TOS and constructs an address set that spans
that range.  (The higher address is not considered a part of that
range though.)

)docstring";
}


value_aset
op_add_aset_cst::operate (std::unique_ptr <value_aset> a,
			  std::unique_ptr <value_cst> b) const
{
  auto bv = addressify (b->get_constant ());
  auto ret = a->get_coverage ();
  ret.add (bv.uval (), 1);
  return value_aset {std::move (ret), 0};
}

std::string
op_add_aset_cst::docstring ()
{
  return
R"docstring(

Takes an address set and a constant on TOS, and adds the constant to
range covered by given address set::

	$ dwgrep ./tests/aranges.o -e 'entry @AT_location address 1 add'
	[0x1, 0x2), [0x10000, 0x10010)
	[0x1, 0x2), [0x10010, 0x1001a)

)docstring";
}


value_aset
op_add_aset_aset::operate (std::unique_ptr <value_aset> a,
			   std::unique_ptr <value_aset> b) const
{
  auto ret = a->get_coverage ();
  ret.add_all (b->get_coverage ());
  return value_aset {std::move (ret), 0};
}

std::string
op_add_aset_aset::docstring ()
{
  return
R"docstring(

Takes two address sets and yields an address set that covers both
their ranges::

	$ dwgrep ./tests/aranges.o -e '
		[|D| D entry @AT_location address]
		(|L| L elem (pos == 0) L elem (pos == 1)) add'
	[0x10000, 0x1001a)

)docstring";
}


value_aset
op_sub_aset_cst::operate (std::unique_ptr <value_aset> a,
			  std::unique_ptr <value_cst> b) const
{
  auto bv = addressify (b->get_constant ());
  auto ret = a->get_coverage ();
  ret.remove (bv.uval (), 1);
  return value_aset {std::move (ret), 0};
}

std::string
op_sub_aset_cst::docstring ()
{
  return
R"docstring(

Takes an address set and a constant on TOS and yields an address set
with a hole poked at the address given by the constant::

	$ dwgrep ./tests/aranges.o -e 'entry @AT_location address 0x10010 sub'
	[0x10000, 0x10010)
	[0x10011, 0x1001a)

)docstring";
}


value_aset
op_sub_aset_aset::operate (std::unique_ptr <value_aset> a,
			   std::unique_ptr <value_aset> b) const
{
  auto ret = a->get_coverage ();
  ret.remove_all (b->get_coverage ());
  return value_aset {std::move (ret), 0};
}

std::string
op_sub_aset_aset::docstring ()
{
  return
R"docstring(

Takes two address sets *A* and *B*, and yields an address set that
contains all of the *A*'s addresses except those covered by *B*.

)docstring";
}


value_cst
op_length_aset::operate (std::unique_ptr <value_aset> a) const
{
  uint64_t length = 0;
  for (size_t i = 0; i < a->get_coverage ().size (); ++i)
    length += a->get_coverage ().at (i).length;

  return value_cst {constant {length, &dec_constant_dom}, 0};
}

std::string
op_length_aset::docstring ()
{
  return
R"docstring(

Takes an address set on TOS and yields number of addresses covered by
that set::

	$ dwgrep '0 0x10 aset 0x100 0x110 aset add length'
	32

)docstring";
}


namespace
{
  struct aset_range_producer
    : public value_producer <value_aset>
  {
    std::unique_ptr <value_aset> m_a;
    size_t m_i;

    aset_range_producer (std::unique_ptr <value_aset> a)
      : m_a {std::move (a)}
      , m_i {0}
    {}

    std::unique_ptr <value_aset>
    next () override
    {
      size_t sz = m_a->get_coverage ().size ();
      if (m_i >= sz)
	return nullptr;

      coverage ret;
      auto range = m_a->get_coverage ().at (m_i);
      ret.add (range.start, range.length);

      return std::make_unique <value_aset> (ret, m_i++);
    }
  };
}

std::unique_ptr <value_producer <value_aset>>
op_range_aset::operate (std::unique_ptr <value_aset> a) const
{
  return std::make_unique <aset_range_producer> (std::move (a));
}

std::string
op_range_aset::docstring ()
{
  return
R"docstring(

Takes an address set on TOS and yields all continuous ranges of that
address set, presented as individual address sets::

	$ dwgrep '0 0x10 aset 0x100 0x110 aset add range'
	[0, 0x10)
	[0x100, 0x110)

)docstring";
}


pred_result
pred_containsp_aset_cst::result (value_aset &a, value_cst &b) const
{
  auto av = addressify (b.get_constant ());
  return pred_result (a.get_coverage ().is_covered (av.uval (), 1));
}

std::string
pred_containsp_aset_cst::docstring ()
{
  return
R"docstring(

Inspects an address set and a constant on TOS and holds for those
where the constant is contained within the address set.  Note that the
high end of the address set is not actually considered part of the
address set::

	$ dwgrep 'if (0 10 aset 10 ?contains) then "yes" else "no"'
	no
	$ dwgrep 'if (0 10 aset 9 ?contains) then "yes" else "no"'
	yes

)docstring";
}


pred_result
pred_containsp_aset_aset::result (value_aset &a, value_aset &b) const
{
  // ?contains holds if A contains all of B.
  for (size_t i = 0; i < b.get_coverage ().size (); ++i)
    {
      auto range = b.get_coverage ().at (i);
      if (! a.get_coverage ().is_covered (range.start, range.length))
	return pred_result::no;
    }
  return pred_result::yes;
}

std::string
pred_containsp_aset_aset::docstring ()
{
  return
R"docstring(

Inspects two address sets on TOS, *A* and *B*, and holds for those
where all of *B*'s addresses are covered by *A*.

)docstring";
}


pred_result
pred_overlapsp_aset_aset::result (value_aset &a, value_aset &b) const
{
  for (size_t i = 0; i < b.get_coverage ().size (); ++i)
    {
      auto range = b.get_coverage ().at (i);
      if (a.get_coverage ().is_overlap (range.start, range.length))
	return pred_result::yes;
    }
  return pred_result::no;
}

std::string
pred_overlapsp_aset_aset::docstring ()
{
  return
R"docstring(

Inspects two address sets on TOS, and holds if the two address sets
overlap.

)docstring";
}


value_aset
op_overlap_aset_aset::operate (std::unique_ptr <value_aset> a,
			       std::unique_ptr <value_aset> b) const
{
  coverage ret;
  for (size_t i = 0; i < b->get_coverage ().size (); ++i)
    {
      auto range = b->get_coverage ().at (i);
      auto cov = a->get_coverage ().intersect (range.start, range.length);
      ret.add_all (cov);
    }
  return value_aset {std::move (ret), 0};
}

std::string
op_overlap_aset_aset::docstring ()
{
  return
R"docstring(

Takes two address sets on TOS, and yields a (possibly empty) address
set that covers those addresses that both of the address sets cover.

)docstring";
}



pred_result
pred_emptyp_aset::result (value_aset &a) const
{
  return pred_result (a.get_coverage ().empty ());
}

std::string
pred_emptyp_aset::docstring ()
{
  return
R"docstring(

Inspects an address set on TOS and holds if it is empty (contains no
addresses).  Could be written as ``!(elem)``.

)docstring";
}
