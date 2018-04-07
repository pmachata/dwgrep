/*
   Copyright (C) 2018 Petr Machata
   Copyright (C) 2014, 2015 Red Hat, Inc.
   This file is part of dwgrep.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   dwgrep is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include <iostream>
#include <memory>
#include <algorithm>

#include "op.hh"
#include "tree.hh"
#include "value.hh"

value_type
value_type::alloc (char const *name, char const *docstring)
{
  static uint8_t last = 0;
  value_type vt {++last, name, docstring};
  if (last == 0)
    {
      std::cerr << "Ran out of value type identifiers." << std::endl;
      std::terminate ();
    }
  return vt;
}

value_type const value::vtype = value_type::alloc ("T_???");

namespace
{
  std::vector <std::pair <uint8_t, char const *>> &
  get_vtype_names ()
  {
    static std::vector <std::pair <uint8_t, char const *>> names;
    return names;
  }

  std::vector <std::pair <uint8_t, std::string>> &
  get_vtype_docstrings ()
  {
    static std::vector <std::pair <uint8_t, std::string>> docstrings;
    return docstrings;
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
value_type::register_type (uint8_t code,
			   char const *name, char const *docstring)
{
  auto &vtn = get_vtype_names ();
  assert (find_vtype_name (code) == nullptr);
  vtn.push_back (std::make_pair (code, name));
  get_vtype_docstrings ().push_back (std::make_pair (code, docstring));
}

char const *
value_type::name () const
{
  auto ret = find_vtype_name (m_code);
  assert (ret != nullptr);
  return ret;
}

std::vector <std::pair <uint8_t, std::string>>
value_type::get_docstrings ()
{
  return get_vtype_docstrings ();
}

std::vector <std::pair <uint8_t, char const *>>
value_type::get_names ()
{
  return get_vtype_names ();
}

std::ostream &
operator<< (std::ostream &o, value_type const &v)
{
  return o << v.name ();
}

static struct
  : public constant_dom
{
  void
  show_invalid (mpz_class const &v, std::ostream &o) const
  {
    o << "T_<invalid type code " << v << ">";
  }

  void
  show (mpz_class const &v, std::ostream &o, brevity brv) const override
  {
    if (v < 0)
      return show_invalid (v, o);

    uint64_t ui = v.uval ();
    static_assert (sizeof (value_type) == 1,
		   "assuming value_type is 8 bits large");
    if (ui > 0xff)
      return show_invalid (v, o);

    char const *name = find_vtype_name (ui);
    if (name == nullptr)
      return show_invalid (v, o);

    assert (name[0] == 'T' && name[1] == '_');
    o << (&name[brv == brevity::full ? 0 : 2]);
  }

  char const *name () const override
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
