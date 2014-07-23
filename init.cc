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

namespace
{
  // value types for builtin types
  builtin_constant builtin_T_CONST_obj
	{std::make_unique <value_cst>
	    (value::get_type_const_of <value_cst> (), 0)};

  builtin_constant builtin_T_STR_obj
	{std::make_unique <value_cst>
	    (value::get_type_const_of <value_str> (), 0)};

  builtin_constant builtin_T_SEQ_obj
	{std::make_unique <value_cst>
	    (value::get_type_const_of <value_seq> (), 0)};

  builtin_constant builtin_T_CLOSURE_obj
	{std::make_unique <value_cst>
	    (value::get_type_const_of <value_closure> (), 0)};

  // arithmetic, except for add, which is an overload
  builtin_sub builtin_sub_obj;
  builtin_mul builtin_mul_obj;
  builtin_div builtin_div_obj;
  builtin_mod builtin_mod_obj;

  // closure builtins
  builtin_apply builtin_apply_obj;

  // comparison assertions
  builtin_eq builtin_eq_obj {true}, builtin_neq_obj {false};
  builtin_lt builtin_lt_obj {true}, builtin_nlt_obj {false};
  builtin_gt builtin_gt_obj {true}, builtin_ngt_obj {false};

  // constants
  builtin_hex builtin_hex_obj;
  builtin_dec builtin_dec_obj;
  builtin_oct builtin_oct_obj;
  builtin_bin builtin_bin_obj;
  builtin_constant builtin_false_obj
	{std::make_unique <value_cst> (constant (0, &bool_constant_dom), 0)};
  builtin_constant builtin_true_obj
	{std::make_unique <value_cst> (constant (1, &bool_constant_dom), 0)};
  builtin_type builtin_type_obj;
  builtin_pos builtin_pos_obj;

  // stack shuffling
  builtin_drop builtin_drop_obj;
  builtin_swap builtin_swap_obj;
  builtin_dup builtin_dup_obj;
  builtin_over builtin_over_obj;
  builtin_rot builtin_rot_obj;

  // "add"
  builtin_add builtin_add_obj;
  overload_op_builtin <op_add_str> builtin_add_str_obj;
  overload_op_builtin <op_add_cst> builtin_add_cst_obj;
  overload_op_builtin <op_add_seq> builtin_add_seq_obj;

  // "elem"
  builtin_elem builtin_elem_obj;
  overload_op_builtin <op_elem_str> builtin_elem_str_obj;
  overload_op_builtin <op_elem_seq> builtin_elem_seq_obj;

  // "empty"
  builtin_empty builtin_empty_obj {true}, builtin_nempty_obj {false};
  overload_pred_builtin <pred_empty_str> builtin_empty_str_obj;
  overload_pred_builtin <pred_empty_seq> builtin_empty_seq_obj;

  // "find"
  builtin_find builtin_find_obj {true}, builtin_nfind_obj {false};
  overload_pred_builtin <pred_find_str> builtin_find_str_obj;
  overload_pred_builtin <pred_find_seq> builtin_find_seq_obj;

  // "length"
  builtin_length builtin_length_obj;
  overload_op_builtin <op_length_str> builtin_length_str_obj;
  overload_op_builtin <op_length_seq> builtin_length_seq_obj;

  // "value"
  builtin_value builtin_value_obj;
  overload_op_builtin <op_value_cst> builtin_value_cst_obj;
}

void
dwgrep_init ()
{
  add_builtin (builtin_T_CONST_obj, value_cst::vtype.name ());
  add_builtin (builtin_T_STR_obj, value_str::vtype.name ());
  add_builtin (builtin_T_SEQ_obj, value_seq::vtype.name ());
  add_builtin (builtin_T_CLOSURE_obj, value_closure::vtype.name ());

  add_builtin (builtin_sub_obj);
  add_builtin (builtin_mul_obj);
  add_builtin (builtin_div_obj);
  add_builtin (builtin_mod_obj);

  add_builtin (builtin_apply_obj);

  add_builtin (builtin_eq_obj);
  add_builtin (builtin_neq_obj);
  add_builtin (builtin_lt_obj);
  add_builtin (builtin_nlt_obj);
  add_builtin (builtin_gt_obj);
  add_builtin (builtin_ngt_obj);

  add_builtin (builtin_neq_obj, "?ne");
  add_builtin (builtin_eq_obj, "!ne");
  add_builtin (builtin_nlt_obj, "?ge");
  add_builtin (builtin_lt_obj, "!ge");
  add_builtin (builtin_ngt_obj, "?le");
  add_builtin (builtin_gt_obj, "!le");

  add_builtin (builtin_hex_obj);
  add_builtin (builtin_dec_obj);
  add_builtin (builtin_oct_obj);
  add_builtin (builtin_bin_obj);
  add_builtin (builtin_false_obj, "false");
  add_builtin (builtin_true_obj, "true");
  add_builtin (builtin_type_obj);
  add_builtin (builtin_pos_obj);

  add_builtin (builtin_drop_obj);
  add_builtin (builtin_swap_obj);
  add_builtin (builtin_dup_obj);
  add_builtin (builtin_over_obj);
  add_builtin (builtin_rot_obj);

  add_builtin (builtin_add_obj);
  ovl_tab_add ().add_overload (value_cst::vtype, builtin_add_cst_obj);
  ovl_tab_add ().add_overload (value_str::vtype, builtin_add_str_obj);
  ovl_tab_add ().add_overload (value_seq::vtype, builtin_add_seq_obj);

  add_builtin (builtin_length_obj);
  ovl_tab_length ().add_overload (value_str::vtype, builtin_length_str_obj);
  ovl_tab_length ().add_overload (value_seq::vtype, builtin_length_seq_obj);

  add_builtin (builtin_value_obj);
  ovl_tab_value ().add_overload (value_cst::vtype, builtin_value_cst_obj);

  add_builtin (builtin_elem_obj);
  ovl_tab_elem ().add_overload (value_str::vtype, builtin_elem_str_obj);
  ovl_tab_elem ().add_overload (value_seq::vtype, builtin_elem_seq_obj);

  add_builtin (builtin_empty_obj);
  add_builtin (builtin_nempty_obj);
  ovl_tab_empty ().add_overload (value_str::vtype, builtin_empty_str_obj);
  ovl_tab_empty ().add_overload (value_seq::vtype, builtin_empty_seq_obj);

  add_builtin (builtin_find_obj);
  add_builtin (builtin_nfind_obj);
  ovl_tab_find ().add_overload (value_str::vtype, builtin_find_str_obj);
  ovl_tab_find ().add_overload (value_seq::vtype, builtin_find_seq_obj);
}
