#include "builtin-arith.hh"
#include "builtin-cmp.hh"
#include "builtin-shf.hh"
#include "builtin-add.hh"
#include "builtin-length.hh"
#include "builtin-value.hh"
#include "builtin-elem.hh"
#include "value-seq.hh"
#include "value-str.hh"

namespace
{
  // arithmetic, except for add, which is an overload
  builtin_sub builtin_sub_obj;
  builtin_mul builtin_mul_obj;
  builtin_div builtin_div_obj;
  builtin_mod builtin_mod_obj;

  // comparison assertions
  builtin_eq builtin_eq_obj {true}, builtin_neq_obj {false};
  builtin_lt builtin_lt_obj {true}, builtin_nlt_obj {false};
  builtin_gt builtin_gt_obj {true}, builtin_ngt_obj {false};

  // stack shuffling
  builtin_drop builtin_drop_obj;
  builtin_swap builtin_swap_obj;
  builtin_dup builtin_dup_obj;
  builtin_over builtin_over_obj;
  builtin_rot builtin_rot_obj;

  // "add"
  builtin_add builtin_add_obj;
  overload_builtin <op_add_str> builtin_add_str_obj;
  overload_builtin <op_add_cst> builtin_add_cst_obj;
  overload_builtin <op_add_seq> builtin_add_seq_obj;

  // "length"
  builtin_length builtin_length_obj;
  overload_builtin <op_length_str> builtin_length_str_obj;
  overload_builtin <op_length_seq> builtin_length_seq_obj;

  // "value"
  builtin_value builtin_value_obj;
  overload_builtin <op_value_cst> builtin_value_cst_obj;

  // "elem"
  builtin_elem builtin_elem_obj;
  overload_builtin <op_elem_str> builtin_elem_str_obj;
  overload_builtin <op_elem_seq> builtin_elem_seq_obj;
}

void
dwgrep_init ()
{
  add_builtin (builtin_sub_obj);
  add_builtin (builtin_mul_obj);
  add_builtin (builtin_div_obj);
  add_builtin (builtin_mod_obj);

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
}
