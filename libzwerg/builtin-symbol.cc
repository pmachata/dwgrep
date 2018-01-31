/*
   Copyright (C) 2017, 2018 Petr Machata
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

#include <vector>
#include "builtin-symbol.hh"
#include "dwit.hh"
#include "dwcst.hh"

namespace
{
  struct symbol_producer
    : public value_producer <value_symbol>
  {
    std::shared_ptr <dwfl_context> m_dwctx;
    dwfl_module_iterator m_dwit;
    dwfl_module m_mod;
    unsigned m_symidx;
    unsigned m_symcount;
    size_t m_i;
    doneness m_doneness;

    bool
    next_module ()
    {
      if (m_dwit != dwfl_module_iterator::end ())
	{
	  m_mod = *m_dwit++;
	  int symcount = dwfl_module_getsymtab (m_mod);
	  if (symcount < 0)
	    throw_libdwfl ();
	  m_symcount = static_cast <unsigned> (symcount);
	  m_symidx = 0;
	  return true;
	}

      m_symcount = -1u;
      m_symidx = -1u;
      return false;
    }

    symbol_producer (std::shared_ptr <dwfl_context> dwctx, doneness d)
      : m_dwctx {dwctx}
      , m_dwit {dwctx->get_dwfl ()}
      , m_i {0}
      , m_doneness {d}
    {
      next_module ();
    }

    std::unique_ptr <value_symbol>
    next ()
    {
      while (m_symidx >= m_symcount)
	if (! next_module ())
	  return nullptr;

      GElf_Sym sym;
      GElf_Addr addr;
      GElf_Word shndx;
      Elf *elf;
      Dwarf_Addr bias;
      int symidx = m_symidx++;
      const char *name = dwfl_module_getsym_info (m_mod, symidx, &sym,
						  &addr, &shndx, &elf, &bias);
      if (name == nullptr)
	throw_libdwfl ();

      return std::make_unique <value_symbol> (m_dwctx, sym, name,
					      symidx, m_i++, m_doneness);
    }
  };
}

std::unique_ptr <value_producer <value_symbol>>
op_symbol_dwarf::operate (std::unique_ptr <value_dwarf> val) const
{
  return std::make_unique <symbol_producer> (val->get_dwctx (),
					     val->get_doneness ());
}

std::string
op_symbol_dwarf::docstring ()
{
  return
R"docstring(

Takes a Dwarf on TOS and yields every symbol found in any of the
symbol tables in ELF files that hosts the Dwarf data in question.

)docstring";
}


std::unique_ptr <value_producer <value_symbol>>
op_symbol_elf::operate (std::unique_ptr <value_elf> val) const
{
  std::abort ();
}

std::string
op_symbol_elf::docstring ()
{
  return
R"docstring(

xxx document me

)docstring";
}


value_str
op_name_symbol::operate (std::unique_ptr <value_symbol> val) const
{
  return {val->get_name (), 0};
}

std::string
op_name_symbol::docstring ()
{
  return
R"docstring(

Takes a symbol on TOS and yields its name.

)docstring";
}


value_cst
op_label_symbol::operate (std::unique_ptr <value_symbol> val) const
{
  return value_cst {val->get_type (), 0};
}

std::string
op_label_symbol::docstring ()
{
  return
R"docstring(

Takes a symbol on TOS and yields its type (a constant with domain
``STT_``).

)docstring";
}


value_cst
op_binding_symbol::operate (std::unique_ptr <value_symbol> val) const
{
  constant cst {GELF_ST_BIND (val->get_symbol ().st_info),
		&elfsym_stb_dom (val->get_dwctx ()->get_machine ())};
  return value_cst {cst, 0};
}

std::string
op_binding_symbol::docstring ()
{
  return
R"docstring(

Takes a symbol on TOS and yields its binding (a constant with domain
``STB_``).

)docstring";
}


value_cst
op_visibility_symbol::operate (std::unique_ptr <value_symbol> val) const
{
  constant cst {GELF_ST_VISIBILITY (val->get_symbol ().st_other),
		&elfsym_stv_dom ()};
  return value_cst {cst, 0};
}

std::string
op_visibility_symbol::docstring ()
{
  return
R"docstring(

Takes a symbol on TOS and yields its ELF visibility (a constant with
domain ``STV_``).

)docstring";
}


value_cst
op_address_symbol::operate (std::unique_ptr <value_symbol> val) const
{
  return value_cst {val->get_address (), 0};
}

std::string
op_address_symbol::docstring ()
{
  return
R"docstring(

Takes a symbol on TOS and yields its address (``st_value`` field of
``GElf_Sym`` structure).

)docstring";
}


value_cst
op_size_symbol::operate (std::unique_ptr <value_symbol> val) const
{
  constant cst {val->get_symbol ().st_size, &dec_constant_dom};
  return value_cst {cst, 0};
}

std::string
op_size_symbol::docstring ()
{
  return
R"docstring(

Takes a symbol on TOS and yields its size.

)docstring";
}
