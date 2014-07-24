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

#include "value-seq.hh"
#include "value-str.hh"
#include "value-closure.hh"

#include "builtin-arith.hh"
#include "builtin-closure.hh"
#include "builtin-cmp.hh"
#include "builtin-cst.hh"
#include "builtin-shf.hh"

#include "builtin-add.hh"
#include "builtin-elem.hh"
#include "builtin-empty.hh"
#include "builtin-find.hh"
#include "builtin-length.hh"
#include "builtin-value.hh"

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
  dict->add (std::make_shared <builtin_add> ());
  {
    // XXX
    static overload_op_builtin <op_add_str> builtin_add_str_obj;
    static overload_op_builtin <op_add_cst> builtin_add_cst_obj;
    static overload_op_builtin <op_add_seq> builtin_add_seq_obj;
    ovl_tab_add ().add_overload (value_cst::vtype, builtin_add_cst_obj);
    ovl_tab_add ().add_overload (value_str::vtype, builtin_add_str_obj);
    ovl_tab_add ().add_overload (value_seq::vtype, builtin_add_seq_obj);
  }

  // "elem"
  dict->add (std::make_shared <builtin_elem> ());
  {
    // XXX
    static overload_op_builtin <op_elem_str> builtin_elem_str_obj;
    static overload_op_builtin <op_elem_seq> builtin_elem_seq_obj;
    ovl_tab_elem ().add_overload (value_str::vtype, builtin_elem_str_obj);
    ovl_tab_elem ().add_overload (value_seq::vtype, builtin_elem_seq_obj);
  }

  // "empty"
  dict->add (std::make_shared <builtin_empty> (true));
  dict->add (std::make_shared <builtin_empty> (false));
  {
    // XXX -- note that the overload table is shared among the two
    // builtins
    static overload_pred_builtin <pred_empty_str> builtin_empty_str_obj;
    static overload_pred_builtin <pred_empty_seq> builtin_empty_seq_obj;
    ovl_tab_empty ().add_overload (value_str::vtype, builtin_empty_str_obj);
    ovl_tab_empty ().add_overload (value_seq::vtype, builtin_empty_seq_obj);
  }

  // "find"
  dict->add (std::make_shared <builtin_find> (true));
  dict->add (std::make_shared <builtin_find> (false));
  {
    // XXX
    static overload_pred_builtin <pred_find_str> builtin_find_str_obj;
    static overload_pred_builtin <pred_find_seq> builtin_find_seq_obj;
    ovl_tab_find ().add_overload (value_str::vtype, builtin_find_str_obj);
    ovl_tab_find ().add_overload (value_seq::vtype, builtin_find_seq_obj);
  }

  // "length"
  dict->add (std::make_shared <builtin_length> ());
  {
    // XXX
    static overload_op_builtin <op_length_str> builtin_length_str_obj;
    static overload_op_builtin <op_length_seq> builtin_length_seq_obj;
    ovl_tab_length ().add_overload (value_str::vtype, builtin_length_str_obj);
    ovl_tab_length ().add_overload (value_seq::vtype, builtin_length_seq_obj);
  }

  // "value"
  dict->add (std::make_shared <builtin_value> ());
  {
    // XXX
    static overload_op_builtin <op_value_cst> builtin_value_cst_obj;
    ovl_tab_value ().add_overload (value_cst::vtype, builtin_value_cst_obj);
  }

  return dict;
}
