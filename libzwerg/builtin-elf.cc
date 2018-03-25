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

#include "builtin-elf.hh"
#include "dwpp.hh"
#include "elfcst.hh"

namespace
{
  int
  find_no_debuginfo (Dwfl_Module *mod, void **userdata,
		     const char *modname, Dwarf_Addr base,
		     const char *file_name,
		     const char *debuglink_file, GElf_Word debuglink_crc,
		     char **debuginfo_file_name)
  {
    *debuginfo_file_name = NULL;
    return -ENOENT;
  }

  Dwfl_Module *
  get_sole_module (Dwfl *dwfl)
  {
    struct init_state
    {
      Dwfl_Module *mod;

      init_state ()
	: mod {nullptr}
      {}

      static int each_module_cb (Dwfl_Module *mod, void **userdata,
				 char const *fn, Dwarf_Addr addr, void *arg)
      {
	auto self = static_cast <init_state *> (arg);

	// For Dwarf contexts that libzwerg currently creates, there ought to be
	// exactly one module.
	assert (self->mod == nullptr);
	self->mod = mod;
	return DWARF_CB_OK;
      }
    };

    init_state ist;
    if (dwfl_getmodules (dwfl, init_state::each_module_cb, &ist, 0) == -1)
      throw_libdwfl ();

    assert (ist.mod != nullptr);
    return ist.mod;
  }

  struct elf_producer
    : public value_producer <value_elf>
  {
    std::unique_ptr <value_dwarf> m_dw;
    char const *m_mainfile;
    char const *m_debugfile;
    unsigned m_pos;

    explicit elf_producer (std::unique_ptr <value_dwarf> dw)
      : m_dw {std::move (dw)}
      , m_pos {0}
    {
      Dwfl_Module *mod = get_sole_module (m_dw->get_dwctx ()->get_dwfl ());
      dwfl_module_info (mod, NULL, NULL, NULL, NULL, NULL,
			&m_mainfile, &m_debugfile);
    }

    std::unique_ptr <value_elf>
    yield_elf (char const **namep)
    {
      std::string fn = *namep;
      *namep = nullptr;

      const static Dwfl_Callbacks callbacks =
	{
	  .find_elf = dwfl_build_id_find_elf,
	  .find_debuginfo = find_no_debuginfo,
	  .section_address = dwfl_offline_section_address,
	};

      auto ctx = std::make_shared <dwfl_context> (open_dwfl (fn, callbacks));
      return std::make_unique <value_elf> (ctx, fn, m_pos++,
					   m_dw->get_doneness ());
    }

    std::unique_ptr <value_elf>
    next () override
    {
      if (m_mainfile)
	return yield_elf (&m_mainfile);
      else if (m_debugfile)
	return yield_elf (&m_debugfile);
      else
	return nullptr;
    }
  };

  std::pair <Elf *, GElf_Addr>
  get_main_elf (Dwfl *dwfl)
  {
    Dwfl_Module *mod = get_sole_module (dwfl);

    GElf_Addr bias;
    Elf *elf = dwfl_module_getelf (mod, &bias);
    assert (elf != nullptr);

    return {elf, bias};
  }
}

std::unique_ptr <value_producer <value_elf>>
op_elf_dwarf::operate (std::unique_ptr <value_dwarf> val) const
{
  return std::make_unique <elf_producer> (std::move (val));
}

std::string
op_elf_dwarf::docstring ()
{
  return
R"docstring(

xxx document me

)docstring";
}


value_str
op_name_elf::operate (std::unique_ptr <value_elf> a) const
{
  return value_str {std::string {a->get_fn ()}, 0};
}

std::string
op_name_elf::docstring ()
{
  return
R"docstring(

xxx document me

)docstring";
}


namespace
{
  unsigned
  elfclass (Dwfl *dwfl)
  {
    Elf *elf = get_main_elf (dwfl).first;
    // Note: Bias ignored.
    GElf_Ehdr ehdr;
    if (gelf_getehdr (elf, &ehdr) == nullptr)
      throw_libelf ();

    return ehdr.e_ident[EI_CLASS];
  }
}

value_cst
op_atclass_dwarf::operate (std::unique_ptr <value_dwarf> val) const
{
  unsigned cls = elfclass (val->get_dwctx ()->get_dwfl ());
  return value_cst {constant {cls, &elf_class_dom ()}, 0};
}

std::string
op_atclass_dwarf::docstring ()
{
  return
R"docstring(

xxx document me

)docstring";
}

value_cst
op_atclass_elf::operate (std::unique_ptr <value_elf> val) const
{
  unsigned cls = elfclass (val->get_dwctx ()->get_dwfl ());
  return value_cst {constant {cls, &elf_class_dom ()}, 0};
}

std::string
op_atclass_elf::docstring ()
{
  return
R"docstring(

xxx document me

)docstring";
}

pred_result
pred_atclass_dwarf::result (value_dwarf &a) const
{
  unsigned cls = elfclass (a.get_dwctx ()->get_dwfl ());
  return pred_result (cls == m_cls);
}

std::string
pred_atclass_dwarf::docstring ()
{
  return
R"docstring(

xxx document me

)docstring";
}

pred_result
pred_atclass_elf::result (value_elf &a) const
{
  unsigned cls = elfclass (a.get_dwctx ()->get_dwfl ());
  return pred_result (cls == m_cls);
}

std::string
pred_atclass_elf::docstring ()
{
  return
R"docstring(

xxx document me

)docstring";
}
