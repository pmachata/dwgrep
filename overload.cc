#include <memory>
#include "make_unique.hh"

#include "overload.hh"

overload_instance::overload_instance
	(std::vector <std::tuple <value_type, builtin &>> const &stencil,
	 dwgrep_graph::sptr q, std::shared_ptr <scope> scope)
{
  std::transform (stencil.begin (), stencil.end (),
		  std::back_inserter (m_overloads),
		  [&q, &scope] (std::tuple <value_type, builtin &> const &v)
		  {
		    auto origin = std::make_shared <op_origin> (nullptr);
		    auto op = std::get <1> (v).build_exec (origin, q, scope);
		    assert (op != nullptr);
		    return std::make_tuple (std::get <0> (v), origin, op);
		  });
}

overload_instance::overload_vec::value_type *
overload_instance::find_overload (value_type vt)
{
  auto it = std::find_if (m_overloads.begin (), m_overloads.end (),
			  [vt] (overload_vec::value_type const &ovl)
			  { return std::get <0> (ovl) == vt; });

  if (it == m_overloads.end ())
    return nullptr;
  else
    return &*it;
}

void
overload_instance::show_error (std::string const &name)
{
  std::cerr << "Error: `" << name << "'";
  if (m_overloads.empty ())
    {
      std::cerr << " has no registered overloads.\n";
      return;
    }

  std::cerr << " expects ";
  size_t i = 0;
  for (auto const &ovl: m_overloads)
    {
      if (i == 0)
	;
      else if (i == m_overloads.size () - 1)
	std::cerr << " or ";
      else
	std::cerr << ", ";

      ++i;
      std::cerr << std::get <0> (ovl).name ();
    }
  std::cerr << " on TOS.\n";
}


void
overload_tab::add_overload (value_type vt, builtin &b)
{
  // If this assert fails, the likely suspect is static initialization
  // order fiasco.
  assert (vt != value_type {0});

  // Check someone didn't order overload for this type yet.
  assert (std::find_if (m_overloads.begin (), m_overloads.end (),
			[vt] (std::tuple <value_type, builtin &> const &p)
			{ return std::get <0> (p) == vt; })
	  == m_overloads.end ());

  m_overloads.push_back (std::tuple <value_type, builtin &> {vt, b});
}

overload_instance
overload_tab::instantiate (dwgrep_graph::sptr q, std::shared_ptr <scope> scope)
{
  return overload_instance {m_overloads, q, scope};
}


struct overload_op::pimpl
{
  std::shared_ptr <op> m_upstream;
  overload_instance m_ovl_inst;
  std::shared_ptr <op> m_op;

  void
  reset_me ()
  {
    m_op = nullptr;
  }

  pimpl (std::shared_ptr <op> upstream, overload_instance ovl_inst)
    : m_upstream {upstream}
    , m_ovl_inst {ovl_inst}
    , m_op {nullptr}
  {}

  valfile::uptr
  next (op &self)
  {
    while (true)
      {
	while (m_op == nullptr)
	  {
	    if (auto vf = m_upstream->next ())
	      {
		value &val = vf->top ();
		auto ovl = m_ovl_inst.find_overload (val.get_type ());
		if (ovl == nullptr)
		  m_ovl_inst.show_error (self.name ());
		else
		  {
		    m_op = std::get <2> (*ovl);
		    m_op->reset ();
		    std::get <1> (*ovl)->set_next (std::move (vf));
		  }
	      }
	    else
	      return nullptr;
	  }

	if (auto vf = m_op->next ())
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

overload_op::overload_op (std::shared_ptr <op> upstream,
			  overload_instance ovl_inst)
  : m_pimpl {std::make_unique <pimpl> (upstream, ovl_inst)}
{}

overload_op::~overload_op ()
{}

valfile::uptr
overload_op::next ()
{
  return m_pimpl->next (*this);
}

void
overload_op::reset ()
{
  return m_pimpl->reset ();
}
