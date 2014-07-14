#include <iostream>
#include <memory>
#include "make_unique.hh"

#include "op.hh"
#include "tree.hh"
#include "value.hh"

value_type
value_type::alloc (char const *name)
{
  static uint8_t last = 0;
  value_type vt {++last, name};
  if (last == 0)
    {
      std::cerr << "Ran out of value type identifiers." << std::endl;
      std::terminate ();
    }
  return vt;
}

namespace
{
  std::vector <std::pair <uint8_t, char const *>> &
  get_vtype_names ()
  {
    static std::vector <std::pair <uint8_t, char const *>> names;
    return names;
  }

  char const *
  find_vtype_name (uint8_t code)
  {
    auto &vtn = get_vtype_names ();
    auto it = std::find_if (vtn.begin (), vtn.end (),
			    [code] (std::pair <uint8_t, char const *> const &v)
			    { return v.first == code; });
    if (it == vtn.end ())
      return nullptr;
    else
      return it->second;
  }
}

void
value_type::register_name (uint8_t code, char const *name)
{
  auto &vtn = get_vtype_names ();
  assert (find_vtype_name (code) == nullptr);
  vtn.push_back (std::make_pair (code, name));
}

char const *
value_type::name () const
{
  auto ret = find_vtype_name (m_code);
  assert (ret != nullptr);
  return ret;
}

static struct
  : public constant_dom
{
  void
  show (mpz_class const &v, std::ostream &o) const override
  {
    if (v.fits_uint_p ())
      {
	unsigned int ui = v.get_ui ();
	static_assert (sizeof (value_type) == 1,
		       "assuming value_type is 8 bits large");
	if (ui <= 0xff)
	  {
	    char const *name = find_vtype_name (ui);
	    assert (name != nullptr);
	    o << name;
	    return;
	  }
      }

    assert (! "Invalid slot type constant value.");
    abort ();
  }

  std::string name () const override
  {
    return "T_*";
  }
} slot_type_dom_obj;

constant_dom const &slot_type_dom = slot_type_dom_obj;

constant
value::get_type_const () const
{
  return {get_type ().code (), &slot_type_dom};
}

std::ostream &
operator<< (std::ostream &o, value const &v)
{
  v.show (o);
  return o;
}

value_type const value_cst::vtype = value_type::alloc ("T_CONST");

void
value_cst::show (std::ostream &o) const
{
  o << m_cst;
}

std::unique_ptr <value>
value_cst::clone () const
{
  return std::make_unique <value_cst> (*this);
}

cmp_result
value_cst::cmp (value const &that) const
{
  if (auto v = value::as <value_cst> (&that))
    {
      // We don't want to evaluate as equal two constants from
      // different domains just because they happen to have the same
      // value.
      if (! m_cst.dom ()->safe_arith () || ! v->m_cst.dom ()->safe_arith ())
	{
	  cmp_result ret = compare (m_cst.dom (), v->m_cst.dom ());
	  if (ret != cmp_result::equal)
	    return ret;
	}

      // Either they are both arithmetic, or they are both from the
      // same non-arithmetic domain.  We can directly compare the
      // values now.
      return compare (m_cst, v->m_cst);
    }
  else
    return cmp_result::fail;
}


value_type const value_closure::vtype = value_type::alloc ("T_CLOSURE");

value_closure::value_closure (tree const &t, dwgrep_graph::sptr q,
			      std::shared_ptr <scope> scope,
			      std::shared_ptr <frame> frame, size_t pos)
  : value {vtype, pos}
  , m_t {std::make_unique <tree> (t)}
  , m_q {q}
  , m_scope {scope}
  , m_frame {frame}
{}

value_closure::value_closure (value_closure const &that)
  : value_closure {*that.m_t, that.m_q, that.m_scope, that.m_frame,
		   that.get_pos ()}
{}

value_closure::~value_closure()
{}

void
value_closure::show (std::ostream &o) const
{
  o << "closure(" << *m_t << ")";
}

std::unique_ptr <value>
value_closure::clone () const
{
  return std::make_unique <value_closure> (*this);
}

cmp_result
value_closure::cmp (value const &v) const
{
  if (auto that = value::as <value_closure> (&v))
    {
      auto a = std::make_tuple (static_cast <tree &> (*m_t), m_scope, m_frame);
      auto b = std::make_tuple (static_cast <tree &> (*that->m_t),
				that->m_scope, that->m_frame);
      return compare (a, b);
    }
  else
    return cmp_result::fail;
}
