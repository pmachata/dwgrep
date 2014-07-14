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
