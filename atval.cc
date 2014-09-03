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

#include <iostream>
#include <dwarf.h>
#include <memory>

#include "atval.hh"
#include "dwcst.hh"
#include "dwpp.hh"
#include "stack.hh"
#include "value-cst.hh"
#include "value-dw.hh"
#include "value-seq.hh"
#include "value-str.hh"

using namespace std::literals::string_literals;

namespace
{
  struct single_value
    : public value_producer
  {
    std::unique_ptr <value> m_value;

    single_value (std::unique_ptr <value> value)
      : m_value {std::move (value)}
    {
      assert (m_value != nullptr);
    }

    std::unique_ptr <value>
    next () override
    {
      return std::move (m_value);
    }
  };

  struct null_producer
    : public value_producer
  {
    std::unique_ptr <value>
    next () override
    {
      return nullptr;
    }
  };

  std::unique_ptr <value_producer>
  pass_single_value (std::unique_ptr <value> value)
  {
    return std::make_unique <single_value> (std::move (value));
  }

  std::unique_ptr <value_producer>
  pass_block (Dwarf_Block const &block)
  {
    value_seq::seq_t vv;
    for (Dwarf_Word i = 0; i < block.length; ++i)
      vv.push_back (std::make_unique <value_cst>
		    (constant { block.data[i], &hex_constant_dom }, 0));
    return pass_single_value
      (std::make_unique <value_seq> (std::move (vv), 0));
  }

  std::unique_ptr <value_producer>
  atval_unsigned_with_domain (Dwarf_Attribute attr, constant_dom const &dom)
  {
    Dwarf_Word uval;
    if (dwarf_formudata (&attr, &uval) != 0)
      throw_libdw ();
    return pass_single_value
      (std::make_unique <value_cst> (constant {uval, &dom}, 0));
  }

  std::unique_ptr <value_producer>
  atval_unsigned (Dwarf_Attribute attr)
  {
    return atval_unsigned_with_domain (attr, dec_constant_dom);
  }

  std::unique_ptr <value_producer>
  atval_signed (Dwarf_Attribute attr)
  {
    Dwarf_Sword sval;
    if (dwarf_formsdata (&attr, &sval) != 0)
      throw_libdw ();
    return pass_single_value
      (std::make_unique <value_cst> (constant {sval, &dec_constant_dom}, 0));
  }

  std::unique_ptr <value_producer>
  atval_addr (Dwarf_Attribute attr)
  {
    // XXX Eventually we might want to have a dedicated type that
    // carries along information necessary for later translation into
    // symbol-relative offsets (like what readelf does).  NIY
    Dwarf_Addr addr;
    if (dwarf_formaddr (&attr, &addr) != 0)
      throw_libdw ();
    return pass_single_value
      (std::make_unique <value_cst> (constant {addr, &dw_address_dom}, 0));
  }

  struct locexpr_producer
    : public value_producer
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Dwarf_Attribute m_attr;
    Dwarf_Addr m_base;
    ptrdiff_t m_offset;
    size_t m_i;

    locexpr_producer (std::shared_ptr <dwfl_context> dwctx,
		      Dwarf_Attribute attr)
      : m_dwctx {dwctx}
      , m_attr (attr)
      , m_offset {0}
      , m_i {0}
    {}

    std::unique_ptr <value>
    next () override
    {
      Dwarf_Addr start, end;
      Dwarf_Op *expr;
      size_t exprlen;

      switch (m_offset = dwarf_getlocations (&m_attr, m_offset, &m_base,
					     &start, &end, &expr, &exprlen))
	{
	case -1:
	  throw_libdw ();
	case 0:
	  return nullptr;
	default:
	  return std::make_unique <value_loclist_elem>
	    (m_dwctx, m_attr, start, end, expr, exprlen, m_i++);
	}
    }
  };

  std::unique_ptr <value_producer>
  atval_locexpr (std::shared_ptr <dwfl_context> dwctx, Dwarf_Attribute attr)
  {
    return std::make_unique <locexpr_producer> (dwctx, attr);
  }

  struct ranges_producer
    : public value_producer
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Dwarf_Die m_die;
    ptrdiff_t m_offset;
    Dwarf_Addr m_base;
    size_t m_i;

    explicit ranges_producer (std::shared_ptr <dwfl_context> dwctx,
			      Dwarf_Die die)
      : m_dwctx {dwctx}
      , m_die (die)
      , m_offset {0}
      , m_i {0}
    {}

    std::unique_ptr <value>
    next () override
    {
      Dwarf_Addr start, end;
      m_offset = dwarf_ranges (&m_die, m_offset, &m_base, &start, &end);
      if (m_offset < 0)
	throw_libdw ();
      if (m_offset == 0)
	return nullptr;

      return std::make_unique <value_addr_range>
	(constant {start, &dw_address_dom},
	 constant {end, &dw_address_dom}, m_i++);
    }
  };
}

std::unique_ptr <value_producer>
die_ranges (std::shared_ptr <dwfl_context> dwctx, Dwarf_Die die)
{
  return std::make_unique <ranges_producer> (dwctx, die);
}

namespace
{
  std::unique_ptr <value_producer>
  handle_at_dependent_value (Dwarf_Attribute attr, Dwarf_Die die,
			     std::shared_ptr <dwfl_context> dwctx)
  {
    switch (dwarf_whatattr (&attr))
      {
      case DW_AT_language:
	return atval_unsigned_with_domain (attr, dw_lang_dom);

      case DW_AT_inline:
	return atval_unsigned_with_domain (attr, dw_inline_dom);

      case DW_AT_encoding:
	return atval_unsigned_with_domain (attr, dw_encoding_dom);

      case DW_AT_accessibility:
	return atval_unsigned_with_domain (attr, dw_access_dom);

      case DW_AT_visibility:
	return atval_unsigned_with_domain (attr, dw_visibility_dom);

      case DW_AT_virtuality:
	return atval_unsigned_with_domain (attr, dw_virtuality_dom);

      case DW_AT_identifier_case:
	return atval_unsigned_with_domain (attr, dw_identifier_case_dom);

      case DW_AT_calling_convention:
	return atval_unsigned_with_domain (attr, dw_calling_convention_dom);

      case DW_AT_ordering:
	return atval_unsigned_with_domain (attr, dw_ordering_dom);

      case DW_AT_decimal_sign:
	return atval_unsigned_with_domain (attr, dw_decimal_sign_dom);

      case DW_AT_address_class:
	return atval_unsigned_with_domain (attr, dw_address_class_dom);

      case DW_AT_endianity:
	return atval_unsigned_with_domain (attr, dw_endianity_dom);

      case DW_AT_decl_line:
      case DW_AT_call_line:
	return atval_unsigned_with_domain (attr, line_number_dom);

      case DW_AT_decl_column:
      case DW_AT_call_column:
	return atval_unsigned_with_domain (attr, column_number_dom);

      case DW_AT_decl_file:
      case DW_AT_call_file:
	{
	  Dwarf_Die cudie;
	  if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
	    throw_libdw ();

	  Dwarf_Files *files;
	  size_t nfiles;
	  if (dwarf_getsrcfiles (&cudie, &files, &nfiles) != 0)
	    throw_libdw ();

	  Dwarf_Word uval;
	  if (dwarf_formudata (&attr, &uval) != 0)
	    throw_libdw ();

	  const char *fn = dwarf_filesrc (files, uval, nullptr, nullptr);
	  if (fn == nullptr)
	    throw_libdw ();

	  return pass_single_value (std::make_unique <value_str> (fn, 0));
	}

      case DW_AT_const_value:
	{
	  // This might be a DW_TAG_variable, DW_TAG_constant,
	  // DW_TAG_formal_parameter, DW_TAG_inlined_subroutine,
	  // DW_TAG_template_value_parameter or DW_TAG_enumerator.
	  // In the former cases, we need to determine signedness by
	  // inspecting the @type.  To handle the enumerator, we need
	  // to look at @type in the parent (DW_TAG_enumeration_type).

	  Dwarf_Die type_die;
	  if (dwarf_tag (&die) != DW_TAG_enumerator)
	    type_die = die;
	  else
	    {
	      assert (! "atval dwarf_offdie");
	      /*
	      // Get DW_TAG_enumeration_type.
	      if (dwarf_offdie (&*gr->dwarf, gr->find_parent (die),
				&type_die) == nullptr)
		throw_libdw ();
	      */

	      if (dwarf_tag (&type_die) != DW_TAG_enumeration_type)
		{
		  std::cerr << "Unexpected: DW_TAG_enumerator's parent is "
		    "not DW_TAG_enumeration_type\n";
		  return atval_unsigned (attr);
		}

	      if (! dwarf_hasattr_integrate (&type_die, DW_AT_type))
		{
		  std::cerr << "Unexpected: DW_TAG_enumeration_type whose "
		    "DW_TAG_enumerator's DW_AT_const_value is "
		    "DW_FORM_data[1248] doesn't have DW_AT_type.\n";
		  return atval_unsigned (attr);
		}
	    }

	  Dwarf_Attribute at;
	  while (dwarf_hasattr_integrate (&type_die, DW_AT_type))
	    {
	      if (dwarf_attr_integrate (&type_die, DW_AT_type,
					&at) == nullptr
		  || dwarf_formref_die (&at, &type_die) == nullptr)
		throw_libdw ();
	      int tag = dwarf_tag (&type_die);
	      if (tag != DW_TAG_const_type
		  && tag != DW_TAG_volatile_type
		  && tag != DW_TAG_restrict_type
		  && tag != DW_TAG_typedef
		  && tag != DW_TAG_subrange_type
		  && tag != DW_TAG_packed_type)
		break;
	    }

	  int tag = dwarf_tag (&type_die);
	  if (tag == DW_TAG_pointer_type)
	    return atval_unsigned_with_domain (attr, dw_address_dom);

	  if (tag != DW_TAG_enumeration_type
	      && (tag != DW_TAG_base_type
		  || ! dwarf_hasattr_integrate (&type_die, DW_AT_encoding)))
	    {
	      char const *name = dwarf_diename (&type_die);
	      if (name == nullptr)
		{
		  if (int e = dwarf_errno ())
		    throw_libdw (e);
		}
	      else
		if ("decltype(nullptr)"s == name)
		  return atval_unsigned_with_domain (attr, dw_address_dom);

	      // Ho hum.  This could be a structure or something
	      // similarly useless.  See if it's a block form at
	      // least.
	      break;
	    }
	  else
	    {
	      Dwarf_Word encoding;
	      if (dwarf_hasattr_integrate (&type_die, DW_AT_encoding))
		{
		  if (dwarf_attr_integrate (&type_die, DW_AT_encoding,
					    &at) == nullptr)
		    throw_libdw ();
		  if (dwarf_formudata (&at, &encoding) != 0)
		    throw_libdw ();
		}

	      else if (tag == DW_TAG_enumeration_type)
		{
		  if (dwarf_hasattr_integrate (&type_die, DW_AT_type))
		    // We can use this DW_AT_type to figure out
		    // whether this is signed or unsigned.
		    assert (! "unhandled: DW_AT_const_value on a DIE whose"
			    " DW_AT_type is a DW_TAG_enumeration_type with"
			    " DW_AT_type");

		  std::cerr << "DW_AT_const_value on a DIE whose DW_AT_type is "
		    "a DW_TAG_enumeration_type without DW_AT_encoding or "
		    "DW_AT_type.  Assuming signed.\n";
		  return atval_signed (attr);
		}

	      switch (encoding)
		{
		case DW_ATE_signed:
		case DW_ATE_signed_char:
		  return atval_signed (attr);

		case DW_ATE_unsigned:
		case DW_ATE_unsigned_char:
		case DW_ATE_address:
		case DW_ATE_boolean:
		  return atval_unsigned (attr);

		case DW_ATE_UTF:
		  // XXX We could decode the character that this
		  // represents.
		  return atval_unsigned (attr);

		case DW_ATE_float:
		case DW_ATE_imaginary_float:
		case DW_ATE_complex_float:
		  // Break out so that it's passed as a block, if it's
		  // a block.
		  break;

		case DW_ATE_signed_fixed:
		case DW_ATE_unsigned_fixed:
		case DW_ATE_packed_decimal:
		case DW_ATE_decimal_float:
		  // OK, gross.
		  assert (! "weird-float enumerator unhandled");
		  abort ();

		default:
		  // There's a couple more that nobody should
		  // probably put inside DW_FORM_data*.
		  assert (! "unknown enumerator encoding");
		  abort ();
		}
	    }
	  break;
	}

      case DW_AT_byte_stride:
      case DW_AT_bit_stride:
	// """Note that the stride can be negative."""
      case DW_AT_binary_scale:
      case DW_AT_decimal_scale:
	return atval_signed (attr);

      case DW_AT_high_pc:
      case DW_AT_entry_pc:
      case DW_AT_byte_size:
      case DW_AT_bit_size:
      case DW_AT_bit_offset:
      case DW_AT_data_bit_offset:
      case DW_AT_lower_bound:
      case DW_AT_upper_bound:
      case DW_AT_count:
      case DW_AT_allocated:
      case DW_AT_associated:
      case DW_AT_start_scope:
      case DW_AT_digit_count:
      case DW_AT_GNU_odr_signature:
	return atval_unsigned (attr);

      case DW_AT_stmt_list:
	return atval_unsigned_with_domain (attr, hex_constant_dom);

      case DW_AT_location:
      case DW_AT_data_member_location:
      case DW_AT_vtable_elem_location:
	return atval_locexpr (dwctx, attr);

      case DW_AT_ranges:
	return die_ranges (dwctx, die);

      case DW_AT_GNU_macros:
      case DW_AT_macro_info:
	std::cerr << "macros NIY\n";
	return atval_unsigned_with_domain (attr, hex_constant_dom);

      case DW_AT_discr_value:
	// ^^^ """The number is signed if the tag type for the
	// variant part containing this variant is a signed
	// type."""
	assert (! "signedness of attribute not implemented yet");
	abort ();
      }

    switch (dwarf_whatform (&attr))
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
      {
	// XXX even for blocks, we need to look at the @type to
	// figure out whether there's a better way to represent this
	// item.  Some of these might be e.g. floats.  NIY.
	Dwarf_Block block;
	if (dwarf_formblock (&attr, &block) != 0)
	  throw_libdw ();

	return pass_block (block);
      }

    std::cerr << dwarf_whatattr (&attr) << std::endl << std::flush;
    assert (! "signedness of attribute unhandled");
    abort ();
  }
}

std::unique_ptr <value_producer>
at_value (std::shared_ptr <dwfl_context> dwctx,
	  Dwarf_Die die, Dwarf_Attribute attr)
{
  switch (dwarf_whatform (&attr))
    {
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_GNU_strp_alt:
      {
	const char *str = dwarf_formstring (&attr);
	if (str == nullptr)
	  throw_libdw ();
	return pass_single_value (std::make_unique <value_str> (str, 0));
      }

    case DW_FORM_ref_addr:
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref8:
    case DW_FORM_ref_udata:
      {
	Dwarf_Die die;
	if (dwarf_formref_die (&attr, &die) == nullptr)
	  throw_libdw ();
	return pass_single_value (std::make_unique <value_die> (dwctx, die, 0));
      }

    case DW_FORM_sdata:
      return atval_signed (attr);

    case DW_FORM_udata:
      return atval_unsigned (attr);

    case DW_FORM_addr:
      return atval_addr (attr);

    case DW_FORM_flag:
    case DW_FORM_flag_present:
      {
	bool flag;
	if (dwarf_formflag (&attr, &flag) != 0)
	  throw_libdw ();
	return pass_single_value
	  (std::make_unique <value_cst>
	   (constant {static_cast <unsigned> (flag), &bool_constant_dom}, 0));
      }

    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_sec_offset:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
      return handle_at_dependent_value (attr, die, dwctx);

    case DW_FORM_exprloc:
      return atval_locexpr (dwctx, attr);

    case DW_FORM_ref_sig8:
    case DW_FORM_GNU_ref_alt:
      std::cerr << "Form unhandled: "
		<< constant (dwarf_whatform (&attr), &dw_form_dom)
		<< std::endl;
      return pass_single_value
	(std::make_unique <value_str> ("(form unhandled)", 0));

    case DW_FORM_indirect:
      assert (! "Unexpected DW_FORM_indirect");
      abort ();
    }

  assert (! "Unhandled DWARF form type.");
  abort ();
}

namespace
{
  template <unsigned N>
  std::unique_ptr <value_producer>
  select (std::unique_ptr <value_producer> a,
	  std::unique_ptr <value_producer> b);

  template <>
  std::unique_ptr <value_producer>
  select<0> (std::unique_ptr <value_producer> a,
	     std::unique_ptr <value_producer> b)
  {
    return a;
  }

  template <>
  std::unique_ptr <value_producer>
  select<1> (std::unique_ptr <value_producer> a,
	     std::unique_ptr <value_producer> b)
  {
    return b;
  }

  // Return up to two constants.  One or both may be default constants
  // (sans domain).  If the former is default, the latter shall be as
  // well.  Both default represent nullary operands, one non-default
  // represents unary, both non-default represent binary op.
  template <unsigned N>
  std::unique_ptr <value_producer>
  locexpr_op_values (std::shared_ptr <dwfl_context> dwctx,
		     Dwarf_Attribute const &at, Dwarf_Op const *op)
  {
    auto signed_cst = [] (Dwarf_Word w, constant_dom const *dom)
      {
	return constant {(Dwarf_Sword) w, dom};
      };

    auto single_constant = [] (constant const &cst)
      {
	return select <N> (pass_single_value
				(std::make_unique <value_cst> (cst, 0)),
			   std::make_unique <null_producer> ());
      };

    auto two_constants = [] (constant const &a, constant const &b)
      {
	return select <N> (pass_single_value
				(std::make_unique <value_cst> (a, 0)),
			   pass_single_value
				(std::make_unique <value_cst> (b, 0)));
      };

    switch (op->atom)
      {
      default:
	return std::make_unique <null_producer> ();

      case DW_OP_addr:
      case DW_OP_call_ref:	// XXX yield a DIE?
	return single_constant ({op->number, &hex_constant_dom});

      case DW_OP_deref_size:
      case DW_OP_xderef_size:
      case DW_OP_pick:
      case DW_OP_const1u:
      case DW_OP_const2u:
      case DW_OP_const4u:
      case DW_OP_const8u:
      case DW_OP_piece:
      case DW_OP_regx:
      case DW_OP_plus_uconst:
      case DW_OP_constu:
      case DW_OP_call2:
      case DW_OP_call4:
      case DW_OP_GNU_convert:		// XXX CU-relative offset to DIE
      case DW_OP_GNU_reinterpret:	// XXX CU-relative offset to DIE
      case DW_OP_GNU_parameter_ref:	// XXX CU-relative offset to DIE
	return single_constant ({op->number, &dec_constant_dom});

      case DW_OP_const1s:
      case DW_OP_const2s:
      case DW_OP_const4s:
      case DW_OP_const8s:
      case DW_OP_fbreg:
      case DW_OP_breg0 ... DW_OP_breg31:
      case DW_OP_consts:
      case DW_OP_skip:	// XXX readelf translates to absolute address
      case DW_OP_bra:	// XXX this one as well
	return single_constant (signed_cst (op->number, &dec_constant_dom));

      case DW_OP_bit_piece:
      case DW_OP_GNU_regval_type:
      case DW_OP_GNU_deref_type:
	return two_constants ({op->number, &dec_constant_dom},
			      {op->number2, &dec_constant_dom});

      case DW_OP_bregx:
	return two_constants ({op->number, &dec_constant_dom},
			      signed_cst (op->number2, &dec_constant_dom));

      case DW_OP_GNU_implicit_pointer:
	{
	  Dwarf_Die die;
	  if (dwarf_getlocation_die
	      (const_cast <Dwarf_Attribute *> (&at), op, &die) != 0)
	    throw_libdw ();

	  return select <N>
	    (pass_single_value (std::make_unique <value_die> (dwctx, die, 0)),
	     pass_single_value (std::make_unique <value_cst>
				(signed_cst (op->number2,
					     &dec_constant_dom), 0)));
	}

      case DW_OP_implicit_value:
	{
	  Dwarf_Block block;
	  if (dwarf_getlocation_implicit_value
	      (const_cast <Dwarf_Attribute *> (&at), op, &block) != 0)
	    throw_libdw ();

	  return select <N> (pass_block (block),
			     std::make_unique <null_producer> ());
	}

      case DW_OP_GNU_entry_value:
	{
	  Dwarf_Attribute result;
	  if (dwarf_getlocation_attr
	      (const_cast <Dwarf_Attribute *> (&at), op, &result) != 0)
	    throw_libdw ();

	  return select <N> (atval_locexpr (dwctx, result),
			     std::make_unique <null_producer> ());
	}

      case DW_OP_GNU_const_type:
	{
	  Dwarf_Attribute *attr = const_cast <Dwarf_Attribute *> (&at);
	  Dwarf_Die die;
	  if (dwarf_getlocation_die (attr, op, &die) != 0)
	    throw_libdw ();

	  Dwarf_Attribute attr2;
	  if (dwarf_getlocation_attr (attr, op, &attr2) != 0)
	    throw_libdw ();

	  Dwarf_Block block;
	  if (dwarf_formblock (&attr2, &block) != 0)
	    throw_libdw ();

	  return select <N>
	    (pass_single_value (std::make_unique <value_die> (dwctx, die, 0)),
	     pass_block (block));
	}
      }
  }
}

std::unique_ptr <value_producer>
dwop_number (std::shared_ptr <dwfl_context> dwctx,
	     Dwarf_Attribute const &attr, Dwarf_Op const *op)
{
  return locexpr_op_values <0> (dwctx, attr, op);
}

std::unique_ptr <value_producer>
dwop_number2 (std::shared_ptr <dwfl_context> dwctx,
	     Dwarf_Attribute const &attr, Dwarf_Op const *op)
{
  return locexpr_op_values <1> (dwctx, attr, op);
}
