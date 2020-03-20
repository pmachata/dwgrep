/*
   Copyright (C) 2018, 2019, 2020 Petr Machata
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

#include "builtin-elfscn.hh"

#include <cstring>
#include <elfutils/elf-knowledge.h>
#include "dwcst.hh"
#include "dwpp.hh"
#include "elfcst.hh"
#include "value-elf.hh"
#include "value-symbol.hh"

namespace
{
  std::string getscnname (value_elf_section &sec)
  {
    GElf_Shdr shdr = ::get_shdr (sec.get_scn ());
    Elf *elf = get_main_elf (sec.get_dwctx ()->get_dwfl ()).first;

    size_t ndx = get_shdrstrndx (elf);
    char *name = elf_strptr (elf, ndx, shdr.sh_name);
    if (name == nullptr)
      throw_libelf ();

    return {name};
  }
}

value_str
op_name_elfscn::operate (std::unique_ptr <value_elf_section> a) const
{
  return value_str {::getscnname (*a), 0};
}

std::string
op_name_elfscn::docstring ()
{
  return
R"docstring(

This word takes the ``T_ELFSCN`` value on TOS and yields the name of that
section.

xxx

)docstring";
}

namespace
{
  struct strtab_producer
    : public value_producer <value>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Elf *m_elf;
    Elf_Scn *m_scn;
    size_t m_offset;
    unsigned m_pos;

    explicit strtab_producer (std::unique_ptr <value_elf_section> sec)
      : m_dwctx {sec->get_dwctx ()}
      , m_elf {get_main_elf (m_dwctx->get_dwfl ()).first}
      , m_scn {sec->get_scn ()}
      , m_offset {0}
      , m_pos {0}
    {}

    std::unique_ptr <value>
    next () override
    {
      char const *nextstr = elf_strptr (m_elf, elf_ndxscn (m_scn), m_offset);
      if (nextstr == nullptr)
	{
	  if (m_offset == 0)
	    // If the first access fails, report an error. Otherwise we likely
	    // overran the section. Unfortunately the logic to determine the
	    // valid section length is not trivial, and libelf keeps error
	    // numbers private, so we can't just query that.
	    throw_libelf ();
	  return nullptr;
	}

      std::string str {nextstr};
      size_t off = m_offset;
      m_offset += str.length () + 1;
      return std::make_unique <value_strtab_entry> (std::move (str), off,
						    m_pos++);
    }
  };

  struct debug_str_producer
    : public value_producer <value>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Dwarf *m_dwarf;
    size_t m_offset;
    unsigned m_pos;

    explicit debug_str_producer (std::unique_ptr <value_elf_section> sec)
      : m_dwctx {sec->get_dwctx ()}
      , m_dwarf {get_main_dwarf (m_dwctx->get_dwfl ()).first}
      , m_offset {0}
      , m_pos {0}
    {}

    std::unique_ptr <value>
    next () override
    {
      size_t len;
      char const *nextstr = dwarf_getstring (m_dwarf, m_offset, &len);
      if (nextstr == nullptr)
	return nullptr;

      size_t off = m_offset;
      m_offset += len + 1;
      return std::make_unique <value_strtab_entry> (std::string {nextstr, len},
						    off, m_pos++);
    }
  };
}

size_t
symtab_producer::get_count (Elf *elf, Elf_Data *data)
{
  size_t elem_sz = gelf_fsize (elf, data->d_type, 1,
			       elf_version (EV_CURRENT));
  return data->d_size / elem_sz;
}

symtab_producer::symtab_producer (std::shared_ptr <dwfl_context> dwctx,
				  Elf_Scn *scn, doneness d)
  : m_dwctx {dwctx}
  , m_elf {get_main_elf (m_dwctx->get_dwfl ()).first}
  , m_scn {scn}
  , m_data {get_data (m_scn)}
  , m_strtabndx {get_link (m_scn)}
  , m_ndx {0}
  , m_count {get_count (m_elf, m_data)}
  , m_pos {0}
  , m_d {d}
{}

symtab_producer::symtab_producer (std::unique_ptr <value_elf_section> sec)
  : symtab_producer {sec->get_dwctx (), sec->get_scn (), sec->get_doneness ()}
{}

std::unique_ptr <value>
symtab_producer::next ()
{
  if (m_ndx >= m_count)
    return nullptr;
  GElf_Sym sym;
  if (gelf_getsym (m_data, m_ndx, &sym) == nullptr)
    throw_libelf ();

  char *name = elf_strptr (m_elf, m_strtabndx, sym.st_name);
  auto ret = std::make_unique <value_symbol> (m_dwctx, sym, name, m_ndx,
					      m_pos++, m_d);
  m_ndx++;
  return ret;
}

namespace
{
  struct relocation_producer
    : public value_producer <value>
    , public doneness_aspect
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Elf_Data *m_relocs;
    size_t m_symtabndx;
    size_t m_offset;
    unsigned m_pos;

    explicit relocation_producer (std::unique_ptr <value_elf_section> sec)
      : doneness_aspect {sec->get_doneness ()}
      , m_dwctx {sec->get_dwctx ()}
      , m_relocs {::get_data (sec->get_scn ())}
      , m_symtabndx {::get_link (sec->get_scn ())}
      , m_offset {0}
      , m_pos {0}
    {}
  };

  struct rela_producer
    : public relocation_producer
  {
    using relocation_producer::relocation_producer;

    std::unique_ptr <value>
    next () override
    {
      GElf_Rela rela;
      if (gelf_getrela (m_relocs, m_pos, &rela))
	return std::make_unique <value_elf_rel> (m_dwctx, rela,
						 m_dwctx->get_machine (),
						 m_symtabndx,
						 m_pos++, get_doneness ());
      return nullptr;
    }
  };

  struct rel_producer
    : public relocation_producer
  {
    using relocation_producer::relocation_producer;

    std::unique_ptr <value>
    next () override
    {
      GElf_Rel rel;
      if (gelf_getrel (m_relocs, m_pos, &rel))
	{
	  GElf_Rela rela { rel.r_offset, rel.r_info, 0 };
	  return std::make_unique <value_elf_rel> (m_dwctx, rela,
						   m_dwctx->get_machine (),
						   m_symtabndx,
						   m_pos++, get_doneness ());
	}
      return nullptr;
    }
  };
}

std::unique_ptr <value_producer <value>>
op_entry_elfscn::operate (std::unique_ptr <value_elf_section> a) const
{
  std::shared_ptr <dwfl_context> dwctx = a->get_dwctx ();
  Elf_Scn *scn = a->get_scn ();
  GElf_Shdr shdr = ::get_shdr (scn);

  switch (shdr.sh_type)
    {
    case SHT_STRTAB:
      return std::make_unique <::strtab_producer> (std::move (a));
    case SHT_SYMTAB:
    case SHT_DYNSYM:
      return std::make_unique <::symtab_producer> (std::move (a));
    case SHT_RELA:
      return std::make_unique <::rela_producer> (std::move (a));
    case SHT_REL:
      return std::make_unique <::rel_producer> (std::move (a));
    }

  std::string name = getscnname (*a);
  if (name == ".debug_str")
    return std::make_unique <::debug_str_producer> (std::move (a));

  return nullptr;
}

std::string
op_entry_elfscn::docstring ()
{
  return
R"docstring(

This word takes the ``T_ELFSCN`` value on TOS, and yields once for each entry of
that section. What entry that will be depends on the section type. E.g. sections
with type of SHT_STRTAB and DWARF string sections will yield values of type
``T_STRTAB_ENTRY``.

xxx

)docstring";
}

std::unique_ptr <value_elf_section>
op_link_elfscn::operate (std::unique_ptr <value_elf_section> a) const
{
  GElf_Shdr shdr = ::get_shdr (a->get_scn ());
  if (shdr.sh_link == 0)
    return nullptr;

  return std::make_unique <value_elf_section> (a->get_dwctx (), shdr.sh_link,
					       0, a->get_doneness ());
}

std::string
op_link_elfscn::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

std::unique_ptr <value>
op_info_elfscn::operate (std::unique_ptr <value_elf_section> a) const
{
  GElf_Shdr shdr = ::get_shdr (a->get_scn ());
  if (a->is_raw () || !SH_INFO_LINK_P (&shdr))
    {
      constant c {shdr.sh_info, &dec_constant_dom};
      return std::make_unique <value_cst> (c, 0);
    }

  if (shdr.sh_info == 0)
    return nullptr;

  return std::make_unique <value_elf_section> (a->get_dwctx (), shdr.sh_info,
					       0, a->get_doneness ());
}

std::string
op_info_elfscn::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

uint64_t
elfscn_size_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::get_shdr (scn).sh_size;
}

zw_cdom const &
elfscn_size_def::cdom (Dwfl *dwfl)
{
  return dec_constant_dom;
}

std::string
elfscn_size_def::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

uint64_t
elfscn_entsize_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::get_shdr (scn).sh_entsize;
}

zw_cdom const &
elfscn_entsize_def::cdom (Dwfl *dwfl)
{
  return dec_constant_dom;
}

std::string
elfscn_entsize_def::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

unsigned
elfscn_label_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::get_shdr (scn).sh_type;
}

zw_cdom const &
elfscn_label_def::cdom (Dwfl *dwfl)
{
  int machine = get_ehdr (dwfl).e_machine;
  return elf_sht_dom (machine);
}

std::string
elfscn_label_def::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}


pred_label_elfscn::pred_label_elfscn (unsigned value, int machine)
  : m_value {constant (value, &elf_sht_dom (machine))}
{}

pred_result
pred_label_elfscn::result (value_elf_section &a) const
{
  Dwfl *dwfl = a.get_dwctx ()->get_dwfl ();
  zw_cdom const &dom = elf_sht_dom (get_ehdr (dwfl).e_machine);

  unsigned type = ::get_shdr (a.get_scn ()).sh_type;
  constant type_cst {type, &dom};

  return pred_result (type_cst == m_value);
}

std::string
pred_label_elfscn::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

unsigned
elfscn_flags_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::get_shdr (scn).sh_flags;
}

zw_cdom const &
elfscn_flags_def::cdom (Dwfl *dwfl)
{
  int machine = get_ehdr (dwfl).e_machine;
  return elf_shf_flags_dom (machine);
}

std::string
elfscn_flags_def::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

pred_result
pred_flags_elfscn::result (value_elf_section &a) const
{
  if ((::get_shdr (a.get_scn ()).sh_flags & m_flag) == m_flag)
    return pred_result::yes;
  else
    return pred_result::no;
}

std::string
pred_flags_elfscn::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

unsigned
elfscn_address_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::get_shdr (scn).sh_addr;
}

zw_cdom const &
elfscn_address_def::cdom (Dwfl *dwfl)
{
  return dw_address_dom ();
}

std::string
elfscn_address_def::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

unsigned
elfscn_alignment_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::get_shdr (scn).sh_addralign;
}

zw_cdom const &
elfscn_alignment_def::cdom (Dwfl *dwfl)
{
  return hex_constant_dom;
}

std::string
elfscn_alignment_def::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

unsigned
elfscn_offset_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::get_shdr (scn).sh_offset;
}

zw_cdom const &
elfscn_offset_def::cdom (Dwfl *dwfl)
{
  return dw_offset_dom ();
}

std::string
elfscn_offset_def::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

namespace
{
  template <unsigned int sz> struct get_unsigned;
  template <> struct get_unsigned <1> { using type = uint8_t; };
  template <> struct get_unsigned <2> { using type = uint16_t; };
  template <> struct get_unsigned <4> { using type = uint32_t; };
  template <> struct get_unsigned <8> { using type = uint64_t; };

  uint8_t be_to_host (uint8_t c) { return c; }
  uint8_t le_to_host (uint8_t c) { return c; }

  uint16_t be_to_host (uint16_t c) { return be16toh (c); }
  uint16_t le_to_host (uint16_t c) { return le16toh (c); }

  uint32_t be_to_host (uint32_t c) { return be32toh (c); }
  uint32_t le_to_host (uint32_t c) { return le32toh (c); }

  uint64_t be_to_host (uint64_t c) { return be64toh (c); }
  uint64_t le_to_host (uint64_t c) { return le64toh (c); }
}

template <typename T>
std::unique_ptr <value_cst>
op_read_elfscn <T>::operate (std::unique_ptr <value_elf_section> a,
			     std::unique_ptr <value_cst> offset_cst) const
{
  Elf_Scn *scn = a->get_scn ();
  Elf_Data *data = get_rawdata (scn);
  uint64_t offset = constant_to_address (offset_cst->get_constant ());
  if (data->d_size < sizeof (T) ||
      static_cast <uint64_t> (data->d_size - sizeof (T)) < offset)
    return nullptr;

  auto buf = static_cast <unsigned char *> (data->d_buf);
  typename get_unsigned <sizeof (T)>::type u;
  std::memcpy (&u, &buf[offset], sizeof u);

  if (a->is_cooked ())
    {
      GElf_Ehdr ehdr = get_ehdr (a->get_dwctx ()->get_dwfl ());
      switch (ehdr.e_ident[EI_DATA])
	{
	case ELFDATA2LSB: // Little endian.
	  u = le_to_host (u);
	  break;

	case ELFDATA2MSB: // Big endian.
	  u = be_to_host (u);
	  break;

	default:
	  throw std::runtime_error ("Invalid endianness");
	}
    }

  T ret = static_cast <T> (u);
  constant c {ret, &dec_constant_dom};
  return std::make_unique <value_cst> (c, 0);
}

template <typename T>
std::string
op_read_elfscn <T>::docstring ()
{
  return
R"docstring(

xxx

)docstring";
}

template class op_read_elfscn <int8_t>;
template class op_read_elfscn <int16_t>;
template class op_read_elfscn <int32_t>;
template class op_read_elfscn <int64_t>;
template class op_read_elfscn <uint8_t>;
template class op_read_elfscn <uint16_t>;
template class op_read_elfscn <uint32_t>;
template class op_read_elfscn <uint64_t>;
