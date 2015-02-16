/*
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

#ifndef _BUILTIN_DW_H_
#define _BUILTIN_DW_H_

#include <memory>

#include "overload.hh"
#include "value-dw.hh"
#include "value-str.hh"

struct vocabulary;
std::unique_ptr <vocabulary> dwgrep_vocabulary_dw ();

struct op_dwopen_str
  : public op_once_overload <value_dwarf, value_str>
{
  using op_once_overload::op_once_overload;

  value_dwarf operate (std::unique_ptr <value_str> a) override;

  static std::string docstring ();
};

struct op_unit_dwarf
  : public op_yielding_overload <value_cu, value_dwarf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_cu>>
  operate (std::unique_ptr <value_dwarf> a) override;

  static std::string docstring ();
};

struct op_unit_die
  : public op_once_overload <value_cu, value_die>
{
  using op_once_overload::op_once_overload;

  value_cu operate (std::unique_ptr <value_die> a) override;

  static std::string docstring ();
};

struct op_unit_attr
  : public op_once_overload <value_cu, value_attr>
{
  using op_once_overload::op_once_overload;

  value_cu operate (std::unique_ptr <value_attr> a) override;

  static std::string docstring ();
};

struct op_entry_cu
  : public op_yielding_overload <value_die, value_cu>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_die>>
  operate (std::unique_ptr <value_cu> a) override;

  static std::string docstring ();
};

struct op_entry_dwarf
  : public op_yielding_overload <value_die, value_dwarf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_die>>
  operate (std::unique_ptr <value_dwarf> a) override;

  static std::string docstring ();
};

struct op_child_die
  : public op_yielding_overload <value_die, value_die>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_die>>
  operate (std::unique_ptr <value_die> a) override;

  static std::string docstring ();
};

struct op_elem_loclist_elem
  : public op_yielding_overload <value_loclist_op, value_loclist_elem>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_loclist_op>>
  operate (std::unique_ptr <value_loclist_elem> a) override;

  static std::string docstring ();
};

struct op_relem_loclist_elem
  : public op_yielding_overload <value_loclist_op, value_loclist_elem>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_loclist_op>>
  operate (std::unique_ptr <value_loclist_elem> a) override;

  static std::string docstring ();
};

struct op_elem_aset
  : public op_yielding_overload <value_cst, value_aset>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_cst>>
  operate (std::unique_ptr <value_aset> val) override;

  static std::string docstring ();
};

struct op_relem_aset
  : public op_yielding_overload <value_cst, value_aset>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_cst>>
  operate (std::unique_ptr <value_aset> val) override;

  static std::string docstring ();
};

struct op_attribute_die
  : public op_yielding_overload <value_attr, value_die>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_attr>>
  operate (std::unique_ptr <value_die> a) override;

  static std::string docstring ();
};

struct op_offset_cu
  : public op_once_overload <value_cst, value_cu>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_cu> a) override;
  static std::string docstring ();
};

struct op_offset_die
  : public op_once_overload <value_cst, value_die>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_die> val) override;
  static std::string docstring ();
};

struct op_offset_loclist_op
  : public op_once_overload <value_cst, value_loclist_op>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_loclist_op> val) override;
  static std::string docstring ();
};

struct op_address_die
  : public op_once_overload <value_aset, value_die>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_address_attr
  : public op_overload <value_cst, value_attr>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst> operate (std::unique_ptr <value_attr> a) override;
  static std::string docstring ();
};

struct op_address_loclist_elem
  : public op_once_overload <value_aset, value_loclist_elem>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_loclist_elem> val) override;
  static std::string docstring ();
};

struct op_label_die
  : public op_once_overload <value_cst, value_die>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_die> val) override;
  static std::string docstring ();
};

struct op_label_attr
  : public op_once_overload <value_cst, value_attr>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_attr> val) override;
  static std::string docstring ();
};

struct op_label_loclist_op
  : public op_once_overload <value_cst, value_loclist_op>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_loclist_op> val) override;
  static std::string docstring ();
};

struct op_form_attr
  : public op_once_overload <value_cst, value_attr>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_attr> val) override;
  static std::string docstring ();
};

struct op_parent_die
  : public op_overload <value_die, value_die>
{
  using op_overload::op_overload;

  std::unique_ptr <value_die> operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct pred_rootp_die
  : public pred_overload <value_die>
{
  using pred_overload <value_die>::pred_overload;

  pred_result result (value_die &a) override;
  static std::string docstring ();
};

struct op_root_cu
  : public op_once_overload <value_die, value_cu>
{
  using op_once_overload::op_once_overload;

  value_die operate (std::unique_ptr <value_cu> a) override;
  static std::string docstring ();
};

struct op_root_die
  : public op_once_overload <value_die, value_die>
{
  using op_once_overload::op_once_overload;

  value_die operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_value_attr
  : public op_yielding_overload <value, value_attr>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value>>
  operate (std::unique_ptr <value_attr> a) override;

  static std::string docstring ();
};

struct op_value_loclist_op
  : public op_yielding_overload <value, value_loclist_op>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value>>
  operate (std::unique_ptr <value_loclist_op> a) override;

  static std::string docstring ();
};

struct op_low_die
  : public op_overload <value_cst, value_die>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst> operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_low_aset
  : public op_overload <value_cst, value_aset>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst> operate (std::unique_ptr <value_aset> a) override;
  static std::string docstring ();
};

struct op_high_die
  : public op_overload <value_cst, value_die>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst> operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_high_aset
  : public op_overload <value_cst, value_aset>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst> operate (std::unique_ptr <value_aset> a) override;
  static std::string docstring ();
};

struct op_aset_cst_cst
  : public op_once_overload <value_aset, value_cst, value_cst>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_cst> a,
		      std::unique_ptr <value_cst> b) override;

  static std::string docstring ();
};

struct op_add_aset_cst
  : public op_once_overload <value_aset, value_aset, value_cst>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_aset> a,
		      std::unique_ptr <value_cst> b) override;

  static std::string docstring ();
};

struct op_add_aset_aset
  : public op_once_overload <value_aset, value_aset, value_aset>
{
  using op_once_overload::op_once_overload;

  value_aset
  operate (std::unique_ptr <value_aset> a,
	   std::unique_ptr <value_aset> b) override;

  static std::string docstring ();
};

struct op_sub_aset_cst
  : public op_once_overload <value_aset, value_aset, value_cst>
{
  using op_once_overload::op_once_overload;

  value_aset
  operate (std::unique_ptr <value_aset> a,
	   std::unique_ptr <value_cst> b) override;

  static std::string docstring ();
};

struct op_sub_aset_aset
  : public op_once_overload <value_aset, value_aset, value_aset>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_aset> a,
		      std::unique_ptr <value_aset> b) override;

  static std::string docstring ();
};

struct op_length_aset
  : public op_once_overload <value_cst, value_aset>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_aset> a) override;
  static std::string docstring ();
};

struct op_length_loclist_elem
  : public op_once_overload <value_cst, value_loclist_elem>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_loclist_elem> a) override;
  static std::string docstring ();
};

struct op_range_aset
  : public op_yielding_overload <value_aset, value_aset>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_aset>>
  operate (std::unique_ptr <value_aset> a) override;

  static std::string docstring ();
};

struct pred_containsp_aset_cst
  : public pred_overload <value_aset, value_cst>
{
  using pred_overload::pred_overload;

  pred_result result (value_aset &a, value_cst &b) override;
  static std::string docstring ();
};

struct pred_containsp_aset_aset
  : public pred_overload <value_aset, value_aset>
{
  using pred_overload::pred_overload;

  pred_result result (value_aset &a, value_aset &b) override;
  static std::string docstring ();
};

struct pred_overlapsp_aset_aset
  : public pred_overload <value_aset, value_aset>
{
  using pred_overload::pred_overload;

  pred_result result (value_aset &a, value_aset &b) override;
  static std::string docstring ();
};

struct op_overlap_aset_aset
  : public op_once_overload <value_aset, value_aset, value_aset>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_aset> a,
		      std::unique_ptr <value_aset> b) override;

  static std::string docstring ();
};

struct pred_emptyp_aset
  : public pred_overload <value_aset>
{
  using pred_overload::pred_overload;

  pred_result result (value_aset &a) override;
  static std::string docstring ();
};

struct pred_haschildrenp_die
  : public pred_overload <value_die>
{
  using pred_overload::pred_overload;

  pred_result result (value_die &a) override;
  static std::string docstring ();
};

struct op_version_cu
  : public op_once_overload <value_cst, value_cu>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_cu> a) override;
  static std::string docstring ();
};

struct op_name_dwarf
  : public op_once_overload <value_str, value_dwarf>
{
  using op_once_overload::op_once_overload;

  value_str operate (std::unique_ptr <value_dwarf> a) override;
  static std::string docstring ();
};

struct op_name_die
  : public op_overload <value_str, value_die>
{
  using op_overload::op_overload;

  std::unique_ptr <value_str> operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_raw_dwarf
  : public op_once_overload <value_dwarf, value_dwarf>
{
  using op_once_overload::op_once_overload;

  value_dwarf operate (std::unique_ptr <value_dwarf> a) override;
  static std::string docstring ();
};

struct op_raw_cu
  : public op_once_overload <value_cu, value_cu>
{
  using op_once_overload::op_once_overload;

  value_cu operate (std::unique_ptr <value_cu> a) override;
  static std::string docstring ();
};

struct op_raw_die
  : public op_once_overload <value_die, value_die>
{
  using op_once_overload::op_once_overload;

  value_die operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_raw_attr
  : public op_once_overload <value_attr, value_attr>
{
  using op_once_overload::op_once_overload;

  value_attr operate (std::unique_ptr <value_attr> a) override;
  static std::string docstring ();
};

struct op_cooked_dwarf
  : public op_once_overload <value_dwarf, value_dwarf>
{
  using op_once_overload::op_once_overload;

  value_dwarf operate (std::unique_ptr <value_dwarf> a) override;
  static std::string docstring ();
};

struct op_cooked_cu
  : public op_once_overload <value_cu, value_cu>
{
  using op_once_overload::op_once_overload;

  value_cu operate (std::unique_ptr <value_cu> a) override;
  static std::string docstring ();
};

struct op_cooked_die
  : public op_once_overload <value_die, value_die>
{
  using op_once_overload::op_once_overload;

  value_die operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_cooked_attr
  : public op_once_overload <value_attr, value_attr>
{
  using op_once_overload::op_once_overload;

  value_attr operate (std::unique_ptr <value_attr> a) override;
  static std::string docstring ();
};

class op_atval_die
  : public op_yielding_overload <value, value_die>
{
  int m_atname;

public:
  op_atval_die (std::shared_ptr <op> upstream, int atname)
    : op_yielding_overload {upstream}
    , m_atname {atname}
  {}

  std::unique_ptr <value_producer <value>>
  operate (std::unique_ptr <value_die> a);

  static std::string docstring ();
};

class pred_atname_die
  : public pred_overload <value_die>
{
  unsigned m_atname;

public:
  pred_atname_die (unsigned atname);
  pred_result result (value_die &a) override;
  static std::string docstring ();
};

class pred_atname_attr
  : public pred_overload <value_attr>
{
  unsigned m_atname;

public:
  pred_atname_attr (unsigned atname);
  pred_result result (value_attr &a) override;
  static std::string docstring ();
};

class pred_atname_cst
  : public pred_overload <value_cst>
{
  constant m_const;

public:
  pred_atname_cst (unsigned atname);
  pred_result result (value_cst &a) override;
  static std::string docstring ();
};

class pred_tag_die
  : public pred_overload <value_die>
{
  int m_tag;

public:
  pred_tag_die (int tag);
  pred_result result (value_die &a) override;
  static std::string docstring ();
};

class pred_tag_cst
  : public pred_overload <value_cst>
{
  constant m_const;

public:
  pred_tag_cst (int tag);
  pred_result result (value_cst &a) override;
  static std::string docstring ();
};

class pred_form_attr
  : public pred_overload <value_attr>
{
  unsigned m_form;

public:
  pred_form_attr (unsigned form);
  pred_result result (value_attr &a) override;
  static std::string docstring ();
};

class pred_form_cst
  : public pred_overload <value_cst>
{
  constant m_const;

public:
  pred_form_cst (unsigned form);
  pred_result result (value_cst &a) override;
  static std::string docstring ();
};

class pred_op_loclist_elem
  : public pred_overload <value_loclist_elem>
{
    unsigned m_op;

public:
  pred_op_loclist_elem (unsigned op);
  pred_result result (value_loclist_elem &a) override;
  static std::string docstring ();
};

class pred_op_loclist_op
  : public pred_overload <value_loclist_op>
{
    unsigned m_op;

public:
  pred_op_loclist_op (unsigned op);
  pred_result result (value_loclist_op &a) override;
  static std::string docstring ();
};

class pred_op_cst
  : public pred_overload <value_cst>
{
    constant m_const;

public:
  pred_op_cst (unsigned form);
  pred_result result (value_cst &a) override;
  static std::string docstring ();
};

#endif /* _BUILTIN_DW_H_ */
