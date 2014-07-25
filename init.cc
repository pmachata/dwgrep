/*
   Copyright (C) 2014 Red Hat, Inc.
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

#include <memory>
#include "make_unique.hh"

#include "overload.hh"

#include "value-closure.hh"
#include "value-cst.hh"
#include "value-seq.hh"
#include "value-str.hh"

#include "builtin-arith.hh"
#include "builtin-closure.hh"
#include "builtin-cmp.hh"
#include "builtin-cst.hh"
#include "builtin-shf.hh"

std::unique_ptr <builtin_dict>
dwgrep_builtins_core ()
{
  auto dict = std::make_unique <builtin_dict> ();

  add_builtin_type_constant <value_cst> (*dict);
  add_builtin_type_constant <value_str> (*dict);
  add_builtin_type_constant <value_seq> (*dict);
  add_builtin_type_constant <value_closure> (*dict);

  // arithmetic, except for add, which is an overload
  dict->add (std::make_shared <builtin_sub> ());
  dict->add (std::make_shared <builtin_mul> ());
  dict->add (std::make_shared <builtin_div> ());
  dict->add (std::make_shared <builtin_mod> ());

  // closure builtins
  dict->add (std::make_shared <builtin_apply> ());

  // comparison assertions
  {
    auto eq = std::make_shared <builtin_eq> (true);
    auto neq = std::make_shared <builtin_eq> (false);

    auto lt = std::make_shared <builtin_lt> (true);
    auto nlt = std::make_shared <builtin_lt> (false);

    auto gt = std::make_shared <builtin_gt> (true);
    auto ngt = std::make_shared <builtin_gt> (false);

    dict->add (eq);
    dict->add (neq);
    dict->add (lt);
    dict->add (nlt);
    dict->add (gt);
    dict->add (ngt);

    dict->add (neq, "?ne");
    dict->add (eq, "!ne");
    dict->add (nlt, "?ge");
    dict->add (lt, "!ge");
    dict->add (ngt, "?le");
    dict->add (gt, "!le");
  }

  // constants
  dict->add (std::make_shared <builtin_hex> ());
  dict->add (std::make_shared <builtin_dec> ());
  dict->add (std::make_shared <builtin_oct> ());
  dict->add (std::make_shared <builtin_bin> ());
  add_builtin_constant (*dict, constant (0, &bool_constant_dom), "false");
  add_builtin_constant (*dict, constant (1, &bool_constant_dom), "true");
  dict->add (std::make_shared <builtin_type> ());
  dict->add (std::make_shared <builtin_pos> ());

  // stack shuffling
  dict->add (std::make_shared <builtin_drop> ());
  dict->add (std::make_shared <builtin_swap> ());
  dict->add (std::make_shared <builtin_dup> ());
  dict->add (std::make_shared <builtin_over> ());
  dict->add (std::make_shared <builtin_rot> ());

  // strings
  dict->add (std::make_shared <builtin_match> (true));
  dict->add (std::make_shared <builtin_match> (false));

  // "add"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_add_cst> ();
    t->add_simple_op_overload <op_add_str> ();
    t->add_simple_op_overload <op_add_seq> ();

    dict->add (std::make_shared <overloaded_op_builtin> ("add", t));
  }

  // "elem"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_elem_str> ();
    t->add_simple_op_overload <op_elem_seq> ();

    dict->add (std::make_shared <overloaded_op_builtin> ("elem", t));
  }

  // "empty"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_pred_overload <pred_empty_str> ();
    t->add_simple_pred_overload <pred_empty_seq> ();

    dict->add (std::make_shared <overloaded_pred_builtin> ("?empty", t));
    dict->add (std::make_shared <overloaded_pred_builtin> ("!empty", t));
  }

  // "find"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_pred_overload <pred_find_str> ();
    t->add_simple_pred_overload <pred_find_seq> ();

    dict->add (std::make_shared <overloaded_pred_builtin> ("?find", t));
    dict->add (std::make_shared <overloaded_pred_builtin> ("!find", t));
  }

  // "length"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_simple_op_overload <op_length_str> ();
    t->add_simple_op_overload <op_length_seq> ();

    dict->add (std::make_shared <overloaded_op_builtin> ("length", t));
  }

  // "value"
  {
    auto t = std::make_shared <overload_tab> ();
    t->add_simple_op_overload <op_value_cst> ();
    dict->add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  return dict;
}
