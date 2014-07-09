#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <regex.h>
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


struct op_sel_unit::pimpl
{
  std::shared_ptr <op> m_upstream;
  dwgrep_graph::sptr m_gr;
  valfile::uptr m_vf;
  all_dies_iterator m_it;
  all_dies_iterator m_end;
  size_t m_pos;

  pimpl (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
    : m_upstream {upstream}
    , m_gr {gr}
    , m_it {all_dies_iterator::end ()}
    , m_end {all_dies_iterator::end ()}
    , m_pos {0}
  {}

  void
  init_from_die (Dwarf_Die die)
  {
    Dwarf_Die cudie;
    if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
      throw_libdw ();

    cu_iterator cuit {&*m_gr->dwarf, cudie};
    m_it = all_dies_iterator (cuit);
    m_end = all_dies_iterator (++cuit);
  }

  void
  reset_me ()
  {
    m_vf = nullptr;
    m_it = all_dies_iterator::end ();
    m_pos = 0;
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
		auto vp = vf->pop ();
		if (auto v = value::as <value_die> (&*vp))
		  {
		    init_from_die (v->get_die ());
		    m_vf = std::move (vf);
		  }
		else if (auto v = value::as <value_attr> (&*vp))
		  {
		    init_from_die (v->get_die ());
		    m_vf = std::move (vf);
		  }
		else
		  std::cerr << "Error: `unit' expects a T_NODE or "
			       "T_ATTR on TOS.\n";
	      }
	    else
	      return nullptr;
	  }

	if (m_it != m_end)
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    ret->push (std::make_unique <value_die> (m_gr, **m_it, m_pos++));
	    ++m_it;
	    return ret;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_sel_unit::op_sel_unit (std::shared_ptr <op> upstream, dwgrep_graph::sptr q)
  : m_pimpl {std::make_unique <pimpl> (upstream, q)}
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
  dwgrep_graph::sptr m_gr;
  valfile::uptr m_vf;
  Dwarf_Die m_child;

  size_t m_pos;

  pimpl (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
    : m_upstream {upstream}
    , m_gr {gr}
    , m_child {}
    , m_pos {0}
  {}

  void
  reset_me ()
  {
    m_vf = nullptr;
    m_pos = 0;
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
		auto vp = vf->pop ();
		if (auto v = value::as <value_die> (&*vp))
		  {
		    Dwarf_Die *die = &v->get_die ();
		    if (dwarf_haschildren (die))
		      {
			if (dwarf_child (die, &m_child) != 0)
			  throw_libdw ();

			// We found our guy.
			m_vf = std::move (vf);
		      }
		  }
		else
		  std::cerr << "Error: `child' expects a T_NODE on TOS.\n";
	      }
	    else
	      return nullptr;
	  }

	auto ret = std::make_unique <valfile> (*m_vf);
	ret->push (std::make_unique <value_die> (m_gr, m_child, m_pos++));

	switch (dwarf_siblingof (&m_child, &m_child))
	  {
	  case -1:
	    throw_libdw ();
	  case 1:
	    // No more siblings.
	    reset_me ();
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
    reset_me ();
    m_upstream->reset ();
  }
};

op_f_child::op_f_child (std::shared_ptr <op> upstream,
			dwgrep_graph::sptr gr)
  : m_pimpl {std::make_unique <pimpl> (upstream, gr)}
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
  dwgrep_graph::sptr m_gr;
  Dwarf_Die m_die;
  valfile::uptr m_vf;
  attr_iterator m_it;

  size_t m_pos;

  pimpl (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
    : m_upstream {upstream}
    , m_gr {gr}
    , m_die {}
    , m_it {attr_iterator::end ()}
    , m_pos {0}
  {}

  void
  reset_me ()
  {
    m_vf = nullptr;
    m_pos = 0;
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
		auto vp = vf->pop ();
		if (auto v = value::as <value_die> (&*vp))
		  {
		    m_die = v->get_die ();
		    m_it = attr_iterator (&m_die);
		    m_vf = std::move (vf);
		  }
		else
		  std::cerr << "Error: `attribute' expects a T_NODE on TOS.\n";
	      }
	    else
	      return nullptr;
	  }

	if (m_it != attr_iterator::end ())
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    ret->push (std::make_unique <value_attr>
		       (m_gr, **m_it, m_die, m_pos++));
	    ++m_it;
	    return ret;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_f_attr::op_f_attr (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
  : m_pimpl {std::make_unique <pimpl> (upstream, gr)}
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
    {
      auto vp = vf->pop ();
      if (auto v = value::as <value_die> (&*vp))
	{
	  if (operate (*vf, v->get_die ()))
	    return vf;
	}
      else if (auto v = value::as <value_attr> (&*vp))
	{
	  if (operate (*vf, v->get_attr (), v->get_die ()))
	    return vf;
	}
      else
	std::cerr << "Error: " << name ()
		  << " expects a T_NODE or T_ATTR on TOS.\n";
    }

  return nullptr;
}

bool
op_f_attr_named::operate (valfile &vf, Dwarf_Die &die)
{
  Dwarf_Attribute attr;
  if (dwarf_attr_integrate (&die, m_name, &attr) == nullptr)
    return false;

  vf.push (std::make_unique <value_attr> (m_g, attr, die, 0));
  return true;
}

std::string
op_f_attr_named::name () const
{
  std::stringstream ss;
  ss << "f_attr_named<" << m_name << ">";
  return ss.str ();
}

bool
op_f_offset::operate (valfile &vf, Dwarf_Die &die)
{
  Dwarf_Off off = dwarf_dieoffset (&die);
  vf.push (std::make_unique <value_cst> (constant {off, &hex_constant_dom}, 0));
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
  operate_tag (valfile &vf, Dwarf_Die &die)
  {
    int tag = dwarf_tag (&die);
    assert (tag >= 0);
    constant cst {(unsigned) tag, &dw_tag_dom};
    vf.push (std::make_unique <value_cst> (cst, 0));
    return true;
  }
}

bool
op_f_name::operate (valfile &vf, Dwarf_Die &die)
{
  return operate_tag (vf, die);
}

bool
op_f_name::operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die)

{
  unsigned name = dwarf_whatattr (&attr);
  constant cst {name, &dw_attr_dom};
  vf.push (std::make_unique <value_cst> (cst, 0));
  return true;
}

std::string
op_f_name::name () const
{
  return "f_name";
}

bool
op_f_tag::operate (valfile &vf, Dwarf_Die &die)
{
  return operate_tag (vf, die);
}

std::string
op_f_tag::name () const
{
  return "f_tag";
}

bool
op_f_form::operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die)
{
  unsigned name = dwarf_whatform (&attr);
  constant cst {name, &dw_form_dom};
  vf.push (std::make_unique <value_cst> (cst, 0));
  return true;
}

std::string
op_f_form::name () const
{
  return "f_form";
}


valfile::uptr
op_f_value::next ()
{
  while (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop ();
      if (auto v = value::as <value_attr> (&*vp))
	{
	  vf->push (at_value (v->get_attr (), v->get_die (), m_gr));
	  return vf;
	}
      else if (auto v = value::as <value_cst> (&*vp))
	{
	  auto &cst = v->get_constant ();
	  constant cst2 {cst.value (), &dec_constant_dom};
	  vf->push (std::make_unique <value_cst> (cst2, 0));
	  return vf;
	}
      else
	std::cerr << "Error: `value' expects a T_ATTR or T_CONST on TOS.\n";
    }

  return nullptr;
}

void
op_f_value::reset ()
{
  m_upstream->reset ();
}

std::string
op_f_value::name () const
{
  return "f_value";
}


valfile::uptr
op_f_cast::next ()
{
  while (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop ();
      if (auto v = value::as <value_cst> (&*vp))
	{
	  constant cst2 {v->get_constant ().value (), m_dom};
	  vf->push (std::make_unique <value_cst> (cst2, 0));
	  return vf;
	}
      else
	std::cerr << "Error: cast to " << m_dom->name ()
		  << " expects a constant on TOS.\n";
    }

  return nullptr;
}

void
op_f_cast::reset ()
{
  m_upstream->reset ();
}

std::string
op_f_cast::name () const
{
  std::stringstream ss;
  ss << "f_cast<" << m_dom->name () << ">";
  return ss.str ();
}


bool
op_f_parent::operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die)
{
  vf.push (std::make_unique <value_die> (m_g, die, 0));
  return true;
}

bool
op_f_parent::operate (valfile &vf, Dwarf_Die &die)
{
  Dwarf_Off par_off = m_g->find_parent (die);
  if (par_off == dwgrep_graph::none_off)
    return false;

  Dwarf_Die par_die;
  if (dwarf_offdie (&*m_g->dwarf, par_off, &par_die) == nullptr)
    throw_libdw ();

  vf.push (std::make_unique <value_die> (m_g, par_die, 0));
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
	  ss << *op_vf->pop ();
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

struct op_format::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <stringer_origin> m_origin;
  std::shared_ptr <stringer> m_stringer;
  size_t m_pos;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <stringer_origin> origin,
	 std::shared_ptr <stringer> stringer)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_stringer {stringer}
    , m_pos {0}
  {}

  void
  reset_me ()
  {
    m_stringer->reset ();
    m_pos = 0;
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	auto s = m_stringer->next ();
	if (s.first != nullptr)
	  {
	    s.first->push (std::make_unique <value_str>
			   (std::move (s.second), m_pos++));
	    return std::move (s.first);
	  }

	if (auto vf = m_upstream->next ())
	  {
	    reset_me ();
	    m_origin->set_next (std::move (vf));
	  }
	else
	  return nullptr;
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_format::op_format (std::shared_ptr <op> upstream,
		      std::shared_ptr <stringer_origin> origin,
		      std::shared_ptr <stringer> stringer)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, stringer)}
{}

op_format::~op_format ()
{}

valfile::uptr
op_format::next ()
{
  return m_pimpl->next ();
}

void
op_format::reset ()
{
  m_pimpl->reset ();
}

std::string
op_format::name () const
{
  return "format";
}


valfile::uptr
op_const::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->push (m_value->clone ());
      return vf;
    }
  return nullptr;
}

std::string
op_const::name () const
{
  std::stringstream ss;
  ss << "const<";
  m_value->show (ss);
  ss << ">";
  return ss.str ();
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


void
op_or::reset_me ()
{
  m_branch_it = m_branches.end ();
  for (auto const &branch: m_branches)
    branch.second->reset ();
}

void
op_or::reset ()
{
  reset_me ();
  m_upstream->reset ();
}

valfile::uptr
op_or::next ()
{
  while (true)
    {
      if (m_branch_it == m_branches.end ())
	{
	  if (auto vf = m_upstream->next ())
	    for (m_branch_it = m_branches.begin ();
		 m_branch_it != m_branches.end (); ++m_branch_it)
	      {
		m_branch_it->second->reset ();
		m_branch_it->first->set_next (std::make_unique <valfile> (*vf));
		if (auto vf2 = m_branch_it->second->next ())
		  return vf2;
	      }
	  return nullptr;
	}

      if (auto vf2 = m_branch_it->second->next ())
	return vf2;

      reset_me ();
    }
}

std::string
op_or::name () const
{
  std::stringstream ss;
  ss << "or<";
  bool sep = false;
  for (auto const &branch: m_branches)
    {
      if (sep)
	ss << " || ";
      sep = true;
      ss << branch.second->name ();
    }
  ss << ">";
  return ss.str ();
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
	vv.push_back (vf2->pop ());

      vf->push (std::make_unique <value_seq> (std::move (vv), 0));
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


struct op_f_elem::pimpl
{
  std::shared_ptr <op> m_upstream;
  valfile::uptr m_vf;
  size_t m_i;

  explicit pimpl (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
    , m_i {0}
  {}

  void
  reset_me ()
  {
    m_vf = nullptr;
    m_i = 0;
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_vf == nullptr)
	  if (auto vf = m_upstream->next ())
	    {
	      if (vf->top ().is <value_str> ())
		{
		  auto s = vf->pop ();

		  std::string const &str
		    = static_cast <value_str const &> (*s).get_string ();
		  value_seq::seq_t seq;
		  for (auto c: str)
		    seq.push_back (std::make_unique <value_str>
				   (std::string {c}, 0));

		  vf->push (std::make_unique <value_seq> (std::move (seq), 0));
		}

	      if (vf->top ().is <value_seq> ())
		m_vf = std::move (vf);
	      else
		std::cerr << "Error: `elem' expects a T_SEQ or T_STR on TOS.\n";
	    }
	  else
	    return nullptr;

	value &vp = m_vf->top ();
	auto &vv = *static_cast <value_seq &> (vp).get_seq ();

	if (m_i < vv.size ())
	  {
	    std::unique_ptr <value> v = vv[m_i]->clone ();
	    v->set_pos (m_i);
	    m_i++;
	    valfile::uptr vf = std::make_unique <valfile> (*m_vf);
	    vf->pop ();
	    vf->push (std::move (v));
	    return vf;
	  }

	reset_me ();
      }
  }
};

op_f_elem::op_f_elem (std::shared_ptr <op> upstream)
  : m_pimpl {std::make_unique <pimpl> (upstream)}
{}

op_f_elem::~op_f_elem ()
{}

valfile::uptr
op_f_elem::next ()
{
  return m_pimpl->next ();
}

void
op_f_elem::reset ()
{
  m_pimpl->reset ();
}

std::string
op_f_elem::name () const
{
  return "f_elem";
}


valfile::uptr
op_f_length::next ()
{
  while (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop ();
      if (auto v = value::as <value_seq> (&*vp))
	{
	  constant t {v->get_seq ()->size (), &dec_constant_dom};
	  vf->push (std::make_unique <value_cst> (t, 0));
	  return vf;
	}
      else if (auto v = value::as <value_str> (&*vp))
	{
	  constant t {v->get_string ().length (), &dec_constant_dom};
	  vf->push (std::make_unique <value_cst> (t, 0));
	  return vf;
	}
      else
	std::cerr << "Error: `length' expects a T_SEQ or T_STR on TOS.\n";
    }

  return nullptr;
}

void
op_f_length::reset ()
{
  m_upstream->reset ();
}

std::string
op_f_length::name () const
{
  return "f_length";
}


valfile::uptr
op_f_type::next ()
{
  if (auto vf = m_upstream->next ())
    {
      constant t = vf->pop ()->get_type_const ();
      vf->push (std::make_unique <value_cst> (t, 0));
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


namespace
{
  struct deref_less
  {
    template <class T>
    bool
    operator() (T const &a, T const &b)
    {
      return *a < *b;
    }
  };
}

struct op_tr_closure::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;

  std::set <std::shared_ptr <valfile>, deref_less> m_seen;
  std::vector <std::shared_ptr <valfile> > m_vfs;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> origin,
	 std::shared_ptr <op> op)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
  {}

  void
  reset_me ()
  {
    m_vfs.clear ();
    m_seen.clear ();
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	if (m_vfs.empty ())
	  {
	    reset_me ();
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

op_tr_closure::op_tr_closure (std::shared_ptr <op> upstream,
			      std::shared_ptr <op_origin> origin,
			      std::shared_ptr <op> op)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, op)}
{}

op_tr_closure::~op_tr_closure ()
{}

valfile::uptr
op_tr_closure::next ()
{
  return m_pimpl->next ();
}

void
op_tr_closure::reset ()
{
  m_pimpl->reset ();
}

std::string
op_tr_closure::name () const
{
  return m_pimpl->name ();
}


struct op_subx::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  valfile::uptr m_vf;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> origin,
	 std::shared_ptr <op> op)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
  {}

  void
  reset_me ()
  {
    m_vf = nullptr;
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_vf == nullptr)
	  if ((m_vf = m_upstream->next ()) != nullptr)
	    {
	      m_op->reset ();
	      m_origin->set_next (std::make_unique <valfile> (*m_vf));
	    }
	  else
	    return nullptr;

	if (auto vf = m_op->next ())
	  {
	    auto ret = std::make_unique <valfile> (*m_vf);
	    ret->push (vf->pop ());
	    return ret;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_subx::op_subx (std::shared_ptr <op> upstream,
		  std::shared_ptr <op_origin> origin,
		  std::shared_ptr <op> op)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, op)}
{}

op_subx::~op_subx ()
{}

valfile::uptr
op_subx::next ()
{
  return m_pimpl->next ();
}

void
op_subx::reset ()
{
  m_pimpl->reset ();
}

std::string
op_subx::name () const
{
  std::stringstream ss;
  ss << "subx<" << m_pimpl->m_op->name () << ">";
  return ss.str ();
}


valfile::uptr
op_f_debug::next ()
{
  while (auto vf = m_upstream->next ())
    {
      {
	std::vector <std::unique_ptr <value>> stk;
	while (vf->size () > 0)
	  stk.push_back (vf->pop ());

	std::cerr << "<";
	std::for_each (stk.rbegin (), stk.rend (),
		       [&vf] (std::unique_ptr <value> &v) {
			 v->show (std::cerr << ' ');
			 vf->push (std::move (v));
		       });
	std::cerr << " > (";
      }

      std::shared_ptr <frame> frame = vf->nth_frame (0);
      while (frame != nullptr)
	{
	  std::cerr << frame;
	  std::cerr << "{";
	  for (auto const &v: frame->m_values)
	    v->show (std::cerr << ' ');
	  std::cerr << " }  ";

	  frame = frame->m_parent;
	}
      std::cerr << ")\n";

      return vf;
    }
  return nullptr;
}

std::string
op_f_debug::name () const
{
  return "f_debug";
}

void
op_f_debug::reset ()
{
  m_upstream->reset ();
}


valfile::uptr
simple_op::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop ();
      vf->push (operate (*vp));
      return vf;
    }

  return nullptr;
}

void
simple_op::reset ()
{
  m_upstream->reset ();
}


std::unique_ptr <value>
op_f_pos::operate (value const &v) const
{
  return std::make_unique <value_cst> (constant {v.get_pos (), &pos_dom}, 0);
}

std::string
op_f_pos::name () const
{
  return "f_pos";
}

struct op_transform::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  std::vector <std::unique_ptr <value> > m_saved;
  unsigned m_depth;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> origin,
	 std::shared_ptr <op> op,
	 unsigned depth)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
    , m_depth {depth}
  {
    assert (m_depth > 0);
  }

  void
  reset_me ()
  {
    m_saved.clear ();
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_saved.empty ())
	  if (auto vf = m_upstream->next ())
	    {
	      for (unsigned i = 0; i < m_depth - 1; ++i)
		m_saved.push_back (vf->pop ());

	      m_op->reset ();
	      m_origin->set_next (std::move (vf));
	    }
	  else
	    return nullptr;

	if (auto ret = m_op->next ())
	  {
	    for (unsigned i = m_saved.size (); i > 0; --i)
	      ret->push (m_saved[i - 1]->clone ());
	    return ret;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_transform::op_transform (std::shared_ptr <op> upstream,
			    std::shared_ptr <op_origin> origin,
			    std::shared_ptr <op> op,
			    unsigned depth)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, op, depth)}
{}

op_transform::~op_transform ()
{}

valfile::uptr
op_transform::next ()
{
  return m_pimpl->next ();
}

void
op_transform::reset ()
{
  return m_pimpl->reset ();
}

std::string
op_transform::name () const
{
  std::stringstream ss;
  ss << "trasform<" << m_pimpl->m_depth
     << ", " << m_pimpl->m_op->name () << ">";
  return ss.str ();
}


struct op_scope::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op_origin> m_origin;
  std::shared_ptr <op> m_op;
  size_t m_num_vars;
  bool m_primed;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> origin,
	 std::shared_ptr <op> op,
	 size_t num_vars)
    : m_upstream {upstream}
    , m_origin {origin}
    , m_op {op}
    , m_num_vars {num_vars}
    , m_primed {false}
  {}

  void
  reset_me ()
  {
    m_primed = false;
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (! m_primed)
	  if (auto vf = m_upstream->next ())
	    {
	      // Push new stack frame.
	      vf->set_frame (std::make_shared <frame> (vf->nth_frame (0),
						       m_num_vars));
	      m_op->reset ();
	      m_origin->set_next (std::move (vf));
	      m_primed = true;
	    }
	  else
	    return nullptr;

	if (auto vf = m_op->next ())
	  {
	    // Pop top stack frame.
	    vf->set_frame (vf->nth_frame (1));
	    return vf;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_scope::op_scope (std::shared_ptr <op> upstream,
		    std::shared_ptr <op_origin> origin,
		    std::shared_ptr <op> op,
		    size_t num_vars)
  : m_pimpl {std::make_unique <pimpl> (upstream, origin, op, num_vars)}
{}

op_scope::~op_scope ()
{}

valfile::uptr
op_scope::next ()
{
  return m_pimpl->next ();
}

void
op_scope::reset ()
{
  return m_pimpl->reset ();
}

std::string
op_scope::name () const
{
  std::stringstream ss;
  ss << "scope<vars=" << m_pimpl->m_num_vars
     << ", " << m_pimpl->m_op->name () << ">";
  return ss.str ();
}


void
op_bind::reset ()
{
  m_upstream->reset ();
}

valfile::uptr
op_bind::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto frame = vf->nth_frame (m_depth);
      frame->bind_value (m_index, vf->pop ());
      return vf;
    }
  return nullptr;
}

std::string
op_bind::name () const
{
  std::stringstream ss;
  ss << "bind<" << m_index << "@" << m_depth << ">";
  return ss.str ();
}


struct op_read::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op> m_apply;
  size_t m_depth;
  var_id m_index;

  pimpl (std::shared_ptr <op> upstream, size_t depth, var_id index)
    : m_upstream {upstream}
    , m_depth {depth}
    , m_index {index}
  {}

  void
  reset_me ()
  {
    m_apply = nullptr;
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	if (m_apply == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		auto frame = vf->nth_frame (m_depth);
		value &val = frame->read_value (m_index);
		bool is_closure = val.is <value_closure> ();
		vf->push (val.clone ());

		// If a referenced value is not a closure, then the
		// result is just that one value.
		if (! is_closure)
		  return vf;

		// If it's a closure, then this is a function
		// reference.  We need to execute it and fetch all the
		// values.

		auto origin = std::make_shared <op_origin> (std::move (vf));
		m_apply = std::make_shared <op_apply> (origin);
	      }
	    else
	      return nullptr;
	  }

	assert (m_apply != nullptr);
	if (auto vf = m_apply->next ())
	  return vf;

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_read::op_read (std::shared_ptr <op> upstream, size_t depth, var_id index)
  : m_pimpl {std::make_unique <pimpl> (upstream, depth, index)}
{}

op_read::~op_read ()
{}

void
op_read::reset ()
{
  m_pimpl->reset ();
}

valfile::uptr
op_read::next ()
{
  return m_pimpl->next ();
}

std::string
op_read::name () const
{
  std::stringstream ss;
  ss << "read<" << m_pimpl->m_index << "@" << m_pimpl->m_depth << ">";
  return ss.str ();
}


void
op_lex_closure::reset ()
{
  m_upstream->reset ();
}

valfile::uptr
op_lex_closure::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->push (std::make_unique <value_closure> (m_t, m_q, m_scope,
						  vf->nth_frame (0), 0));
      return vf;
    }
  return nullptr;
}

std::string
op_lex_closure::name () const
{
  return "lex_closure";
}


struct op_apply::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op> m_op;
  std::shared_ptr <frame> m_old_frame;

  pimpl (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  void
  reset_me ()
  {
    m_op = nullptr;
    m_old_frame = nullptr;
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	while (m_op == nullptr)
	  if (auto vf = m_upstream->next ())
	    {
	      if (! vf->top ().is <value_closure> ())
		{
		  std::cerr << "Error: `apply' expects a T_CLOSURE on TOS.\n";
		  continue;
		}

	      auto val = vf->pop ();
	      auto &cl = static_cast <value_closure &> (*val);

	      m_old_frame = vf->nth_frame (0);
	      vf->set_frame (cl.get_frame ());
	      auto origin = std::make_shared <op_origin> (std::move (vf));
	      m_op = cl.get_tree ().build_exec (origin, cl.get_graph (),
						cl.get_scope ());
	    }
	  else
	    return nullptr;

	if (auto vf = m_op->next ())
	  {
	    vf->set_frame (m_old_frame);
	    return vf;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_apply::op_apply (std::shared_ptr <op> upstream)
  : m_pimpl {std::make_unique <pimpl> (upstream)}
{}

op_apply::~op_apply ()
{}

void
op_apply::reset ()
{
  m_pimpl->reset ();
}

valfile::uptr
op_apply::next ()
{
  return m_pimpl->next ();
}

std::string
op_apply::name () const
{
  return "apply";
}


struct op_ifelse::pimpl
{
  std::shared_ptr <op> m_upstream;

  std::shared_ptr <op_origin> m_cond_origin;
  std::shared_ptr <op> m_cond_op;

  std::shared_ptr <op_origin> m_then_origin;
  std::shared_ptr <op> m_then_op;

  std::shared_ptr <op_origin> m_else_origin;
  std::shared_ptr <op> m_else_op;

  std::shared_ptr <op_origin> m_sel_origin;
  std::shared_ptr <op> m_sel_op;

  pimpl (std::shared_ptr <op> upstream,
	 std::shared_ptr <op_origin> cond_origin,
	 std::shared_ptr <op> cond_op,
	 std::shared_ptr <op_origin> then_origin,
	 std::shared_ptr <op> then_op,
	 std::shared_ptr <op_origin> else_origin,
	 std::shared_ptr <op> else_op)
    : m_upstream {upstream}
    , m_cond_origin {cond_origin}
    , m_cond_op {cond_op}
    , m_then_origin {then_origin}
    , m_then_op {then_op}
    , m_else_origin {else_origin}
    , m_else_op {else_op}
  {
    reset_me ();
  }

  void
  reset_me ()
  {
    m_sel_origin = nullptr;
    m_sel_op = nullptr;
  }

  valfile::uptr
  next ()
  {
    while (true)
      {
	if (m_sel_op == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		m_cond_op->reset ();
		m_cond_origin->set_next (std::make_unique <valfile> (*vf));

		if (m_cond_op->next () != nullptr)
		  {
		    m_sel_origin = m_then_origin;
		    m_sel_op = m_then_op;
		  }
		else
		  {
		    m_sel_origin = m_else_origin;
		    m_sel_op = m_else_op;
		  }

		m_sel_op->reset ();
		m_sel_origin->set_next (std::move (vf));
	      }
	    else
	      return nullptr;
	  }

	if (auto vf = m_sel_op->next ())
	  return vf;

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_ifelse::op_ifelse (std::shared_ptr <op> upstream,
		      std::shared_ptr <op_origin> cond_origin,
		      std::shared_ptr <op> cond_op,
		      std::shared_ptr <op_origin> then_origin,
		      std::shared_ptr <op> then_op,
		      std::shared_ptr <op_origin> else_origin,
		      std::shared_ptr <op> else_op)
  : m_pimpl {std::make_unique <pimpl> (upstream, cond_origin, cond_op,
				       then_origin, then_op,
				       else_origin, else_op)}
{}

op_ifelse::~op_ifelse ()
{}

void
op_ifelse::reset ()
{
  m_pimpl->reset ();
}

valfile::uptr
op_ifelse::next ()
{
  return m_pimpl->next ();
}

std::string
op_ifelse::name () const
{
  return "ifelse";
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
  ss << "not<" << m_a->name () << ">";
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
  ss << "and<" << m_a->name () << "><" << m_b->name () << ">";
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
  ss << "or<" << m_a->name () << "><" << m_b->name () << ">";
  return ss.str ();
}

pred_result
pred_at::result (valfile &vf)
{
  if (auto v = vf.top_as <value_die> ())
    {
      Dwarf_Die *die = &v->get_die ();
      return pred_result (dwarf_hasattr_integrate (die, m_atname) != 0);
    }
  else if (auto v = vf.top_as <value_attr> ())
    return pred_result (dwarf_whatattr (&v->get_attr ()) == m_atname);
  else
    {
      std::cerr << "Error: `?@" << constant {m_atname, &dw_attr_short_dom}
		<< "' expects a T_NODE or T_ATTR on TOS.\n";
      return pred_result::fail;
    }
}

std::string
pred_at::name () const
{
  std::stringstream ss;
  ss << "pred_at<" << constant {m_atname, &dw_attr_dom} << ">";
  return ss.str ();
}

pred_result
pred_tag::result (valfile &vf)
{
  if (auto v = vf.top_as <value_die> ())
    return pred_result (dwarf_tag (&v->get_die ()) == m_tag);
  else
    {
      std::cerr << "Error: `?"
		<< constant {(unsigned) m_tag, &dw_tag_short_dom}
		<< "' expects a T_NODE on TOS.\n";
      return pred_result::fail;
    }
}

std::string
pred_tag::name () const
{
  std::stringstream ss;
  ss << "pred_tag<" << constant {(unsigned) m_tag, &dw_tag_dom} << ">";
  return ss.str ();
}

pred_root::pred_root (dwgrep_graph::sptr g)
  : m_g {g}
{}

pred_result
pred_root::result (valfile &vf)
{
  if (auto v = vf.top_as <value_die> ())
    return pred_result (m_g->is_root (v->get_die ()));

  else if (vf.top ().is <value_attr> ())
    // By definition, attributes are never root.
    return pred_result::no;

  else
    {
      std::cerr << "Error: `?root' expects a T_ATTR or T_NODE on TOS.\n";
      return pred_result::fail;
    }
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
  if (auto v = vf.top_as <value_str> ())
    return pred_result (v->get_string () == "");

  else if (auto v = vf.top_as <value_seq> ())
    return pred_result (v->get_seq ()->empty ());

  else
    {
      std::cerr << "Error: `?empty' expects a T_STR or T_SEQ on TOS.\n";
      return pred_result::fail;
    }
}

std::string
pred_empty::name () const
{
  return "pred_empty";
}

pred_result
pred_match::result (valfile &vf)
{
  auto va = vf.below_as <value_str> ();
  if (va == nullptr)
    {
      std::cerr << "Error: match: value below TOS is not a string\n";
      return pred_result::fail;
    }

  auto vb = vf.top_as <value_str> ();
  if (vb == nullptr)
    {
      std::cerr << "Error: match: value on TOS is not a string\n";
      return pred_result::fail;
    }

  regex_t re;
  if (regcomp(&re, vb->get_string ().c_str(), REG_EXTENDED | REG_NOSUB) != 0)
    {
      std::cerr << "Error: could not compile regular expression: '"
		<< vb->get_string () << "'\n";
      return pred_result::fail;
    }

  const int reti = regexec(&re, va->get_string ().c_str (),
		   /* nmatch: size of pmatch array */ 0,
		   /* pmatch: array of matches */ NULL,
		   /* no extra flags */ 0);

  pred_result retval = pred_result::fail;
  if (reti == 0)
    retval = pred_result::yes;
  else if (reti == REG_NOMATCH)
    retval = pred_result::no;
  else
    {
      char msgbuf[100];
      regerror(reti, &re, msgbuf, sizeof(msgbuf));
      std::cerr << "Error: match failed: " << msgbuf << "\n";
    }

  regfree(&re);
  return retval;
}

std::string pred_match::name () const
{
    return "pred_match";
}
