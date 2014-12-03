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

#ifndef BUILTIN_DW_ABBREV_H
#define BUILTIN_DW_ABBREV_H

#include "overload.hh"
#include "value-dw.hh"
#include "value-cst.hh"

struct op_entry_abbrev_unit
  : public op_yielding_overload <value_abbrev, value_abbrev_unit>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_abbrev>>
  operate (std::unique_ptr <value_abbrev_unit> a) override;

  static std::string docstring ();
};

struct op_attribute_abbrev
  : public op_yielding_overload <value_abbrev_attr, value_abbrev>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_abbrev_attr>>
  operate (std::unique_ptr <value_abbrev> a) override;

  static std::string docstring ();
};

struct op_offset_abbrev_unit
  : public op_once_overload <value_cst, value_abbrev_unit>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_abbrev_unit> a) override;

  static std::string docstring ();
};

struct op_offset_abbrev
  : public op_once_overload <value_cst, value_abbrev>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_abbrev> a) override;
  static std::string docstring ();
};

struct op_offset_abbrev_attr
  : public op_once_overload <value_cst, value_abbrev_attr>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_abbrev_attr> a) override;
  static std::string docstring ();
};

struct op_label_abbrev
  : public op_once_overload <value_cst, value_abbrev>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_abbrev> a) override;
  static std::string docstring ();
};

struct op_label_abbrev_attr
  : public op_once_overload <value_cst, value_abbrev_attr>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_abbrev_attr> a) override;
  static std::string docstring ();
};

struct op_form_abbrev_attr
  : public op_once_overload <value_cst, value_abbrev_attr>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_abbrev_attr> a) override;
  static std::string docstring ();
};

struct pred_haschildrenp_abbrev
  : public pred_overload <value_abbrev>
{
  using pred_overload::pred_overload;

  pred_result result (value_abbrev &a) override;
  static std::string docstring ();
};

struct op_abbrev_dwarf
  : public op_yielding_overload <value_abbrev_unit, value_dwarf>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_abbrev_unit>>
  operate (std::unique_ptr <value_dwarf> a) override;

  static std::string docstring ();
};

struct op_abbrev_cu
  : public op_once_overload <value_abbrev_unit, value_cu>
{
  using op_once_overload::op_once_overload;

  value_abbrev_unit operate (std::unique_ptr <value_cu> a) override;
  static std::string docstring ();
};

struct op_abbrev_die
  : public op_once_overload <value_abbrev, value_die>
{
  using op_once_overload::op_once_overload;

  value_abbrev operate (std::unique_ptr <value_die> a) override;
  static std::string docstring ();
};

struct op_code_abbrev
  : public op_once_overload <value_cst, value_abbrev>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_abbrev> a) override;
  static std::string docstring ();
};

struct pred_atname_abbrev
  : public pred_overload <value_abbrev>
{
  unsigned m_atname;

  pred_atname_abbrev (unsigned atname)
    : m_atname {atname}
  {}

  pred_result result (value_abbrev &a) override;
  static std::string docstring ();
};

struct pred_atname_abbrev_attr
  : public pred_overload <value_abbrev_attr>
{
  unsigned m_atname;

  pred_atname_abbrev_attr (unsigned atname)
    : m_atname {atname}
  {}

  pred_result result (value_abbrev_attr &a) override;
  static std::string docstring ();
};

struct pred_tag_abbrev
  : public pred_overload <value_abbrev>
{
  unsigned int m_tag;

  pred_tag_abbrev (unsigned int tag)
    : m_tag {tag}
  {}

  pred_result result (value_abbrev &a) override;
  static std::string docstring ();
};

struct pred_form_abbrev_attr
  : public pred_overload <value_abbrev_attr>
{
  unsigned m_form;

  pred_form_abbrev_attr (unsigned form)
    : m_form {form}
  {}

  pred_result result (value_abbrev_attr &a) override;
  static std::string docstring ();
};

#endif /* BUILTIN_DW_ABBREV_H */
