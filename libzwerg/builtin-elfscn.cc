/*
   Copyright (C) 2018 Petr Machata
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
#include "dwcst.hh"
#include "dwpp.hh"
#include "elfcst.hh"
#include "value-elf.hh"
#include "value-symbol.hh"

namespace
{
  GElf_Shdr getshdr (Elf_Scn *scn)
  {
    GElf_Shdr shdr;
    if (gelf_getshdr (scn, &shdr) == nullptr)
      throw_libelf ();
    return shdr;
  }

  std::string getscnname (value_elf_section &sec)
  {
    GElf_Shdr shdr = ::getshdr (sec.get_scn ());

    std::shared_ptr <dwfl_context> ctx = sec.get_dwctx ();
    Elf *elf = get_main_elf (ctx->get_dwfl ()).first;

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

  struct symtab_producer
    : public value_producer <value>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    Elf *m_elf;
    Elf_Scn *m_scn;
    Elf_Data *m_data;
    size_t m_strtabndx;
    size_t m_ndx;
    size_t m_count;
    unsigned m_pos;
    doneness m_d;

    static Elf_Data *get_data (Elf_Scn *scn)
    {
      Elf_Data *ret = elf_getdata (scn, NULL);
      if (ret == nullptr)
	throw_libelf ();
      return ret;
    }

    static size_t get_strtabndx (Elf_Scn *symtab)
    {
      return ::getshdr (symtab).sh_link;
    }

    static size_t get_count (Elf *elf, Elf_Data *data)
    {
      size_t elem_sz = gelf_fsize (elf, data->d_type, 1,
				   elf_version (EV_CURRENT));
      return data->d_size / elem_sz;
    }

    explicit symtab_producer (std::unique_ptr <value_elf_section> sec)
      : m_dwctx {sec->get_dwctx ()}
      , m_elf {get_main_elf (m_dwctx->get_dwfl ()).first}
      , m_scn {sec->get_scn ()}
      , m_data {get_data (m_scn)}
      , m_strtabndx {get_strtabndx (m_scn)}
      , m_ndx {0}
      , m_count {get_count (m_elf, m_data)}
      , m_pos {0}
      , m_d {sec->get_doneness ()}
    {}

    std::unique_ptr <value>
    next () override
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
  };
}

std::unique_ptr <value_producer <value>>
op_entry_elfscn::operate (std::unique_ptr <value_elf_section> a) const
{
  std::shared_ptr <dwfl_context> dwctx = a->get_dwctx ();
  Elf_Scn *scn = a->get_scn ();
  GElf_Shdr shdr = ::getshdr (scn);

  switch (shdr.sh_type)
    {
    case SHT_STRTAB:
      return std::make_unique <::strtab_producer> (std::move (a));
    case SHT_SYMTAB:
    case SHT_DYNSYM:
      return std::make_unique <::symtab_producer> (std::move (a));
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
  GElf_Shdr shdr = ::getshdr (a->get_scn ());
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

uint64_t
elfscn_size_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::getshdr (scn).sh_size;
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
  return ::getshdr (scn).sh_entsize;
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
  return ::getshdr (scn).sh_type;
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

unsigned
elfscn_flags_def::value (Dwfl *dwfl, Elf_Scn *scn)
{
  return ::getshdr (scn).sh_flags;
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
  if ((::getshdr (a.get_scn ()).sh_flags & m_flag) == m_flag)
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
  return ::getshdr (scn).sh_addr;
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
  return ::getshdr (scn).sh_addralign;
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
  return ::getshdr (scn).sh_offset;
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
