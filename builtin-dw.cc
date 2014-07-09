#include <memory>
#include "make_unique.hh"

#include "builtin.hh"
#include "dwit.hh"
#include "op.hh"
#include "dwpp.hh"

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

static struct register_dw
{
  register_dw ()
  {
    add_builtin (builtin_winfo);
    add_builtin (builtin_unit);
    add_builtin (builtin_child);
    add_builtin (builtin_attribute);
  }
} register_dw;
