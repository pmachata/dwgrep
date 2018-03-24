/*
   Copyright (C) 2017, 2018 Petr Machata
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

#include <dwarf.h>
#include "builtin-aset.hh"
#include "builtin-dw.hh"
#include "builtin-dw-abbrev.hh"
#include "builtin-symbol.hh"
#include "builtin-elf.hh"
#include "dwcst.hh"
#include "elfcst.hh"
#include "known-dwarf.h"
#include "known-dwarf-macro-gnu.h"
#include "known-elf.h"

std::unique_ptr <vocabulary>
dwgrep_vocabulary_dw ()
{
  auto ret = std::make_unique <vocabulary> ();
  vocabulary &voc = *ret;

  add_builtin_type_constant <value_dwarf> (voc);
  add_builtin_type_constant <value_cu> (voc);
  add_builtin_type_constant <value_die> (voc);
  add_builtin_type_constant <value_attr> (voc);
  add_builtin_type_constant <value_abbrev_unit> (voc);
  add_builtin_type_constant <value_abbrev> (voc);
  add_builtin_type_constant <value_abbrev_attr> (voc);
  add_builtin_type_constant <value_aset> (voc);
  add_builtin_type_constant <value_loclist_elem> (voc);
  add_builtin_type_constant <value_loclist_op> (voc);
  add_builtin_type_constant <value_symbol> (voc);
  add_builtin_type_constant <value_elf> (voc);

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_dwopen_str> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("dwopen", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_unit_dwarf> ();
    t->add_op_overload <op_unit_die> ();
    t->add_op_overload <op_unit_attr> ();
    // xxx rawattr

    voc.add (std::make_shared <overloaded_op_builtin> ("unit", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_entry_dwarf> ();
    t->add_op_overload <op_entry_cu> ();
    t->add_op_overload <op_entry_abbrev_unit> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("entry", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_attribute_die> ();
    t->add_op_overload <op_attribute_abbrev> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("attribute", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_rootp_die> ();

    voc.add (std::make_shared <overloaded_pred_builtin> ("?root", t, true));
    voc.add (std::make_shared <overloaded_pred_builtin> ("!root", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_root_cu> ();
    t->add_op_overload <op_root_die> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("root", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_child_die> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("child", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elem_loclist_elem> ();
    t->add_op_overload <op_elem_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("elem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_relem_loclist_elem> ();
    t->add_op_overload <op_relem_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("relem", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_value_attr> ();
    // xxx raw
    t->add_op_overload <op_value_loclist_op> ();
    t->add_op_overload <op_address_symbol> ();   // [sic]

    voc.add (std::make_shared <overloaded_op_builtin> ("value", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_offset_cu> ();
    t->add_op_overload <op_offset_die> ();
    t->add_op_overload <op_offset_abbrev_unit> ();
    t->add_op_overload <op_offset_abbrev> ();
    t->add_op_overload <op_offset_abbrev_attr> ();
    t->add_op_overload <op_offset_loclist_op> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("offset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_address_die> ();
    t->add_op_overload <op_address_attr> ();
    t->add_op_overload <op_address_loclist_elem> ();
    t->add_op_overload <op_address_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("address", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_label_die> ();
    t->add_op_overload <op_label_attr> ();
    t->add_op_overload <op_label_abbrev> ();
    t->add_op_overload <op_label_abbrev_attr> ();
    t->add_op_overload <op_label_loclist_op> ();
    t->add_op_overload <op_label_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("label", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_form_attr> ();
    t->add_op_overload <op_form_abbrev_attr> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("form", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_parent_die> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("parent", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_low_die> ();
    t->add_op_overload <op_low_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("low", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_high_die> ();
    t->add_op_overload <op_high_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("high", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_aset_cst_cst> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("aset", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_add_aset_cst> ();
    t->add_op_overload <op_add_aset_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("add", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_sub_aset_cst> ();
    t->add_op_overload <op_sub_aset_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("sub", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_length_aset> ();
    t->add_op_overload <op_length_loclist_elem> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("length", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_range_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("range", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_containsp_aset_cst> ();
    t->add_pred_overload <pred_containsp_aset_aset> ();

    voc.add
      (std::make_shared <overloaded_pred_builtin> ("?contains", t, true));
    voc.add
      (std::make_shared <overloaded_pred_builtin> ("!contains", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_overlapsp_aset_aset> ();

    voc.add
      (std::make_shared <overloaded_pred_builtin> ("?overlaps", t, true));
    voc.add
      (std::make_shared <overloaded_pred_builtin> ("!overlaps", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_overlap_aset_aset> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("overlap", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_emptyp_aset> ();

    voc.add (std::make_shared <overloaded_pred_builtin> ("?empty", t, true));
    voc.add (std::make_shared <overloaded_pred_builtin> ("!empty", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_abbrev_dwarf> ();
    t->add_op_overload <op_abbrev_cu> ();
    t->add_op_overload <op_abbrev_die> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("abbrev", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_pred_overload <pred_haschildrenp_die> ();
    t->add_pred_overload <pred_haschildrenp_abbrev> ();

    voc.add (std::make_shared
	     <overloaded_pred_builtin> ("?haschildren", t, true));
    voc.add (std::make_shared
	     <overloaded_pred_builtin> ("!haschildren", t, false));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_code_abbrev> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("code", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_version_cu> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("version", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_name_dwarf> ();
    t->add_op_overload <op_name_elf> ();
    t->add_op_overload <op_name_die> ();
    t->add_op_overload <op_name_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("name", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_raw_dwarf> ();
    t->add_op_overload <op_raw_cu> ();
    t->add_op_overload <op_raw_die> ();
    t->add_op_overload <op_raw_attr> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("raw", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_cooked_dwarf> ();
    t->add_op_overload <op_cooked_cu> ();
    t->add_op_overload <op_cooked_die> ();
    t->add_op_overload <op_cooked_attr> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("cooked", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_symbol_dwarf> ();
    t->add_op_overload <op_symbol_elf> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("symbol", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_binding_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("binding", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_visibility_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("visibility", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_size_symbol> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("size", t));
  }

  auto add_dw_at = [&voc] (unsigned code,
			   char const *qname, char const *bname,
			   char const *atname,
			   char const *lqname, char const *lbname,
			   char const *latname)
    {
      // ?AT_* etc.
      {
	auto t = std::make_shared <overload_tab> ();

	t->add_pred_overload <pred_atname_die> (code);
	t->add_pred_overload <pred_atname_attr> (code);
	t->add_pred_overload <pred_atname_abbrev> (code);
	t->add_pred_overload <pred_atname_abbrev_attr> (code);
	t->add_pred_overload <pred_atname_cst> (code);

	voc.add (std::make_shared <overloaded_pred_builtin> (qname, t, true));
	voc.add (std::make_shared <overloaded_pred_builtin> (bname, t, false));
	voc.add (std::make_shared <overloaded_pred_builtin> (lqname, t, true));
	voc.add (std::make_shared <overloaded_pred_builtin> (lbname, t, false));
      }

      // @AT_* etc.
      {
	auto t = std::make_shared <overload_tab> ();

	t->add_op_overload <op_atval_die> (code);
	// xxx raw shouldn't interpret values

	voc.add (std::make_shared <overloaded_op_builtin> (atname, t));
	voc.add (std::make_shared <overloaded_op_builtin> (latname, t));
      }

      // DW_AT_*
      add_builtin_constant (voc, constant (code, &dw_attr_dom ()), lqname + 1);
    };

#define DWARF_ONE_KNOWN_DW_AT(NAME, CODE)					\
  add_dw_at (CODE, "?AT_" #NAME, "!AT_" #NAME, "@AT_" #NAME,		\
	     "?" #CODE, "!" #CODE, "@" #CODE);
  DWARF_ALL_KNOWN_DW_AT;
#undef DWARF_ONE_KNOWN_DW_AT

  auto add_dw_tag = [&voc] (int code,
			    char const *qname, char const *bname,
			    char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_tag_die> (code);
      t->add_pred_overload <pred_tag_abbrev> (code);
      t->add_pred_overload <pred_tag_cst> (code);

      voc.add (std::make_shared <overloaded_pred_builtin> (qname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (bname, t, false));
      voc.add (std::make_shared <overloaded_pred_builtin> (lqname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (lbname, t, false));

      add_builtin_constant (voc, constant (code, &dw_tag_dom ()), lqname + 1);
    };

#define DWARF_ONE_KNOWN_DW_TAG(NAME, CODE)				\
  add_dw_tag (CODE, "?TAG_" #NAME, "!TAG_" #NAME, "?" #CODE, "!" #CODE);
  DWARF_ALL_KNOWN_DW_TAG;
#undef DWARF_ONE_KNOWN_DW_TAG

  auto add_dw_form = [&voc] (unsigned code,
			     char const *qname, char const *bname,
			     char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_form_attr> (code);
      t->add_pred_overload <pred_form_abbrev_attr> (code);
      t->add_pred_overload <pred_form_cst> (code);

      voc.add (std::make_shared <overloaded_pred_builtin> (qname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (bname, t, false));
      voc.add (std::make_shared <overloaded_pred_builtin> (lqname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (lbname, t, false));

      add_builtin_constant (voc, constant (code, &dw_form_dom ()), lqname + 1);
    };

#define DWARF_ONE_KNOWN_DW_FORM(NAME, CODE)				\
  add_dw_form (CODE, "?FORM_" #NAME, "!FORM_" #NAME, "?" #CODE, "!" #CODE);
  DWARF_ALL_KNOWN_DW_FORM;
#undef DWARF_ONE_KNOWN_DW_FORM

  auto add_dw_op = [&voc] (unsigned code,
			   char const *qname, char const *bname,
			   char const *lqname, char const *lbname)
    {
      auto t = std::make_shared <overload_tab> ();

      t->add_pred_overload <pred_op_loclist_elem> (code);
      t->add_pred_overload <pred_op_loclist_op> (code);
      t->add_pred_overload <pred_op_cst> (code);

      voc.add (std::make_shared <overloaded_pred_builtin> (qname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (bname, t, false));
      voc.add (std::make_shared <overloaded_pred_builtin> (lqname, t, true));
      voc.add (std::make_shared <overloaded_pred_builtin> (lbname, t, false));

      add_builtin_constant (voc, constant (code, &dw_locexpr_opcode_dom ()),
			    lqname + 1);
    };

#define DWARF_ONE_KNOWN_DW_OP(NAME, CODE)				\
  add_dw_op (CODE, "?OP_" #NAME, "!OP_" #NAME, "?" #CODE, "!" #CODE);
  DWARF_ALL_KNOWN_DW_OP;
#undef DWARF_ONE_KNOWN_DW_OP

#define DWARF_ONE_KNOWN_DW_LANG(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_lang_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_LANG;
#undef DWARF_ONE_KNOWN_DW_LANG

#define DWARF_ONE_KNOWN_DW_MACINFO(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_macinfo_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_MACINFO;
#undef DWARF_ONE_KNOWN_DW_MACINFO

#define DWARF_ONE_KNOWN_DW_MACRO_GNU(NAME, CODE)			\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_macro_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_MACRO_GNU;
#undef DWARF_ONE_KNOWN_DW_MACRO_GNU

#define DWARF_ONE_KNOWN_DW_INL(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_inline_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_INL;
#undef DWARF_ONE_KNOWN_DW_INL

#define DWARF_ONE_KNOWN_DW_ATE(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_encoding_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_ATE;
#undef DWARF_ONE_KNOWN_DW_ATE

#define DWARF_ONE_KNOWN_DW_ACCESS(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_access_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_ACCESS;
#undef DWARF_ONE_KNOWN_DW_ACCESS

#define DWARF_ONE_KNOWN_DW_VIS(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_visibility_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_VIS;
#undef DWARF_ONE_KNOWN_DW_VIS

#define DWARF_ONE_KNOWN_DW_VIRTUALITY(NAME, CODE)			\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_virtuality_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_VIRTUALITY;
#undef DWARF_ONE_KNOWN_DW_VIRTUALITY

#define DWARF_ONE_KNOWN_DW_ID(NAME, CODE)				\
  {									\
    add_builtin_constant (voc,						\
			  constant (CODE, &dw_identifier_case_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_ID;
#undef DWARF_ONE_KNOWN_DW_ID

#define DWARF_ONE_KNOWN_DW_CC(NAME, CODE)				\
  {									\
    add_builtin_constant (voc,						\
			  constant (CODE, &dw_calling_convention_dom ()), \
			  #CODE);					\
  }
  DWARF_ALL_KNOWN_DW_CC;
#undef DWARF_ONE_KNOWN_DW_CC

#define DWARF_ONE_KNOWN_DW_ORD(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_ordering_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_ORD;
#undef DWARF_ONE_KNOWN_DW_ORD

#define DWARF_ONE_KNOWN_DW_DSC(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_discr_list_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_DSC;
#undef DWARF_ONE_KNOWN_DW_DSC

#define DWARF_ONE_KNOWN_DW_DS(NAME, CODE)				\
  {									\
    add_builtin_constant (voc,						\
			  constant (CODE, &dw_decimal_sign_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_DS;
#undef DWARF_ONE_KNOWN_DW_DS

  add_builtin_constant (voc, constant (DW_ADDR_none, &dw_address_class_dom ()),
			"DW_ADDR_none");

#define DWARF_ONE_KNOWN_DW_END(NAME, CODE)				\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_endianity_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_END;
#undef DWARF_ONE_KNOWN_DW_END

#define DWARF_ONE_KNOWN_DW_DEFAULTED(NAME, CODE)			\
  {									\
    add_builtin_constant (voc, constant (CODE, &dw_defaulted_dom ()), #CODE); \
  }
  DWARF_ALL_KNOWN_DW_DEFAULTED;
#undef DWARF_ONE_KNOWN_DW_DEFAULTED


#define ELF_ONE_KNOWN_STT(NAME, CODE)					\
  add_builtin_constant (voc,						\
			constant (CODE, &elfsym_stt_dom (machine)), #CODE);

  {
    constexpr int machine = EM_NONE;
    ELF_ALL_KNOWN_STT
  }

#define ELF_ONE_KNOWN_STT_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_STT_##ARCH			\
    }
  ELF_ALL_KNOWN_STT_ARCHES

#undef ELF_ONE_KNOWN_STT_ARCH
#undef ELF_ONE_KNOWN_STT


#define ELF_ONE_KNOWN_STB(NAME, CODE)					\
  add_builtin_constant (voc,						\
			constant (CODE, &elfsym_stb_dom (machine)), #CODE);

  {
    constexpr int machine = EM_NONE;
    ELF_ALL_KNOWN_STB
  }

#define ELF_ONE_KNOWN_STB_ARCH(ARCH)		\
    {						\
      constexpr int machine = EM_##ARCH;	\
      ELF_ALL_KNOWN_STB_##ARCH			\
    }
  ELF_ALL_KNOWN_STB_ARCHES

#undef ELF_ONE_KNOWN_STB_ARCH
#undef ELF_ONE_KNOWN_STB


#define ELF_ONE_KNOWN_STV(NAME, CODE)					\
  add_builtin_constant (voc, constant (CODE, &elfsym_stv_dom ()), #CODE);

    ELF_ALL_KNOWN_STV

#undef ELF_ONE_KNOWN_STV

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_elf_dwarf> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("elf", t));
  }

  {
    auto t = std::make_shared <overload_tab> ();

    t->add_op_overload <op_atclass_elf> ();

    voc.add (std::make_shared <overloaded_op_builtin> ("@class", t));
  }

#define ELF_ONE_KNOWN_ELFCLASS(CODE)					\
  add_builtin_constant (voc, constant (CODE, &elf_class_dom ()), #CODE);

  ELF_ONE_KNOWN_ELFCLASS(ELFCLASSNONE)
  ELF_ONE_KNOWN_ELFCLASS(ELFCLASS32)
  ELF_ONE_KNOWN_ELFCLASS(ELFCLASS64)

#undef ELF_ONE_KNOWN_ELFCLASS

  return ret;
}
