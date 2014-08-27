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

#include "overload.hh"

#include "value-closure.hh"
#include "value-cst.hh"
#include "value-seq.hh"
#include "value-str.hh"

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

    dict->add (eq, "==");
    dict->add (neq, "!=");
    dict->add (lt, "<");
    dict->add (nlt, ">=");
    dict->add (gt, ">");
    dict->add (ngt, "<=");
  }

  // domain conversions
  dict->add (std::make_shared <builtin_hex> ());
  dict->add (std::make_shared <builtin_dec> ());
  dict->add (std::make_shared <builtin_oct> ());
  dict->add (std::make_shared <builtin_bin> ());

  add_builtin_constant (*dict, constant (0, &bool_constant_dom), "false");
  add_builtin_constant (*dict, constant (1, &bool_constant_dom), "true");
  add_simple_exec_builtin <op_type> (*dict, "type");
  add_simple_exec_builtin <op_pos> (*dict, "pos");

  // stack shuffling
  add_simple_exec_builtin <op_drop> (*dict, "drop");
  add_simple_exec_builtin <op_swap> (*dict, "swap");
  add_simple_exec_builtin <op_dup> (*dict, "dup");
  add_simple_exec_builtin <op_over> (*dict, "over");
  add_simple_exec_builtin <op_rot> (*dict, "rot");

  // "add"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_add_cst> ();
    t->add_op_overload <op_add_str> ();
    t->add_op_overload <op_add_seq> ();

    dict->add (std::make_shared <overloaded_op_builtin> ("add", t));
  }

  // "sub"
  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_sub_cst> ();
    dict->add (std::make_shared <overloaded_op_builtin> ("sub", t));
  }

  // "mul"
  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_mul_cst> ();
    dict->add (std::make_shared <overloaded_op_builtin> ("mul", t));
  }

  // "div"
  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_div_cst> ();
    dict->add (std::make_shared <overloaded_op_builtin> ("div", t));
  }

  // "mod"
  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_mod_cst> ();
    dict->add (std::make_shared <overloaded_op_builtin> ("mod", t));
  }

  // "elem"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elem_str> ();
    t->add_op_overload <op_elem_seq> ();

    dict->add (std::make_shared <overloaded_op_builtin> ("elem", t));
  }

  // "relem"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_relem_str> ();
    t->add_op_overload <op_relem_seq> ();

    dict->add (std::make_shared <overloaded_op_builtin> ("relem", t));
  }

  // "empty"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_empty_str> ();
    t->add_pred_overload <pred_empty_seq> ();

    dict->add
      (std::make_shared <overloaded_pred_builtin <true>> ("?empty", t));
    dict->add
      (std::make_shared <overloaded_pred_builtin <false>> ("!empty", t));
  }

  // "find"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_find_str> ();
    t->add_pred_overload <pred_find_seq> ();

    dict->add (std::make_shared <overloaded_pred_builtin <true>> ("?find", t));
    dict->add (std::make_shared <overloaded_pred_builtin <false>> ("!find", t));
  }

  // "match"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_match_str> ();

    dict->add
      (std::make_shared <overloaded_pred_builtin <true>> ("?match", t));
    dict->add
      (std::make_shared <overloaded_pred_builtin <false>> ("!match", t));

    dict->add (std::make_shared <overloaded_pred_builtin <true>> ("=~", t));
    dict->add (std::make_shared <overloaded_pred_builtin <false>> ("!~", t));
  }

  // "length"
  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_length_str> ();
    t->add_op_overload <op_length_seq> ();

    dict->add (std::make_shared <overloaded_op_builtin> ("length", t));
  }

  // "value"
  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_value_cst> ();
    dict->add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  return dict;
}
