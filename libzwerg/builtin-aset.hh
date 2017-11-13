/*
   Copyright (C) 2017 Petr Machata
   Copyright (C) 2015 Red Hat, Inc.
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

#ifndef BUILTIN_ASET_HH
#define BUILTIN_ASET_HH

#include "builtin.hh"
#include "value-aset.hh"
#include "value-cst.hh"

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

struct op_low_aset
  : public op_overload <value_cst, value_aset>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_aset> a) const override;
  static std::string docstring ();
};

struct op_high_aset
  : public op_overload <value_cst, value_aset>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_aset> a) const override;
  static std::string docstring ();
};

struct op_aset_cst_cst
  : public op_once_overload <value_aset, value_cst, value_cst>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_cst> a,
		      std::unique_ptr <value_cst> b) const override;

  static std::string docstring ();
};

struct op_add_aset_cst
  : public op_once_overload <value_aset, value_aset, value_cst>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_aset> a,
		      std::unique_ptr <value_cst> b) const override;

  static std::string docstring ();
};

struct op_add_aset_aset
  : public op_once_overload <value_aset, value_aset, value_aset>
{
  using op_once_overload::op_once_overload;

  value_aset
  operate (std::unique_ptr <value_aset> a,
	   std::unique_ptr <value_aset> b) const override;

  static std::string docstring ();
};

struct op_sub_aset_cst
  : public op_once_overload <value_aset, value_aset, value_cst>
{
  using op_once_overload::op_once_overload;

  value_aset
  operate (std::unique_ptr <value_aset> a,
	   std::unique_ptr <value_cst> b) const override;

  static std::string docstring ();
};

struct op_sub_aset_aset
  : public op_once_overload <value_aset, value_aset, value_aset>
{
  using op_once_overload::op_once_overload;

  value_aset operate (std::unique_ptr <value_aset> a,
		      std::unique_ptr <value_aset> b) const override;

  static std::string docstring ();
};

struct op_length_aset
  : public op_once_overload <value_cst, value_aset>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_aset> a) const override;
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
		      std::unique_ptr <value_aset> b) const override;

  static std::string docstring ();
};

struct pred_emptyp_aset
  : public pred_overload <value_aset>
{
  using pred_overload::pred_overload;

  pred_result result (value_aset &a) override;
  static std::string docstring ();
};

#endif /* BUILTIN_ASET_HH */
