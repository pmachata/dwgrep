#include "builtin-arith.hh"
#include "builtin-add.hh"
#include "builtin-length.hh"
#include "value-seq.hh"
#include "value-str.hh"

namespace
{
  builtin_sub builtin_sub_obj;
  builtin_mul builtin_mul_obj;
  builtin_div builtin_div_obj;
  builtin_mod builtin_mod_obj;

  builtin_add builtin_add_obj;
  overload_builtin <op_add_str> builtin_add_str_obj;
  overload_builtin <op_add_cst> builtin_add_cst_obj;
  overload_builtin <op_add_seq> builtin_add_seq_obj;

  builtin_length builtin_length_obj;
  overload_builtin <op_length_str> builtin_length_str_obj;
  overload_builtin <op_length_seq> builtin_length_seq_obj;
}

void
dwgrep_init ()
{
  add_builtin (builtin_sub_obj);
  add_builtin (builtin_mul_obj);
  add_builtin (builtin_div_obj);
  add_builtin (builtin_mod_obj);

  add_builtin (builtin_add_obj);
  ovl_tab_add ().add_overload (value_cst::vtype, builtin_add_cst_obj);
  ovl_tab_add ().add_overload (value_str::vtype, builtin_add_str_obj);
  ovl_tab_add ().add_overload (value_seq::vtype, builtin_add_seq_obj);

  add_builtin (builtin_length_obj);
  ovl_tab_length ().add_overload (value_str::vtype, builtin_length_str_obj);
  ovl_tab_length ().add_overload (value_seq::vtype, builtin_length_seq_obj);
}
