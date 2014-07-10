#include <memory>
#include "make_unique.hh"
#include <sstream>

#include "builtin.hh"
#include "dwit.hh"
#include "op.hh"
#include "dwpp.hh"
#include "dwcst.hh"
#include "known-dwarf.h"

static struct
  : public builtin
{
  struct winfo
    : public op
  {
    std::shared_ptr <op> m_upstream;
    dwgrep_graph::sptr m_gr;
    all_dies_iterator m_it;
    valfile::uptr m_vf;
    size_t m_pos;

    winfo (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
      : m_upstream {upstream}
      , m_gr {gr}
      , m_it {all_dies_iterator::end ()}
      , m_pos {0}
    {}

    void
    reset_me ()
    {
      m_vf = nullptr;
      m_pos = 0;
    }

    valfile::uptr
    next () override
    {
      while (true)
	{
	  if (m_vf == nullptr)
	    {
	      m_vf = m_upstream->next ();
	      if (m_vf == nullptr)
		return nullptr;
	      m_it = all_dies_iterator (&*m_gr->dwarf);
	    }

	  if (m_it != all_dies_iterator::end ())
	    {
	      auto ret = std::make_unique <valfile> (*m_vf);
	      auto v = std::make_unique <value_die> (m_gr, **m_it, m_pos++);
	      ret->push (std::move (v));
	      ++m_it;
	      return ret;
	    }

	  reset_me ();
	}
    }

    void
    reset () override
    {
      reset_me ();
      m_upstream->reset ();
    }

    std::string
    name () const override
    {
      return "winfo";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <winfo> (upstream, q);
  }

  char const *
  name () const override
  {
    return "winfo";
  }
} builtin_winfo;

static struct
  : public builtin
{
  struct unit
    : public op
  {
    std::shared_ptr <op> m_upstream;
    dwgrep_graph::sptr m_gr;
    valfile::uptr m_vf;
    all_dies_iterator m_it;
    all_dies_iterator m_end;
    size_t m_pos;

    unit (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
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
    next () override
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
    reset () override
    {
      reset_me ();
      m_upstream->reset ();
    }

    std::string
    name () const override
    {
      return "unit";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <unit> (upstream, q);
  }

  char const *
  name () const override
  {
    return "unit";
  }
} builtin_unit;

static struct
  : public builtin
{
  struct child
    : public op
  {
    std::shared_ptr <op> m_upstream;
    dwgrep_graph::sptr m_gr;
    valfile::uptr m_vf;
    Dwarf_Die m_child;

    size_t m_pos;

    child (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
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
    next () override
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
    reset () override
    {
      reset_me ();
      m_upstream->reset ();
    }

    std::string
    name () const override
    {
      return "child";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <child> (upstream, q);
  }

  char const *
  name () const override
  {
    return "child";
  }
} builtin_child;

static struct
  : public builtin
{
  struct attribute
    : public op
  {
    std::shared_ptr <op> m_upstream;
    dwgrep_graph::sptr m_gr;
    Dwarf_Die m_die;
    valfile::uptr m_vf;
    attr_iterator m_it;

    size_t m_pos;

    attribute (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
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
		    std::cerr
		      << "Error: `attribute' expects a T_NODE on TOS.\n";
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

    std::string
    name () const override
    {
      return "attribute";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <attribute> (upstream, q);
  }

  char const *
  name () const override
  {
    return "attribute";
  }
} builtin_attribute;

namespace
{
  class dwop_f
    : public op
  {
    std::shared_ptr <op> m_upstream;

  protected:
    dwgrep_graph::sptr m_g;

  public:
    dwop_f (std::shared_ptr <op> upstream, dwgrep_graph::sptr gr)
      : m_upstream {upstream}
      , m_g {gr}
    {}

    valfile::uptr
    next () override final
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

    void reset () override final
    { m_upstream->reset (); }

    virtual std::string name () const override = 0;

    virtual bool operate (valfile &vf, Dwarf_Die &die)
    { return false; }

    virtual bool operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die)
    { return false; }
  };
}

static struct
  : public builtin
{
  struct offset
    : public dwop_f
  {
    using dwop_f::dwop_f;

    bool
    operate (valfile &vf, Dwarf_Die &die) override
    {
      Dwarf_Off off = dwarf_dieoffset (&die);
      auto cst = constant {off, &hex_constant_dom};
      vf.push (std::make_unique <value_cst> (cst, 0));
      return true;
    }

    std::string
    name () const override
    {
      return "offset";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <offset> (upstream, q);
  }

  char const *
  name () const override
  {
    return "offset";
  }
} builtin_offset;

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

static struct
  : public builtin
{
  struct op
    : public dwop_f
  {
    using dwop_f::dwop_f;

    bool
    operate (valfile &vf, Dwarf_Die &die) override
    {
      return operate_tag (vf, die);
    }

    bool
    operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die) override
    {
      unsigned name = dwarf_whatattr (&attr);
      constant cst {name, &dw_attr_dom};
      vf.push (std::make_unique <value_cst> (cst, 0));
      return true;
    }

    std::string
    name () const override
    {
      return "name";
    }
  };

  std::shared_ptr < ::op>
  build_exec (std::shared_ptr < ::op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <op> (upstream, q);
  }

  char const *
  name () const override
  {
    return "name";
  }
} builtin_name;

static struct
  : public builtin
{
  struct tag
    : public dwop_f
  {
    using dwop_f::dwop_f;

    bool
    operate (valfile &vf, Dwarf_Die &die) override
    {
      return operate_tag (vf, die);
    }

    std::string
    name () const override
    {
      return "tag";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <tag> (upstream, q);
  }

  char const *
  name () const override
  {
    return "tag";
  }
} builtin_tag;

static struct
  : public builtin
{
  struct form
    : public dwop_f
  {
    using dwop_f::dwop_f;

    bool
    operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die) override
    {
      unsigned name = dwarf_whatform (&attr);
      constant cst {name, &dw_form_dom};
      vf.push (std::make_unique <value_cst> (cst, 0));
      return true;
    }

    std::string
    name () const override
    {
      return "form";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <form> (upstream, q);
  }

  char const *
  name () const override
  {
    return "form";
  }
} builtin_form;

static struct
  : public builtin
{
  struct parent
    : public dwop_f
  {
    using dwop_f::dwop_f;

    bool
    operate (valfile &vf, Dwarf_Die &die) override
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

    bool
    operate (valfile &vf, Dwarf_Attribute &attr, Dwarf_Die &die) override
    {
      vf.push (std::make_unique <value_die> (m_g, die, 0));
      return true;
    }

    std::string
    name () const override
    {
      return "parent";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <parent> (upstream, q);
  }

  char const *
  name () const override
  {
    return "parent";
  }
} builtin_parent;

static struct
  : public pred_builtin
{
  using pred_builtin::pred_builtin;

  struct p
    : public pred
  {
    dwgrep_graph::sptr m_g;

    explicit p (dwgrep_graph::sptr g)
      : m_g {g}
    {}

    pred_result
    result (valfile &vf) override
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

    void
    reset () override
    {}

    std::string
    name () const override
    {
      return "root";
    }
  };

  std::unique_ptr <pred>
  build_pred (dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return maybe_invert (std::make_unique <p> (q));
  }

  char const *
  name () const override
  {
    if (m_positive)
      return "?root";
    else
      return "!root";
  }
} builtin_rootp {true}, builtin_nrootp {false};

namespace
{
  struct builtin_attr_named
    : public builtin
  {
    int m_atname;
    explicit builtin_attr_named (int atname)
      : m_atname {atname}
    {}

    struct o
      : public dwop_f
    {
      int m_atname;

    public:
      o (std::shared_ptr <op> upstream, dwgrep_graph::sptr g, int atname)
	: dwop_f {upstream, g}
	, m_atname {atname}
      {}

      bool
      operate (valfile &vf, Dwarf_Die &die) override
      {
	Dwarf_Attribute attr;
	if (dwarf_attr_integrate (&die, m_atname, &attr) == nullptr)
	  return false;

	vf.push (std::make_unique <value_attr> (m_g, attr, die, 0));
	return true;
      }

      std::string
      name () const override
      {
	std::stringstream ss;
	ss << "@" << constant {m_atname, &dw_attr_dom};
	return ss.str ();
      }
    };

    std::shared_ptr <op>
    build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
		std::shared_ptr <scope> scope) const override
    {
      auto t = std::make_shared <o> (upstream, q, m_atname);
      return std::make_shared <op_f_value> (t, q);
    }

    char const *
    name () const override
    {
      return "@attr";
    }
  };
}

#define ONE_KNOWN_DW_AT(NAME, CODE)		\
  static builtin_attr_named builtin_attr_##NAME {CODE};
ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT

namespace
{
  struct builtin_pred_attr
    : public pred_builtin
  {
    int m_atname;
    builtin_pred_attr (int atname, bool positive)
      : pred_builtin {positive}
      , m_atname {atname}
    {}

    struct p
      : public pred
    {
      unsigned m_atname;

      explicit p (unsigned atname)
	: m_atname (atname)
      {}

      pred_result
      result (valfile &vf) override
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
	    std::cerr << "Error: `?AT_"
		      << constant {m_atname, &dw_attr_short_dom}
		      << "' expects a T_NODE or T_ATTR on TOS.\n";
	    return pred_result::fail;
	  }
      }

      void reset () override {}

      std::string
      name () const override
      {
	std::stringstream ss;
	ss << "?AT_" << constant {m_atname, &dw_attr_short_dom};
	return ss.str ();
      }
    };

    std::unique_ptr <pred>
    build_pred (dwgrep_graph::sptr q,
		std::shared_ptr <scope> scope) const override
    {
      return maybe_invert (std::make_unique <p> (m_atname));
    }

    char const *
    name () const override
    {
      if (m_positive)
	return "?attr";
      else
	return "!attr";
    }
  };
}

#define ONE_KNOWN_DW_AT(NAME, CODE)					\
  static builtin_pred_attr builtin_pred_attr_##NAME {CODE, true},	\
    builtin_pred_nattr_##NAME {CODE, false};
ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT

static struct register_dw
{
  register_dw ()
  {
    add_builtin (builtin_winfo);
    add_builtin (builtin_unit);

    add_builtin (builtin_child);
    add_builtin (builtin_attribute);
    add_builtin (builtin_offset);
    add_builtin (builtin_name);
    add_builtin (builtin_tag);
    add_builtin (builtin_form);
    add_builtin (builtin_parent);

#define ONE_KNOWN_DW_AT(NAME, CODE)				\
    add_builtin (builtin_attr_##NAME, "@" #NAME);		\
    add_builtin (builtin_pred_attr_##NAME, "?@" #NAME);	\
    add_builtin (builtin_pred_nattr_##NAME, "!@" #NAME);
    ALL_KNOWN_DW_AT
#undef ONE_KNOWN_DW_AT

    add_builtin (builtin_rootp);
    add_builtin (builtin_nrootp);
  }
} register_dw;
