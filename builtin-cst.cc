#include <sstream>
#include <memory>
#include "make_unique.hh"

#include "builtin-cst.hh"
#include "op.hh"

namespace
{
  class op_cast
    : public op
  {
    std::shared_ptr <op> m_upstream;
    constant_dom const *m_dom;

  public:
    op_cast (std::shared_ptr <op> upstream, constant_dom const *dom)
      : m_upstream {upstream}
      , m_dom {dom}
    {}

    valfile::uptr
    next () override
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

    std::string name () const override
    {
      std::stringstream ss;
      ss << "f_cast<" << m_dom->name () << ">";
      return ss.str ();
    }

    void reset () override
    {
      return m_upstream->reset ();
    }
  };
}


std::shared_ptr <op>
builtin_constant::build_exec (std::shared_ptr <op> upstream,
			      dwgrep_graph::sptr q,
			      std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_const> (upstream, m_value->clone ());
}

char const *
builtin_constant::name () const
{
  return "constant";
}


std::shared_ptr <op>
builtin_hex::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_cast> (upstream, &hex_constant_dom);
}

char const *
builtin_hex::name () const
{
  return "hex";
}


std::shared_ptr <op>
builtin_dec::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_cast> (upstream, &dec_constant_dom);
}

char const *
builtin_dec::name () const
{
  return "dec";
}


std::shared_ptr <op>
builtin_oct::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_cast> (upstream, &oct_constant_dom);
}

char const *
builtin_oct::name () const
{
  return "oct";
}


std::shared_ptr <op>
builtin_bin::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_cast> (upstream, &bin_constant_dom);
}

char const *
builtin_bin::name () const
{
  return "bin";
}


namespace
{
  class op_f_type
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  constant t = vf->pop ()->get_type_const ();
	  vf->push (std::make_unique <value_cst> (t, 0));
	  return vf;
	}

      return nullptr;
    }

    std::string
    name () const override
    {
      return "type";
    }
  };
}

std::shared_ptr <op>
builtin_type::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_f_type> (upstream);
}

char const *
builtin_type::name () const
{
  return "type";
}


namespace
{
  class op_f_pos
    : public inner_op
  {
  public:
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  auto vp = vf->pop ();
	  static numeric_constant_dom_t pos_dom_obj ("pos");
	  vf->push (std::make_unique <value_cst>
		    (constant {vp->get_pos (), &pos_dom_obj}, 0));
	  return vf;
	}

      return nullptr;
    }

    std::string
    name () const override
    {
      return "pos";
    }
  };
}

std::shared_ptr <op>
builtin_pos::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_f_pos> (upstream);
}

char const *
builtin_pos::name () const
{
  return "pos";
}


namespace
{
  class op_f_nth
    : public inner_op
  {
  public:
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      while (auto vf = m_upstream->next ())
	if (auto n = vf->pop_as <value_cst> ())
	  {
	    constant pos {vf->top ().get_pos (), &dec_constant_dom};
	    constant const &nc = n->get_constant ();
	    check_constants_comparable (nc, pos);
	    if (pos == nc)
	      return vf;
	  }

      return nullptr;
    }

    std::string
    name () const override
    {
      return "nth";
    }
  };
}

std::shared_ptr <op>
builtin_nth::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_f_nth> (upstream);
}

char const *
builtin_nth::name () const
{
  return "nth";
}
