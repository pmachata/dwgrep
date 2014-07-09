#include <iostream>
#include "builtin.hh"
#include "op.hh"

namespace
{
  void
  check_arith (constant const &cst_a, constant const &cst_b)
  {
    // If a named constant partakes, warn.
    if (! cst_a.dom ()->safe_arith () || ! cst_b.dom ()->safe_arith ())
      std::cerr << "Warning: doing arithmetic with " << cst_a << " and "
		<< cst_b << " is probably not meaningful.\n";
  }

  struct arith_op
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      while (auto vf = m_upstream->next ())
	{
	  auto bp = vf->pop ();
	  auto ap = vf->pop ();
	  if (auto cp = operate (*ap, *bp))
	    {
	      vf->push (std::move (cp));
	      return vf;
	    }
	}

      return nullptr;
    }

    virtual std::unique_ptr <value>
    operate (value const &a, value const &b) const = 0;
  };

  template <class F>
  std::unique_ptr <value>
  simple_arith_op (value const &a, value const &b,
		   char const *name, F f)
  {
    if (auto va = value::as <value_cst> (&a))
      if (auto vb = value::as <value_cst> (&b))
	{
	  constant const &cst_a = va->get_constant ();
	  constant const &cst_b = vb->get_constant ();

	  check_arith (cst_a, cst_b);

	  constant_dom const *d = cst_a.dom ()->plain ()
	    ? cst_b.dom () : cst_a.dom ();

	  return f (cst_a, cst_b, d);
	}

    std::cerr << "Error: `" << name << "' expects T_CONST at and below TOS.\n";
    return nullptr;
  }
}

static struct
  : public builtin
{
  struct add
    : public arith_op
  {
    using arith_op::arith_op;

    std::unique_ptr <value>
    operate (value const &vpa, value const &vpb) const override
    {
      auto show_error = [] ()
	{
	  std::cerr << "Error: `add' expects T_CONST, T_STR or T_SEQ "
	  "at and below TOS.\n";
	};

      if (auto va = value::as <value_cst> (&vpa))
	{
	  if (auto vb = value::as <value_cst> (&vpb))
	    {
	      constant const &cst_a = va->get_constant ();
	      constant const &cst_b = vb->get_constant ();

	      check_arith (cst_a, cst_b);

	      constant_dom const *d = cst_a.dom ()->plain ()
		? cst_b.dom () : cst_a.dom ();

	      constant result {cst_a.value () + cst_b.value (), d};
	      return std::make_unique <value_cst> (result, 0);
	    }
	  else
	    show_error ();
	}
      else if (auto va = value::as <value_str> (&vpa))
	{
	  if (auto vb = value::as <value_str> (&vpb))
	    {
	      std::string result = va->get_string () + vb->get_string ();

	      return std::make_unique <value_str> (std::move (result), 0);
	    }
	  else
	    show_error ();
	}
      else if (auto va = value::as <value_seq> (&vpa))
	{
	  if (auto vb = value::as <value_seq> (&vpb))
	    {
	      value_seq::seq_t res;
	      for (auto const &v: *va->get_seq ())
		res.emplace_back (v->clone ());
	      for (auto const &v: *vb->get_seq ())
		res.emplace_back (v->clone ());

	      return std::make_unique <value_seq> (std::move (res), 0);
	    }
	  else
	    show_error ();
	}
      else
	show_error ();

      return nullptr;
    }

    std::string name () const override
    {
      return "add";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <add> (upstream);
  }

  char const *
  name () const
  {
    return "add";
  }
} builtin_add_obj;

static struct
  : public builtin
{
  struct sub
    : public arith_op
  {
    using arith_op::arith_op;

    std::unique_ptr <value>
    operate (value const &a, value const &b) const override
    {
      return simple_arith_op
	(a, b, "sub",
	 [] (constant const &cst_a, constant const &cst_b,
	     constant_dom const *d)
	 {
	   constant r {cst_a.value () - cst_b.value (), d};
	   return std::make_unique <value_cst> (r, 0);
	 });
    }

    std::string name () const override
    {
      return "sub";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <sub> (upstream);
  }

  char const *
  name () const
  {
    return "sub";
  }
} builtin_sub_obj;

static struct
  : public builtin
{
  struct mul
    : public arith_op
  {
    using arith_op::arith_op;

    std::unique_ptr <value>
    operate (value const &a, value const &b) const override
    {
      return simple_arith_op
	(a, b, "mul",
	 [] (constant const &cst_a, constant const &cst_b,
	     constant_dom const *d)
	 {
	   constant r {cst_a.value () * cst_b.value (), d};
	   return std::make_unique <value_cst> (r, 0);
	 });
    }

    std::string name () const override
    {
      return "mul";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <mul> (upstream);
  }

  char const *
  name () const
  {
    return "mul";
  }
} builtin_mul_obj;

static struct
  : public builtin
{
  struct div
    : public arith_op
  {
    using arith_op::arith_op;

    std::unique_ptr <value>
    operate (value const &a, value const &b) const override
    {
      return simple_arith_op
	(a, b, "div",
	 [] (constant const &cst_a, constant const &cst_b,
	     constant_dom const *d) -> std::unique_ptr <value>
	 {
	   if (cst_b.value () == 0)
	     {
	       std::cerr << "Error: `div': division by zero.\n";
	       return nullptr;
	     }

	   constant r {cst_a.value () / cst_b.value (), d};
	   return std::make_unique <value_cst> (r, 0);
	 });
    }

    std::string name () const override
    {
      return "div";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <div> (upstream);
  }

  char const *
  name () const
  {
    return "div";
  }
} builtin_div_obj;

static struct
  : public builtin
{
  struct mod
    : public arith_op
  {
    using arith_op::arith_op;

    std::unique_ptr <value>
    operate (value const &a, value const &b) const override
    {
      return simple_arith_op
	(a, b, "mod",
	 [] (constant const &cst_a, constant const &cst_b,
	     constant_dom const *d) -> std::unique_ptr <value>
	 {
	   if (cst_b.value () == 0)
	     {
	       std::cerr << "Error: `mod': division by zero.\n";
	       return nullptr;
	     }

	   constant r {cst_a.value () % cst_b.value (), d};
	   return std::make_unique <value_cst> (r, 0);
	 });
    }

    std::string name () const override
    {
      return "mod";
    }
  };

  std::shared_ptr <op>
  build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
	      std::shared_ptr <scope> scope) const override
  {
    return std::make_shared <mod> (upstream);
  }

  char const *
  name () const
  {
    return "mod";
  }
} builtin_mod_obj;

static struct register_arith
{
  register_arith ()
  {
    add_builtin (builtin_add_obj);
    add_builtin (builtin_sub_obj);
    add_builtin (builtin_mul_obj);
    add_builtin (builtin_div_obj);
    add_builtin (builtin_mod_obj);
  }
} register_arith;
