/*
   Copyright (C) 2018 Petr Machata
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

#include "builtin-core-voc.hh"
#include "builtin-closure.hh"
#include "builtin-cmp.hh"
#include "builtin-cst.hh"
#include "builtin-shf.hh"

std::unique_ptr <vocabulary>
dwgrep_vocabulary_core ()
{
  auto voc = std::make_unique <vocabulary> ();

  add_builtin_type_constant <value_cst> (*voc);
  add_builtin_type_constant <value_str> (*voc);
  add_builtin_type_constant <value_seq> (*voc);
  add_builtin_type_constant <value_closure> (*voc);

  // closure builtins
  voc->add (std::make_shared <builtin_apply> ());

  // comparison assertions
  {
    auto eq = std::make_shared <builtin_eq> (true);
    auto neq = std::make_shared <builtin_eq> (false);

    auto lt = std::make_shared <builtin_lt> (true);
    auto nlt = std::make_shared <builtin_lt> (false);

    auto gt = std::make_shared <builtin_gt> (true);
    auto ngt = std::make_shared <builtin_gt> (false);

    voc->add (eq);
    voc->add (neq);
    voc->add (lt);
    voc->add (nlt);
    voc->add (gt);
    voc->add (ngt);

    voc->add (neq, "?ne");
    voc->add (eq, "!ne");
    voc->add (nlt, "?ge");
    voc->add (lt, "!ge");
    voc->add (ngt, "?le");
    voc->add (gt, "!le");

    voc->add (eq, "==");
    voc->add (neq, "!=");
    voc->add (lt, "<");
    voc->add (nlt, ">=");
    voc->add (gt, ">");
    voc->add (ngt, "<=");
  }

  // domain conversions
  voc->add (std::make_shared <builtin_hex> ());
  voc->add (std::make_shared <builtin_dec> ());
  voc->add (std::make_shared <builtin_oct> ());
  voc->add (std::make_shared <builtin_bin> ());

  add_builtin_constant (*voc, constant (0, &bool_constant_dom), "false");
  add_builtin_constant (*voc, constant (1, &bool_constant_dom), "true");
  voc->add (std::make_shared <simple_exec_builtin <op_type>> ("type"));
  voc->add (std::make_shared <simple_exec_builtin <op_pos>> ("pos"));

  // stack shuffling
  voc->add (std::make_shared <simple_exec_builtin <op_drop>> ("drop"));
  voc->add (std::make_shared <simple_exec_builtin <op_swap>> ("swap"));
  voc->add (std::make_shared <simple_exec_builtin <op_dup>> ("dup"));
  voc->add (std::make_shared <simple_exec_builtin <op_over>> ("over"));
  voc->add (std::make_shared <simple_exec_builtin <op_rot>> ("rot"));

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_add_cst> ();
    t->add_op_overload <op_add_str> ();
    t->add_op_overload <op_add_seq> ();

    voc->add (std::make_shared <overloaded_op_builtin> ("add", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_sub_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("sub", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_mul_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("mul", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_div_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("div", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_mod_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("mod", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_and_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("and", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_or_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("or", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_xor_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("xor", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_neg_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("neg", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_shl_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("shl", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_shr_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("shr", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elem_str> ();
    t->add_op_overload <op_elem_seq> ();

    voc->add (std::make_shared <overloaded_op_builtin> ("elem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_relem_str> ();
    t->add_op_overload <op_relem_seq> ();

    voc->add (std::make_shared <overloaded_op_builtin> ("relem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_empty_str> ();
    t->add_pred_overload <pred_empty_seq> ();

    voc->add
      (std::make_shared <overloaded_pred_builtin> ("?empty", t, true));
    voc->add
      (std::make_shared <overloaded_pred_builtin> ("!empty", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_find_str> ();
    t->add_pred_overload <pred_find_seq> ();

    voc->add (std::make_shared <overloaded_pred_builtin> ("?find", t, true));
    voc->add (std::make_shared <overloaded_pred_builtin> ("!find", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_starts_str> ();
    t->add_pred_overload <pred_starts_seq> ();

    voc->add (std::make_shared <overloaded_pred_builtin> ("?starts", t, true));
    voc->add (std::make_shared <overloaded_pred_builtin> ("!starts", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_ends_str> ();
    t->add_pred_overload <pred_ends_seq> ();

    voc->add (std::make_shared <overloaded_pred_builtin> ("?ends", t, true));
    voc->add (std::make_shared <overloaded_pred_builtin> ("!ends", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_match_str> ();

    voc->add
      (std::make_shared <overloaded_pred_builtin> ("?match", t, true));
    voc->add
      (std::make_shared <overloaded_pred_builtin> ("!match", t, false));

    voc->add (std::make_shared <overloaded_pred_builtin> ("=~", t, true));
    voc->add (std::make_shared <overloaded_pred_builtin> ("!~", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_length_str> ();
    t->add_op_overload <op_length_seq> ();

    voc->add (std::make_shared <overloaded_op_builtin> ("length", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_value_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();
    t->add_op_overload <op_bit_cst> ();
    voc->add (std::make_shared <overloaded_op_builtin> ("bit", t));
  }

  return voc;
}
