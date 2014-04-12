#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include "make_unique.hh"

#include "op.hh"
#include "dwit.hh"
#include "dwpp.hh"
#include "dwcst.hh"
#include "vfcst.hh"
#include "atval.hh"

valfile::uptr
op_origin::next ()
{
  return std::move (m_vf);
}

std::string
op_origin::name () const
{
  return "origin";
}

void
op_origin::set_next (valfile::uptr vf)
{
  assert (m_vf == nullptr);

  // set_next should have been preceded with a reset() call that
  // should have percolated all the way here.
  assert (m_reset);
  m_reset = false;

  m_vf = std::move (vf);
}

void
op_origin::reset ()
{
  m_vf = nullptr;
  m_reset = true;
}


struct op_sel_universe::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <Dwarf> m_dw;
  all_dies_iterator m_it;
  valfile::uptr m_vf;
  slot_idx m_dst;

  pimpl (std::shared_ptr <op> upstream, dwgrep_graph::sptr q, slot_idx dst)
    : m_upstream (upstream)
    , m_dw (q->dwarf)
    , m_it (all_dies_iterator::end ())
    , m_dst (dst)
  {}

  valfile::uptr
  next ()
  {
    while (true)
      {
	if (m_vf == nullptr)
	  {
	    m_vf = m_upstream->next ();
	    if (m_vf == nullptr)
	      return nullptr;
	    m_it = all_dies_iterator (&*m_dw);
	  }

	if (m_it != all_dies_iterator::end ())
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    ret->set_slot (m_dst, std::make_unique <value_die> (**m_it));
	    ++m_it;
	    return ret;
	  }

	m_vf = nullptr;
      }
  }

  void
  reset ()
  {
    m_vf = nullptr;
    m_upstream->reset ();
  }
};

op_sel_universe::op_sel_universe (std::shared_ptr <op> upstream,
				  dwgrep_graph::sptr q, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, q, dst))
{}

op_sel_universe::~op_sel_universe ()
{}

valfile::uptr
op_sel_universe::next ()
{
  return m_pimpl->next ();
}

void
op_sel_universe::reset ()
{
  m_pimpl->reset ();
}

std::string
op_sel_universe::name () const
{
  return "sel_universe";
}


struct op_sel_unit::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <Dwarf> m_dw;
  valfile::uptr m_vf;
  slot_idx m_src;
  slot_idx m_dst;
  all_dies_iterator m_it;
  all_dies_iterator m_end;

  pimpl (std::shared_ptr <op> upstream,
	 dwgrep_graph::sptr g, slot_idx src, slot_idx dst)
    : m_upstream (upstream)
    , m_dw (g->dwarf)
    , m_src (src)
    , m_dst (dst)
    , m_it (all_dies_iterator::end ())
    , m_end (all_dies_iterator::end ())
  {}

  void
  init_from_die (Dwarf_Die die)
  {
    Dwarf_Die cudie;
    if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
      throw_libdw ();

    cu_iterator cuit {&*m_dw, cudie};
    m_it = all_dies_iterator (cuit);
    m_end = all_dies_iterator (++cuit);
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_vf == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		if (auto v = vf->get_slot_as <value_die> (m_src))
		  {
		    init_from_die (v->get_die ());
		    m_vf = std::move (vf);
		  }
		else if (auto v = vf->get_slot_as <value_attr> (m_src))
		  {
		    init_from_die (v->get_die ());
		    m_vf = std::move (vf);
		  }
	      }
	    else
	      return nullptr;
	  }

	if (m_it != m_end)
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    ret->set_slot (m_dst, std::make_unique <value_die> (**m_it));
	    ++m_it;
	    return ret;
	  }

	m_vf = nullptr;
	m_it = all_dies_iterator::end ();
      }
  }

  void
  reset ()
  {
    m_vf = nullptr;
    m_it = all_dies_iterator::end ();
    m_upstream->reset ();
  }
};

op_sel_unit::op_sel_unit (std::shared_ptr <op> upstream,
			  dwgrep_graph::sptr q, slot_idx src, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, q, src, dst))
{}

op_sel_unit::~op_sel_unit ()
{}

valfile::uptr
op_sel_unit::next ()
{
  return m_pimpl->next ();
}

void
op_sel_unit::reset ()
{
  m_pimpl->reset ();
}

std::string
op_sel_unit::name () const
{
  return "sel_unit";
}


struct op_f_child::pimpl
{
  std::shared_ptr <op> m_upstream;
  valfile::uptr m_vf;
  Dwarf_Die m_child;

  slot_idx m_src;
  slot_idx m_dst;

  pimpl (std::shared_ptr <op> upstream, slot_idx src, slot_idx dst)
    : m_upstream (upstream)
    , m_child {}
    , m_src (src)
    , m_dst (dst)
  {}

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_vf == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		if (auto v = vf->get_slot_as <value_die> (m_src))
		  {
		    Dwarf_Die die = v->get_die ();
		    if (dwarf_haschildren (&die))
		      {
			if (dwarf_child (&die, &m_child) != 0)
			  throw_libdw ();

			// We found our guy.
			m_vf = std::move (vf);
		      }
		  }
	      }
	    else
	      return nullptr;
	  }

	auto ret = std::make_unique <valfile> (*m_vf);
	ret->set_slot (m_dst, std::make_unique <value_die> (m_child));

	switch (dwarf_siblingof (&m_child, &m_child))
	  {
	  case -1:
	    throw_libdw ();
	  case 1:
	    // No more siblings.
	    m_vf = nullptr;
	    break;
	  case 0:
	    break;
	  }

	return ret;
      }
  }

  void
  reset ()
  {
    m_vf = nullptr;
    m_upstream->reset ();
  }
};

op_f_child::op_f_child (std::shared_ptr <op> upstream,
			slot_idx src, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, src, dst))
{}

op_f_child::~op_f_child ()
{}

valfile::uptr
op_f_child::next ()
{
  return m_pimpl->next ();
}

std::string
op_f_child::name () const
{
  return "f_child";
}

void
op_f_child::reset ()
{
  m_pimpl->reset ();
}


struct op_f_attr::pimpl
{
  std::shared_ptr <op> m_upstream;
  Dwarf_Die m_die;
  valfile::uptr m_vf;
  attr_iterator m_it;

  slot_idx m_src;
  slot_idx m_dst;

  pimpl (std::shared_ptr <op> upstream, slot_idx src, slot_idx dst)
    : m_upstream (upstream)
    , m_die {}
    , m_it (attr_iterator::end ())
    , m_src (src)
    , m_dst (dst)
  {}

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_vf == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		if (auto v = vf->get_slot_as <value_die> (m_src))
		  {
		    m_die = v->get_die ();
		    m_it = attr_iterator (&m_die);
		    m_vf = std::move (vf);
		  }
	      }
	    else
	      return nullptr;
	  }

	if (m_it != attr_iterator::end ())
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    auto vp = std::make_unique <value_attr> (**m_it, m_die);
	    ret->set_slot (m_dst, std::move (vp));
	    ++m_it;
	    return ret;
	  }

	m_vf = nullptr;
      }
  }

  void
  reset ()
  {
    m_vf = nullptr;
    m_upstream->reset ();
  }
};

op_f_attr::op_f_attr (std::shared_ptr <op> upstream, slot_idx src, slot_idx dst)
  : m_pimpl (std::make_unique <pimpl> (upstream, src, dst))
{}

op_f_attr::~op_f_attr ()
{}

valfile::uptr
op_f_attr::next ()
{
  return m_pimpl->next ();
}

std::string
op_f_attr::name () const
{
  return "f_attr";
}

void
op_f_attr::reset ()
{
  m_pimpl->reset ();
}


valfile::uptr
op_nop::next ()
{
  return m_upstream->next ();
}

std::string
op_nop::name () const
{
  return "nop";
}


valfile::uptr
op_assert::next ()
{
  while (auto vf = m_upstream->next ())
    if (m_pred->result (*vf) == pred_result::yes)
      return vf;
  return nullptr;
}

std::string
op_assert::name () const
{
  std::stringstream ss;
  ss << "assert<" << m_pred->name () << ">";
  return ss.str ();
}

valfile::uptr
dwop_f::next ()
{
  while (auto vf = m_upstream->next ())
    if (auto v = vf->get_slot_as <value_die> (m_src))
      {
	Dwarf_Die die = v->get_die ();
	if (operate (*vf, m_dst, die))
	  return vf;
      }
    else if (auto v = vf->get_slot_as <value_attr> (m_src))
      {
	if (operate (*vf, m_dst, v->get_attr (), v->get_die ()))
	  return vf;
      }

  return nullptr;
}

bool
op_f_atval::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate (&die, m_name, &attr) == nullptr)
    return false;

  vf.set_slot (dst, at_value (attr, die));
  return true;
}

std::string
op_f_atval::name () const
{
  std::stringstream ss;
  ss << "f_atval<" << m_name << ">";
  return ss.str ();
}

bool
op_f_offset::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  Dwarf_Off off = dwarf_dieoffset (&die);
  auto v = std::make_unique <value_cst> (constant {off, &hex_constant_dom});
  vf.set_slot (dst, std::move (v));
  return true;
}

std::string
op_f_offset::name () const
{
  return "f_offset";
}

namespace
{
  bool
  operate_tag (valfile &vf, slot_idx dst, Dwarf_Die die)
  {
    int tag = dwarf_tag (&die);
    assert (tag >= 0);
    constant cst {(unsigned) tag, &dw_tag_dom};
    vf.set_slot (dst, std::make_unique <value_cst> (cst));
    return true;
  }
}

bool
op_f_name::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  return operate_tag (vf, dst, die);
}

bool
op_f_name::operate (valfile &vf, slot_idx dst,
		    Dwarf_Attribute attr, Dwarf_Die die) const

{
  unsigned name = dwarf_whatattr (&attr);
  constant cst {name, &dw_attr_dom};
  vf.set_slot (dst, std::make_unique <value_cst> (cst));
  return true;
}

std::string
op_f_name::name () const
{
  return "f_name";
}

bool
op_f_tag::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  return operate_tag (vf, dst, die);
}

std::string
op_f_tag::name () const
{
  return "f_tag";
}

bool
op_f_form::operate (valfile &vf, slot_idx dst,
		    Dwarf_Attribute attr, Dwarf_Die die) const
{
  unsigned name = dwarf_whatform (&attr);
  constant cst {name, &dw_form_dom};
  vf.set_slot (dst, std::make_unique <value_cst> (cst));
  return true;
}

std::string
op_f_form::name () const
{
  return "f_form";
}


bool
op_f_value::operate (valfile &vf, slot_idx dst,
		     Dwarf_Attribute attr, Dwarf_Die die) const
{
  vf.set_slot (dst, at_value (attr, die));
  return true;
}

std::string
op_f_value::name () const
{
  return "f_value";
}


bool
op_f_parent::operate (valfile &vf, slot_idx dst,
		      Dwarf_Attribute attr, Dwarf_Die die) const
{
  vf.set_slot (dst, std::make_unique <value_die> (die));
  return true;
}

bool
op_f_parent::operate (valfile &vf, slot_idx dst, Dwarf_Die die) const
{
  Dwarf_Off par_off = m_g->find_parent (die);
  if (par_off == dwgrep_graph::none_off)
    return false;

  if (dwarf_offdie (&*m_g->dwarf, par_off, &die) == nullptr)
    throw_libdw ();
  vf.set_slot (dst, std::make_unique <value_die> (die));
  return true;
}

std::string
op_f_parent::name () const
{
  return "f_parent";
}


void
stringer_origin::set_next (valfile::uptr vf)
{
  assert (m_vf == nullptr);

  // set_next should have been preceded with a reset() call that
  // should have percolated all the way here.
  assert (m_reset);
  m_reset = false;

  m_vf = std::move (vf);
}

std::pair <valfile::uptr, std::string>
stringer_origin::next ()
{
  return std::make_pair (std::move (m_vf), "");
}

void
stringer_origin::reset ()
{
  m_vf = nullptr;
  m_reset = true;
}

std::pair <valfile::uptr, std::string>
stringer_lit::next ()
{
  auto up = m_upstream->next ();
  if (up.first == nullptr)
    return std::make_pair (nullptr, "");
  up.second += m_str;
  return up;
}

void
stringer_lit::reset ()
{
  m_upstream->reset ();
}

std::pair <valfile::uptr, std::string>
stringer_op::next ()
{
  while (true)
    {
      if (! m_have)
	{
	  auto up = m_upstream->next ();
	  if (up.first == nullptr)
	    return std::make_pair (nullptr, "");

	  m_op->reset ();
	  m_origin->set_next (std::move (up.first));
	  m_str = up.second;

	  m_have = true;
	}

      if (auto op_vf = m_op->next ())
	{
	  std::stringstream ss;
	  op_vf->get_slot (m_src).show (ss);
	  op_vf->set_slot (m_src, nullptr);
	  return std::make_pair (std::move (op_vf), m_str + ss.str ());
	}

      m_have = false;
    }
}

void
stringer_op::reset ()
{
  m_have = false;
  m_op->reset ();
  m_upstream->reset ();
}

valfile::uptr
op_format::next ()
{
  while (true)
    {
      auto s = m_stringer->next ();
      if (s.first != nullptr)
	{
	  s.first->set_slot
	    (m_dst, std::make_unique <value_str> (std::move (s.second)));
	  return std::move (s.first);
	}

      if (auto vf = m_upstream->next ())
	{
	  m_stringer->reset ();
	  m_origin->set_next (std::move (vf));
	}
      else
	return nullptr;
    }
}

void
op_format::reset ()
{
  m_stringer->reset ();
  m_upstream->reset ();
}

std::string
op_format::name () const
{
  return "format";
}

valfile::uptr
op_drop::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_idx, nullptr);
      return vf;
    }
  return nullptr;
}

void
op_drop::reset ()
{
  m_upstream->reset ();
}

std::string
op_drop::name () const
{
  return "drop";
}

valfile::uptr
op_dup::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_dst, vf->get_slot (m_src).clone ());
      return vf;
    }
  return nullptr;
}

void
op_dup::reset ()
{
  m_upstream->reset ();
}

std::string
op_dup::name () const
{
  return "drop";
}


valfile::uptr
op_swap::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto tmp = vf->take_slot (m_dst_a);
      vf->set_slot (m_dst_a, vf->take_slot (m_dst_b));
      vf->set_slot (m_dst_b, std::move (tmp));
      return vf;
    }
  return nullptr;
}

void
op_swap::reset ()
{
  m_upstream->reset ();
}

std::string
op_swap::name () const
{
  return "swap";
}


valfile::uptr
op_const::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_dst, std::make_unique <value_cst> (m_cst));
      return vf;
    }
  return nullptr;
}

std::string
op_const::name () const
{
  std::stringstream ss;
  ss << "const<" << m_cst << ">";
  return ss.str ();
}


valfile::uptr
op_strlit::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_dst, std::make_unique <value_str> (std::string (m_str)));
      return vf;
    }
  return nullptr;
}

std::string
op_strlit::name () const
{
  std::stringstream ss;
  ss << "strlit<" << m_str << ">";
  return ss.str ();
}


valfile::uptr
op_empty_list::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->set_slot (m_dst, std::make_unique <value_seq> (value_seq::seq_t {}));
      return vf;
    }
  return nullptr;
}

std::string
op_empty_list::name () const
{
  return "empty_list";
}


valfile::uptr
op_tine::next ()
{
  if (*m_done)
    return nullptr;

  if (std::all_of (m_file->begin (), m_file->end (),
		   [] (valfile::uptr const &ptr) { return ptr == nullptr; }))
    {
      if (auto vf = m_upstream->next ())
	for (auto &ptr: *m_file)
	  ptr = std::make_unique <valfile> (*vf);
      else
	{
	  *m_done = true;
	  return nullptr;
	}
    }

  return std::move ((*m_file)[m_branch_id]);
}

void
op_tine::reset ()
{
  for (auto &vf: *m_file)
    vf = nullptr;
  m_upstream->reset ();
}

std::string
op_tine::name () const
{
  return "tine";
}


valfile::uptr
op_merge::next ()
{
  if (*m_done)
    return nullptr;

  while (! *m_done)
    {
      if (auto ret = (*m_it)->next ())
	return ret;
      if (++m_it == m_ops.end ())
	m_it = m_ops.begin ();
    }

  return nullptr;
}

void
op_merge::reset ()
{
  *m_done = false;
  m_it = m_ops.begin ();
  for (auto op: m_ops)
    op->reset ();
}

std::string
op_merge::name () const
{
  return "merge";
}


valfile::uptr
op_capture::next ()
{
  if (auto vf = m_upstream->next ())
    {
      m_op->reset ();
      m_origin->set_next (std::make_unique <valfile> (*vf));

      value_seq::seq_t vv;
      while (auto vf2 = m_op->next ())
	vv.push_back (vf2->get_slot (m_src).clone ());
      vf->set_slot (m_dst, std::make_unique <value_seq> (std::move (vv)));
      return vf;
    }
  return nullptr;
}

void
op_capture::reset ()
{
  m_op->reset ();
  m_upstream->reset ();
}

std::string
op_capture::name () const
{
  std::stringstream ss;
  ss << "capture<" << m_op->name () << ">";
  return ss.str ();
}


valfile::uptr
op_f_each::next ()
{
  while (true)
    {
      while (m_vf == nullptr)
	{
	  if (auto vf = m_upstream->next ())
	    {
	      if (vf->get_slot_as <value_seq> (m_src) != nullptr)
		{
		  m_i = 0;
		  m_vf = std::move (vf);
		}
	    }
	  else
	    return nullptr;
	}

      auto &slot = static_cast <value_seq const &> (m_vf->get_slot (m_src));
      auto &vv = slot.get_seq ();

      if (m_i < vv.size ())
	{
	  std::unique_ptr <value> v = vv[m_i++]->clone ();
	  valfile::uptr vf = std::make_unique <valfile> (*m_vf);
	  vf->set_slot (m_dst, std::move (v));
	  return vf;
	}

      m_vf = nullptr;
    }
}

void
op_f_each::reset ()
{
  m_vf = nullptr;
  m_upstream->reset ();
}

std::string
op_f_each::name () const
{
  return "f_each";
}


valfile::uptr
op_f_type::next ()
{
  if (auto vf = m_upstream->next ())
    {
      constant t = vf->get_slot (m_src).get_type_const ();
      vf->set_slot (m_dst, std::make_unique <value_cst> (t));
      return vf;
    }

  return nullptr;
}

void
op_f_type::reset ()
{
  m_upstream->reset ();
}

std::string
op_f_type::name () const
{
  return "f_type";
}


struct op_close::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;

  struct vf_lt {
    bool operator() (std::shared_ptr <valfile> const &a,
		     std::shared_ptr <valfile> const &b) {
      return *a < *b;
    }
  };

  std::set <std::shared_ptr <valfile>, vf_lt> m_seen;
  std::vector <std::shared_ptr <valfile> > m_vfs;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> origin,
	 std::shared_ptr <op> op)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
  {}

  void
  do_reset ()
  {
    m_vfs.clear ();
    m_seen.clear ();
  }

  void
  reset ()
  {
    do_reset ();
    m_upstream->reset ();
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	if (m_vfs.empty ())
	  {
	    do_reset ();
	    if (std::shared_ptr <valfile> vf = m_upstream->next ())
	      {
		m_vfs.push_back (vf);
		m_seen.insert (vf);
	      }
	    else
	      return nullptr;
	  }

	auto vf = m_vfs.back ();
	m_vfs.pop_back ();

	m_op->reset ();
	m_origin->set_next (std::make_unique <valfile> (*vf));

	while (std::shared_ptr <valfile> vf2 = m_op->next ())
	  if (m_seen.find (vf2) == m_seen.end ())
	    {
	      m_vfs.push_back (vf2);
	      m_seen.insert (vf2);
	    }

	return std::make_unique <valfile> (*vf);
      }
    return nullptr;
  }

  std::string
  name () const
  {
    std::stringstream ss;
    ss << "close<" << m_upstream->name () << ">";
    return ss.str ();
  }
};

op_close::op_close (std::shared_ptr <op> upstream,
		    std::shared_ptr <op_origin> origin,
		    std::shared_ptr <op> op)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, op)}
{}

op_close::~op_close ()
{}

valfile::uptr
op_close::next ()
{
  return m_pimpl->next ();
}

void
op_close::reset ()
{
  m_pimpl->reset ();
}

std::string
op_close::name () const
{
  return m_pimpl->name ();
}


namespace
{
  void
  check_arith (constant cst_a, constant cst_b)
  {
    // If a named constant partakes, warn.
    if (! cst_a.dom ()->safe_arith () || ! cst_b.dom ()->safe_arith ())
      std::cerr << "Warning: doing arithmetic with " << cst_a << " and "
		<< cst_b << " is probably not meaningful.\n";
  }
}

valfile::uptr
op_f_add::next ()
{
  while (auto vf = m_upstream->next ())
    if (auto va = vf->get_slot_as <value_cst> (m_src_a))
      {
	if (auto vb = vf->get_slot_as <value_cst> (m_src_b))
	  {
	    constant cst_a = va->get_constant ();
	    constant cst_b = vb->get_constant ();

	    check_arith (cst_a, cst_b);

	    constant result;
	    if (cst_a.dom ()->sign () == cst_b.dom ()->sign ()
		|| (cst_b.dom ()->sign () && ((int64_t) cst_b.value ()) >= 0))
	      // Either both are signed, or both are unsigned, in
	      // which case we can simply add.  Or B is signed and A
	      // unsigned, but B is non-negative, and we can pretend
	      // it's unsigned.
	      result = constant {cst_a.value () + cst_b.value (),
				 cst_a.dom ()->plain ()
					? cst_b.dom () : cst_a.dom ()};

	    else if (cst_a.dom ()->sign () && ((int64_t) cst_a.value ()) >= 0)
	      // A's signed but non-negative, treat it as unsigned.
	      result = constant {cst_a.value () + cst_b.value (),
				 cst_b.dom ()};

	    else if ((int64_t) (cst_a.value () + cst_b.value ()) >= 0)
	      // A or B is signed and negative, but the other one is
	      // greater, so we can keep unsigned dom.
	      result = constant {cst_a.value () + cst_b.value (),
				 cst_a.dom ()->sign ()
					? cst_b.dom () : cst_a.dom ()};

	    else
	      // Otherwise we need to take the signed dom.
	      result = constant {cst_a.value () + cst_b.value (),
				 cst_a.dom ()->sign ()
					? cst_a.dom () : cst_b.dom ()};
	    if (m_src_a == m_dst)
	      vf->set_slot (m_src_b, nullptr);
	    else if (m_src_b == m_dst)
	      vf->set_slot (m_src_a, nullptr);

	    vf->set_slot (m_dst, std::make_unique <value_cst> (result));
	    return vf;
	  }
      }
    else if (auto va = vf->get_slot_as <value_str> (m_src_a))
      {
	if (auto vb = vf->get_slot_as <value_str> (m_src_b))
	  {
	    std::string result = va->get_string () + vb->get_string ();

	    if (m_src_a == m_dst)
	      vf->set_slot (m_src_b, nullptr);
	    else if (m_src_b == m_dst)
	      vf->set_slot (m_src_a, nullptr);

	    vf->set_slot (m_dst,
			  std::make_unique <value_str> (std::move (result)));
	    return vf;
	  }
      }
    else if (auto va = vf->get_slot_as <value_seq> (m_src_a))
      {
	if (auto vb = vf->get_slot_as <value_seq> (m_src_b))
	  {
	    value_seq::seq_t seq_a = va->move_seq ();
	    for (auto &v: vb->move_seq ())
	      seq_a.emplace_back (std::move (v));

	    if (m_src_a == m_dst)
	      vf->set_slot (m_src_b, nullptr);
	    else if (m_src_b == m_dst)
	      vf->set_slot (m_src_a, nullptr);

	    vf->set_slot (m_dst,
			  std::make_unique <value_seq> (std::move (seq_a)));
	    return vf;
	  }
      }

  return nullptr;
}

void
op_f_add::reset ()
{
  m_upstream->reset ();
}

std::string
op_f_add::name () const
{
  return "f_add";
}


pred_result
pred_not::result (valfile &vf)
{
  return ! m_a->result (vf);
}

std::string
pred_not::name () const
{
  std::stringstream ss;
  ss << "not <" << m_a->name () << ">";
  return ss.str ();
}


pred_result
pred_and::result (valfile &vf)
{
  return m_a->result (vf) && m_b->result (vf);
}

std::string
pred_and::name () const
{
  std::stringstream ss;
  ss << "and <" << m_a->name () << "><" << m_b->name () << ">";
  return ss.str ();
}


pred_result
pred_or::result (valfile &vf)
{
  return m_a->result (vf) || m_b->result (vf);
}

std::string
pred_or::name () const
{
  std::stringstream ss;
  ss << "or <" << m_a->name () << "><" << m_b->name () << ">";
  return ss.str ();
}

pred_result
pred_at::result (valfile &vf)
{
  if (auto v = vf.get_slot_as <value_die> (m_idx))
    {
      Dwarf_Die die = v->get_die ();
      return pred_result (dwarf_hasattr_integrate (&die, m_atname) != 0);
    }
  else if (auto v = vf.get_slot_as <value_attr> (m_idx))
    {
      Dwarf_Attribute attr = v->get_attr ();
      return pred_result (dwarf_whatattr (&attr) == m_atname);
    }
  else
    return pred_result::fail;
}

std::string
pred_at::name () const
{
  std::stringstream ss;
  ss << "pred_at<" << m_atname << ">";
  return ss.str ();
}

pred_result
pred_tag::result (valfile &vf)
{
  if (auto v = vf.get_slot_as <value_die> (m_idx))
    {
      Dwarf_Die die = v->get_die ();
      return pred_result (dwarf_tag (&die) == m_tag);
    }
  else
    return pred_result::fail;
}

std::string
pred_tag::name () const
{
  std::stringstream ss;
  ss << "pred_tag<" << m_tag << ">";
  return ss.str ();
}

namespace
{
  pred_result
  comparison_result (valfile &vf, slot_idx idx_a, slot_idx idx_b,
		     cmp_result want)
  {
    cmp_result r = vf.get_slot (idx_a).cmp (vf.get_slot (idx_b));
    if (r == cmp_result::fail)
      return pred_result::fail;
    else
      return pred_result (r == want);
  }
}

pred_result
pred_eq::result (valfile &vf)
{
  return comparison_result (vf, m_idx_a, m_idx_b, cmp_result::equal);
}

std::string
pred_eq::name () const
{
  return "pred_eq";
}

pred_result
pred_lt::result (valfile &vf)
{
  return comparison_result (vf, m_idx_a, m_idx_b, cmp_result::less);
}

std::string
pred_lt::name () const
{
  return "pred_lt";
}

pred_result
pred_gt::result (valfile &vf)
{
  return comparison_result (vf, m_idx_a, m_idx_b, cmp_result::greater);
}

std::string
pred_gt::name () const
{
  return "pred_gt";
}

pred_root::pred_root (dwgrep_graph::sptr g, slot_idx idx_a)
  : m_g (g)
  , m_idx_a (idx_a)
{
}

pred_result
pred_root::result (valfile &vf)
{
  if (auto v = vf.get_slot_as <value_die> (m_idx_a))
    return pred_result (m_g->is_root (v->get_die ()));

  else if (vf.get_slot_as <value_attr> (m_idx_a) != nullptr)
    // By definition, attributes are never root.
    return pred_result::no;

  else
    return pred_result::fail;
}

std::string
pred_root::name () const
{
  return "pred_root";
}

pred_result
pred_subx_any::result (valfile &vf)
{
  m_op->reset ();
  m_origin->set_next (std::make_unique <valfile> (vf));
  if (m_op->next () != nullptr)
    return pred_result::yes;
  else
    return pred_result::no;
}

std::string
pred_subx_any::name () const
{
  std::stringstream ss;
  ss << "pred_subx_any<" << m_op->name () << ">";
  return ss.str ();
}

void
pred_subx_any::reset ()
{
  m_op->reset ();
}

pred_result
pred_empty::result (valfile &vf)
{
  if (auto v = vf.get_slot_as <value_str> (m_idx_a))
    return pred_result (v->get_string () == "");

  else if (auto v = vf.get_slot_as <value_seq> (m_idx_a))
    return pred_result (v->get_seq ().empty ());

  else
    return pred_result::fail;
}

std::string
pred_empty::name () const
{
  return "pred_empty";
}
