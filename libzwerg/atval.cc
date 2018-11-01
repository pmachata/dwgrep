/*
   Copyright (C) 2018 Petr Machata
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
#include "flag_saver.hh"
#include "dwit.hh"

namespace
{
  struct single_value
    : public value_producer <value>
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
    : public value_producer <value>
  {
    std::unique_ptr <value>
    next () override
    {
      return nullptr;
    }
  };

  std::unique_ptr <value_producer <value>>
  pass_single_value (std::unique_ptr <value> value)
  {
    return std::make_unique <single_value> (std::move (value));
  }

  std::unique_ptr <value_producer <value>>
  pass_block (Dwarf_Block const &block)
  {
    value_seq::seq_t vv;
    for (Dwarf_Word i = 0; i < block.length; ++i)
      vv.push_back (std::make_unique <value_cst>
		    (constant { block.data[i], &hex_constant_dom }, 0));
    return pass_single_value
      (std::make_unique <value_seq> (std::move (vv), 0));
  }

  value_cst
  extract_unsigned_with_domain (Dwarf_Attribute attr, constant_dom const &dom)
  {
    Dwarf_Word uval;
    if (dwarf_formudata (&attr, &uval) != 0)
      throw_libdw ();
    return value_cst (constant {uval, &dom}, 0);
  }

  value_cst
  extract_unsigned (Dwarf_Attribute attr)
  {
    return extract_unsigned_with_domain (attr, dec_constant_dom);
  }

  std::unique_ptr <value_producer <value>>
  atval_unsigned_with_domain (Dwarf_Attribute attr, constant_dom const &dom)
  {
    return pass_single_value
      (std::make_unique <value_cst> (extract_unsigned_with_domain (attr, dom)));
  }

  std::unique_ptr <value_producer <value>>
  atval_unsigned (Dwarf_Attribute attr)
  {
    return atval_unsigned_with_domain (attr, dec_constant_dom);
  }

  int
  fix_dwarf_formsdata (Dwarf_Attribute *attr, Dwarf_Sword *sval)
  {
    // In elfutils 0.170 and sooner, dwarf_formsdata is broken and returns an
    // unsigned value for fixed-width forms (DW_FORM_data8 is correct, because
    // Dwarf_Sword is 8-byte). Fix the signedness by hand.
    Dwarf_Sword value;
    int err = dwarf_formsdata (attr, &value);
    if (err != 0)
      return err;

    switch (attr->form)
      {
      case DW_FORM_data1:
	*sval = (int8_t) (uint8_t) value;
	break;
      case DW_FORM_data2:
	*sval = (int16_t) (uint16_t) value;
	break;
      case DW_FORM_data4:
	*sval = (int32_t) (uint32_t) value;
	break;
      default:
	*sval = value;
	break;
      }
    return 0;
  }

  std::unique_ptr <value_producer <value>>
  atval_signed (Dwarf_Attribute attr)
  {
    Dwarf_Sword sval;
    if (fix_dwarf_formsdata (&attr, &sval) != 0)
      throw_libdw ();
    return pass_single_value
      (std::make_unique <value_cst> (constant {sval, &dec_constant_dom}, 0));
  }

  std::unique_ptr <value_producer <value>>
  atval_addr (Dwarf_Attribute attr)
  {
    // XXX Eventually we might want to have a dedicated type that
    // carries along information necessary for later translation into
    // symbol-relative offsets (like what readelf does).  NIY
    Dwarf_Addr addr;
    if (dwarf_formaddr (&attr, &addr) != 0)
      throw_libdw ();
    return pass_single_value
      (std::make_unique <value_cst> (constant {addr, &dw_address_dom ()}, 0));
  }

  std::unique_ptr <value_producer <value>>
  atval_block (Dwarf_Attribute attr)
  {
    // XXX even for blocks, we need to look at the @type to
    // figure out whether there's a better way to represent this
    // item.  Some of these might be e.g. floats.  NIY.
    Dwarf_Block block;
    if (dwarf_formblock (&attr, &block) != 0)
      throw_libdw ();

    return pass_block (block);
  }

  std::unique_ptr <value_producer <value>>
  atval_string (Dwarf_Attribute attr)
  {
    const char *str = dwarf_formstring (&attr);
    if (str == nullptr)
      throw_libdw ();
    return pass_single_value (std::make_unique <value_str> (str, 0));
  }

  std::unique_ptr <value_producer <value>>
  atval_flag (Dwarf_Attribute attr)
  {
    bool flag;
    if (dwarf_formflag (&attr, &flag) != 0)
      throw_libdw ();
    return pass_single_value
      (std::make_unique <value_cst>
       (constant {static_cast <unsigned> (flag), &bool_constant_dom}, 0));
  }

  struct locexpr_producer
    : public value_producer <value>
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
}

namespace
{
  struct macinfo_producer
    : public value_producer <value>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Dwarf_Die m_cudie;
    Dwarf_Addr m_base;
    ptrdiff_t m_offset;
    size_t m_i;

    macinfo_producer (std::shared_ptr <dwfl_context> dwctx,
		      Dwarf_Die cudie)
      : m_dwctx {dwctx}
      , m_cudie (cudie)
      , m_offset {0}
      , m_i {0}
    {}

    using result_t = std::pair <macinfo_producer &, std::unique_ptr <value>>;

    static int
    callback (Dwarf_Macro *macro, void *data)
    {
      auto retp = static_cast <result_t *> (data);
      size_t &m_i = retp->first.m_i;

      value_seq::seq_t seq;

      unsigned int opcode;
      if (dwarf_macro_opcode (macro, &opcode) < 0)
	throw_libdw ();

      {
	constant c {opcode, &dw_macinfo_dom ()};
	seq.push_back (std::make_unique <value_cst> (c, 0));
      }

      constant_dom const *param1dom;
      switch (opcode)
	{
	case DW_MACINFO_define:
	case DW_MACINFO_undef:
	case DW_MACINFO_start_file:
	  param1dom = &line_number_dom;
	  break;

	case DW_MACINFO_vendor_ext:
	  param1dom = &dec_constant_dom;
	  break;

	default:
	  param1dom = nullptr;
	};

      if (param1dom != nullptr)
	{
	  Dwarf_Word param1;
	  if (dwarf_macro_param1 (macro, &param1) < 0)
	    throw_libdw ();
	  constant c {param1, param1dom};
	  seq.push_back (std::make_unique <value_cst> (c, 0));
	}

      switch (opcode)
	{
	case DW_MACINFO_define:
	case DW_MACINFO_undef:
	case DW_MACINFO_vendor_ext:
	  {
	    const char *str;
	    if (dwarf_macro_param2 (macro, nullptr, &str) < 0)
	      throw_libdw ();
	    seq.push_back
	      (std::make_unique <value_str> (std::string {str}, 0));
	    break;
	  }

	case DW_MACINFO_start_file:
	  // XXX file index that should be translated to a string.
	  {
	    Dwarf_Word param2;
	    if (dwarf_macro_param2 (macro, &param2, nullptr) < 0)
	      throw_libdw ();
	    constant c {param2, &dec_constant_dom};
	    seq.push_back (std::make_unique <value_cst> (c, 0));
	    break;
	  }

	default:;
	}

      retp->second = std::make_unique <value_seq> (std::move (seq), m_i);
      m_i++;

      return DWARF_CB_ABORT;
    }

    std::unique_ptr <value>
    next () override
    {
      result_t result {*this, nullptr};
      m_offset = dwarf_getmacros (&m_cudie, callback, &result, m_offset);
      if (m_offset < 0)
	throw_libdw ();
      if (m_offset == 0)
	return nullptr;

      return std::move (result.second);
    }
  };
}

value_aset
die_ranges (Dwarf_Die die)
{
  coverage cov;
  Dwarf_Addr base; // Cache for dwarf_ranges.
  for (ptrdiff_t off = 0;;)
    {
      Dwarf_Addr start, end;
      off = dwarf_ranges (&die, off, &base, &start, &end);
      if (off < 0)
	throw_libdw ();
      if (off == 0)
	break;

      cov.add (start, end - start);
    }

  return value_aset {cov, 0};
}

namespace
{
  bool
  is_block (Dwarf_Attribute attr)
  {
    switch (dwarf_whatform (&attr))
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
      return true;

    return false;
  }

  std::unique_ptr <value_producer <value>>
  handle_encoding_data (Dwarf_Attribute attr, Dwarf_Word encoding)
  {
    switch (encoding)
      {
      case DW_ATE_signed:
      case DW_ATE_signed_char:
	return atval_signed (attr);

      case DW_ATE_unsigned:
      case DW_ATE_unsigned_char:
      case DW_ATE_address:
	return atval_unsigned (attr);

      case DW_ATE_boolean:
	return atval_unsigned_with_domain (attr, bool_constant_dom);

      case DW_ATE_UTF:
	// XXX We could decode the character that this
	// represents.
	return atval_unsigned (attr);

      case DW_ATE_float:
      case DW_ATE_imaginary_float:
      case DW_ATE_complex_float:
      case DW_ATE_signed_fixed:
      case DW_ATE_unsigned_fixed:
      case DW_ATE_packed_decimal:
      case DW_ATE_decimal_float:
	return nullptr;

      default:
	{
	  std::stringstream ss;
	  ss << "Unhandled enumerator encoding: ";
	  dw_encoding_dom ().show (encoding, ss, brevity::full);
	  throw std::runtime_error (ss.str ());
	}
      }
  }

  std::unique_ptr <value_producer <value>>
  handle_encoding_block (Dwarf_Attribute attr, Dwarf_Word encoding)
  {
    Dwarf_Block block;
    if (dwarf_formblock (&attr, &block) != 0)
      throw_libdw ();

    // As long as the block is of convenient size, we can pretend the value is
    // actually a DW_FORM_dataX, and have the above code handle it.

    auto attr_as = [attr, block] (int form)
      {
	Dwarf_Attribute ret = attr;
	ret.form = form;
	ret.valp = block.data;
	return ret;
      };

    switch (block.length)
      {
      case 1:
	return handle_encoding_data (attr_as (DW_FORM_data1), encoding);
      case 2:
	return handle_encoding_data (attr_as (DW_FORM_data2), encoding);
      case 4:
	return handle_encoding_data (attr_as (DW_FORM_data4), encoding);
      case 8:
	return handle_encoding_data (attr_as (DW_FORM_data8), encoding);
      }

    // Pass as block.
    return nullptr;
  }

  std::unique_ptr <value_producer <value>>
  handle_encoding (Dwarf_Attribute attr, Dwarf_Word encoding)
  {
    if (is_block (attr))
      return handle_encoding_block (attr, encoding);
    else
      return handle_encoding_data (attr, encoding);
  }

  std::unique_ptr <value_producer <value>>
  handle_at_dependent_value (Dwarf_Attribute attr, value_die const &vd,
			     std::shared_ptr <dwfl_context> dwctx)
  {
    Dwarf_Die die = vd.get_die ();
    int code = dwarf_whatattr (&attr);
    switch (code)
      {
      case DW_AT_language:
	return atval_unsigned_with_domain (attr, dw_lang_dom ());

      case DW_AT_inline:
	return atval_unsigned_with_domain (attr, dw_inline_dom ());

      case DW_AT_encoding:
	return atval_unsigned_with_domain (attr, dw_encoding_dom ());

      case DW_AT_accessibility:
	return atval_unsigned_with_domain (attr, dw_access_dom ());

      case DW_AT_visibility:
	return atval_unsigned_with_domain (attr, dw_visibility_dom ());

      case DW_AT_virtuality:
	return atval_unsigned_with_domain (attr, dw_virtuality_dom ());

      case DW_AT_identifier_case:
	return atval_unsigned_with_domain (attr, dw_identifier_case_dom ());

      case DW_AT_calling_convention:
	return atval_unsigned_with_domain (attr, dw_calling_convention_dom ());

      case DW_AT_ordering:
	return atval_unsigned_with_domain (attr, dw_ordering_dom ());

      case DW_AT_decimal_sign:
	return atval_unsigned_with_domain (attr, dw_decimal_sign_dom ());

      case DW_AT_address_class:
	return atval_unsigned_with_domain (attr, dw_address_class_dom ());

      case DW_AT_endianity:
	return atval_unsigned_with_domain (attr, dw_endianity_dom ());

      case DW_AT_defaulted:
	return atval_unsigned_with_domain (attr, dw_defaulted_dom ());

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
	      type_die = vd.get_parent ()->get_die ();

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

	  auto get_type_die = [] (Dwarf_Die die)
	    {
	      auto fetch_type = [] (Dwarf_Die &die)
		{
		  Dwarf_Attribute a;
		  if (dwarf_hasattr_integrate (&die, DW_AT_type))
		    if (dwarf_attr_integrate (&die, DW_AT_type, &a) == nullptr
			|| dwarf_formref_die (&a, &die) == nullptr)
		      throw_libdw ();
		    else
		      return true;
		  else
		    return false;
		};

	      auto keep_peeling = [] (Dwarf_Die &die)
		{
		  switch (dwarf_tag (&die))
		  case DW_TAG_const_type:
		  case DW_TAG_volatile_type:
		  case DW_TAG_restrict_type:
		  case DW_TAG_typedef:
		  case DW_TAG_subrange_type:
		  case DW_TAG_packed_type:
		    return true;

		  return false;
		};

	      while (fetch_type (die) && keep_peeling (die))
		;

	      return die;
	    };

	  type_die = get_type_die (type_die);

	  int tag = dwarf_tag (&type_die);
	  if (tag == DW_TAG_pointer_type
	      || tag == DW_TAG_ptr_to_member_type)
	    return atval_unsigned_with_domain (attr, dw_address_dom ());

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
		if (name == std::string ("decltype(nullptr)"))
		  return atval_unsigned_with_domain (attr, dw_address_dom ());

	      // Ho hum.  This could be a structure or something
	      // similarly useless.  See if it's a block form at
	      // least.
	      break;
	    }
	  else
	    {
	      auto extract_encoding = [] (Dwarf_Die &die, Dwarf_Word &ret_enc)
		{
		  if (! dwarf_hasattr_integrate (&die, DW_AT_encoding))
		    return false;

		  Dwarf_Attribute at;
		  if (dwarf_attr_integrate (&die, DW_AT_encoding,
					    &at) == nullptr)
		    throw_libdw ();
		  if (dwarf_formudata (&at, &ret_enc) != 0)
		    throw_libdw ();
		  return true;
		};

	      Dwarf_Word encoding;
	      if (extract_encoding (type_die, encoding))
		{
		  if (auto ret = handle_encoding (attr, encoding))
		    return ret;
		}
	      else if (tag == DW_TAG_enumeration_type)
		{
		  // If there is a DW_AT_type on the enumeration type,
		  // we might be able to use it to figure out the type
		  // of the underlying datum.
		  if (dwarf_hasattr_integrate (&type_die, DW_AT_type))
		    {
		      Dwarf_Die type_type_die = get_type_die (type_die);
		      if (extract_encoding (type_type_die, encoding))
			if (auto ret = handle_encoding (attr, encoding))
			  return ret;
		    }

		  // We may be able to figure out the signedness from
		  // form of DW_AT_value of DW_TAG_enumeration_type's
		  // children.
		  bool seen_signed = false;
		  bool seen_unsigned = false;
		  for (auto it = child_iterator {type_die};
		       it != child_iterator::end (); ++it)
		    {
		      Dwarf_Attribute a;
		      if (dwarf_tag (*it) == DW_TAG_enumerator
			  && dwarf_attr (*it, DW_AT_const_value, &a) != nullptr)
			{
			  if (a.form == DW_FORM_sdata)
			    seen_signed = true;
			  else if (a.form == DW_FORM_udata)
			    seen_unsigned = true;
			}
		    }

		  if (seen_signed && ! seen_unsigned)
		    return atval_signed (attr);
		  if (seen_unsigned && ! seen_signed)
		    return atval_unsigned (attr);

		  // Maybe the value is small enough that signedness
		  // doesn't actually matter.
		  value_cst vcst = extract_unsigned (attr);
		  auto const &val = vcst.get_constant ().value ();
		  assert (val.is_unsigned ());
		  if (static_cast <int64_t> (val.uval ()) >= 0)
		    return pass_single_value
		      (std::make_unique <value_cst> (vcst));

		  {
		    ios_flag_saver ifs {std::cerr};
		    std::cerr << "Can't figure out signedness of "
				 "DW_AT_const_value on DIE "
			      << std::hex << std::showbase
			      << dwarf_dieoffset (&die) << std::endl;
		  }

		  return atval_signed (attr);
		}

	      // Otherwise break out and see if we can pass this as
	      // block.
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
      case DW_AT_string_length_bit_size:
      case DW_AT_string_length_byte_size:
      case DW_AT_rank:
      case DW_AT_alignment:
	return atval_unsigned (attr);

      case DW_AT_stmt_list:
      case DW_AT_str_offsets_base:
      case DW_AT_addr_base:
      case DW_AT_rnglists_base:
      case DW_AT_loclists_base:
	return atval_unsigned_with_domain (attr, hex_constant_dom);

      case DW_AT_data_member_location:
      case DW_AT_data_location:
      case DW_AT_frame_base:
      case DW_AT_location:
      case DW_AT_return_addr:
      case DW_AT_segment:
      case DW_AT_static_link:
      case DW_AT_use_location:
      case DW_AT_vtable_elem_location:
	return std::make_unique <locexpr_producer> (dwctx, attr);

      case DW_AT_ranges:
	return pass_single_value
		(std::make_unique <value_aset> (die_ranges (die)));

      case DW_AT_macro_info:
	{
	  Dwarf_Die cudie;
	  if (dwarf_diecu (&die, &cudie, nullptr, nullptr) == nullptr)
	    throw_libdw ();
	  return std::make_unique <macinfo_producer> (dwctx, cudie);
	}

      case DW_AT_GNU_macros:
	std::cerr << "DW_AT_GNU_macros NIY\n";
	return atval_unsigned_with_domain (attr, hex_constant_dom);

      case DW_AT_discr_value:
	// ^^^ """The number is signed if the tag type for the
	// variant part containing this variant is a signed
	// type."""
	{
	  std::stringstream ss;
	  ss << "Signedness of attribute ";
	  dw_attr_dom ().show (DW_AT_discr_value, ss, brevity::full);
	  ss << " not handled";
	  throw std::runtime_error (ss.str ());
	}
      }

    if (is_block (attr))
      return atval_block (attr);

    // There can be stuff out there that we have no chance of keeping up with.
    // Just assume unsigned instead of terminating the evaluation.
    if (code >= DW_AT_lo_user && code <= DW_AT_hi_user)
      return atval_unsigned (attr);

    std::stringstream ss;
    ss << "Signedness of attribute ";
    dw_attr_dom ().show (code, ss, brevity::full);
    ss << " not handled";
    throw std::runtime_error (ss.str ());
  }
}

std::unique_ptr <value_producer <value>>
at_value_cooked (std::shared_ptr <dwfl_context> dwctx,
		 value_die const &vd, Dwarf_Attribute attr)
{
  int form = dwarf_whatform (&attr);
  switch (form)
    {
    case DW_FORM_string:
    case DW_FORM_strp:
    case DW_FORM_GNU_strp_alt:
      return atval_string (attr);

    case DW_FORM_ref_addr:
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref8:
    case DW_FORM_ref_udata:
    case DW_FORM_GNU_ref_alt:
      {
	Dwarf_Die die;
	if (dwarf_formref_die (&attr, &die) == nullptr)
	  throw_libdw ();
	return pass_single_value
	  (std::make_unique <value_die> (dwctx, die, 0, doneness::cooked));
      }

    case DW_FORM_sdata:
      return atval_signed (attr);

    case DW_FORM_udata:
      return atval_unsigned (attr);

    case DW_FORM_addr:
      return atval_addr (attr);

    case DW_FORM_flag:
    case DW_FORM_flag_present:
      return atval_flag (attr);

    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
    case DW_FORM_sec_offset:
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
      return handle_at_dependent_value (attr, vd, dwctx);

    case DW_FORM_exprloc:
      return std::make_unique <locexpr_producer> (dwctx, attr);

    case DW_FORM_ref_sig8:
      std::cerr << "Form unhandled: "
		<< constant (dwarf_whatform (&attr), &dw_form_dom ())
		<< std::endl;
      return pass_single_value
	(std::make_unique <value_str> ("(form unhandled)", 0));

    case DW_FORM_indirect:
      assert (! "Unexpected DW_FORM_indirect");
      abort ();
    }

  std::stringstream ss;
  ss << "Unhandled DWARF form ";
  dw_form_dom ().show (form, ss, brevity::full);
  throw std::runtime_error (ss.str ());
}

namespace
{
  std::unique_ptr <value_producer <value>>
  atval_raw_strp (Dwarf_Attribute attr, bool alt)
  {
    const char *str = dwarf_formstring (&attr);
    if (str == nullptr)
      throw_libdw ();

    Dwarf *dw = dwarf_cu_getdwarf (attr.cu);
    if (alt)
      dw = dwarf_getalt (dw);
    if (dw == nullptr)
      throw_libdw ();

    size_t len;
    const char *str0 = dwarf_getstring (dw, 0, &len);

    uint64_t d = str - str0;

    value_cst cst {constant {d, &dw_address_dom ()}, 0};
    return pass_single_value (std::make_unique <value_cst> (cst));
  }
}

std::unique_ptr <value_producer <value>>
at_value_raw (Dwarf_Attribute attr)
{
  int form = dwarf_whatform (&attr);
  switch (form)
    {
    case DW_FORM_string:
      return atval_string (attr);

    case DW_FORM_strp:
      return atval_raw_strp (attr, false);
    case DW_FORM_GNU_strp_alt:
      return atval_raw_strp (attr, true);

    case DW_FORM_addr:
      return atval_addr (attr);

    case DW_FORM_ref_addr:
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref8:
    case DW_FORM_ref_udata:
    case DW_FORM_GNU_ref_alt:
      {
	Dwarf_Die die;
	if (dwarf_formref_die (&attr, &die) == nullptr)
	  throw_libdw ();
	Dwarf_Off off = dwarf_dieoffset (&die);
	return pass_single_value
	  (std::make_unique <value_cst>
	   (constant {static_cast <unsigned> (off), &dw_address_dom ()}, 0));
      }

    case DW_FORM_sec_offset:
      return atval_unsigned_with_domain (attr, dw_address_dom ());

    case DW_FORM_sdata:
      return atval_signed (attr);

    case DW_FORM_udata:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_data4:
    case DW_FORM_data8:
      return atval_unsigned (attr);

    case DW_FORM_flag:
    case DW_FORM_flag_present:
      return atval_flag (attr);

    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
    case DW_FORM_block:
    case DW_FORM_exprloc:
      return atval_block (attr);

    case DW_FORM_ref_sig8:
      std::cerr << "Form unhandled: "
		<< constant (dwarf_whatform (&attr), &dw_form_dom ())
		<< std::endl;
      return pass_single_value
	(std::make_unique <value_str> ("(form unhandled)", 0));

    case DW_FORM_indirect:
      assert (! "Unexpected DW_FORM_indirect");
      abort ();
    }

  std::stringstream ss;
  ss << "Unhandled DWARF form ";
  dw_form_dom ().show (form, ss, brevity::full);
  throw std::runtime_error (ss.str ());
}

namespace
{
  template <unsigned N>
  std::unique_ptr <value_producer <value>>
  select (std::unique_ptr <value_producer <value>> a,
	  std::unique_ptr <value_producer <value>> b);

  template <>
  std::unique_ptr <value_producer <value>>
  select<0> (std::unique_ptr <value_producer <value>> a,
	     std::unique_ptr <value_producer <value>> b)
  {
    return a;
  }

  template <>
  std::unique_ptr <value_producer <value>>
  select<1> (std::unique_ptr <value_producer <value>> a,
	     std::unique_ptr <value_producer <value>> b)
  {
    return b;
  }

  // Return up to two constants.  One or both may be default constants
  // (sans domain).  If the former is default, the latter shall be as
  // well.  Both default represent nullary operands, one non-default
  // represents unary, both non-default represent binary op.
  template <unsigned N>
  std::unique_ptr <value_producer <value>>
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
	    (pass_single_value
		(std::make_unique <value_die> (dwctx, die, 0,
					       doneness::cooked)),
	     pass_single_value
		(std::make_unique <value_cst>
			(signed_cst (op->number2, &dec_constant_dom), 0)));
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
	  Dwarf_Attribute attr;
	  if (dwarf_getlocation_attr
	      (const_cast <Dwarf_Attribute *> (&at), op, &attr) != 0)
	    throw_libdw ();

	  return select <N> (std::make_unique <locexpr_producer> (dwctx, attr),
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
	    (pass_single_value
		(std::make_unique <value_die> (dwctx, die, 0,
					       doneness::cooked)),
	     pass_block (block));
	}
      }
  }
}

std::unique_ptr <value_producer <value>>
dwop_number (std::shared_ptr <dwfl_context> dwctx,
	     Dwarf_Attribute const &attr, Dwarf_Op const *op)
{
  return locexpr_op_values <0> (dwctx, attr, op);
}

std::unique_ptr <value_producer <value>>
dwop_number2 (std::shared_ptr <dwfl_context> dwctx,
	     Dwarf_Attribute const &attr, Dwarf_Op const *op)
{
  return locexpr_op_values <1> (dwctx, attr, op);
}
