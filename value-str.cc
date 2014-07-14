#include "value-str.hh"
#include "vfcst.hh"

value_type const value_str::vtype = value_type::alloc ("T_STR");

void
value_str::show (std::ostream &o) const
{
  o << m_str;
}

std::unique_ptr <value>
value_str::clone () const
{
  return std::make_unique <value_str> (*this);
}

constant
value_str::get_type_const () const
{
  return {(int) slot_type_id::T_STR, &slot_type_dom};
}

cmp_result
value_str::cmp (value const &that) const
{
  if (auto v = value::as <value_str> (&that))
    return compare (m_str, v->m_str);
  else
    return cmp_result::fail;
}

valfile::uptr
op_add_str::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_str> ();

      // XXX add arity to the framework
      auto wp = vf->pop ();
      assert (wp->is <value_str> ());
      auto &w = static_cast <value_str &> (*wp);

      std::string result = w.get_string () + vp->get_string ();
      vf->push (std::make_unique <value_str> (std::move (result), 0));
      return vf;
    }

  return nullptr;
}

valfile::uptr
op_length_str::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto vp = vf->pop_as <value_str> ();
      constant t {vp->get_string ().length (), &dec_constant_dom};
      vf->push (std::make_unique <value_cst> (t, 0));
      return vf;
    }

  return nullptr;
}
