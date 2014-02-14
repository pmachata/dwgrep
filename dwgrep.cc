#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <exception>
#include <memory>
#include <system_error>
#include <iostream>

#include <dwarf.h>
#include <libdw.h>
#include <libelf.h>

#include "value.hh"
#include "dwpp.hh"
#include "patx.hh"

namespace
{
  inline void
  throw_system ()
  {
    throw std::runtime_error
      (std::error_code (errno, std::system_category ()).message ());
  }
}

std::shared_ptr <Dwarf>
open_dwarf (char const *fn)
{
  int fd = open (fn, O_RDONLY);
  if (fd == -1)
    throw_system ();

  Dwarf *dw = dwarf_begin (fd, DWARF_C_READ);
  if (dw == nullptr)
    throw_libdw ();

  return std::shared_ptr <Dwarf> (dw, dwarf_end);
}

int
main(int argc, char *argv[])
{
  elf_version (EV_CURRENT);

  auto x0 = std::unique_ptr <patx_group> {new patx_group {}};
  {
    {
      auto x1 = std::unique_ptr <patx_nodep_tag>
	{new patx_nodep_tag {DW_TAG_subprogram}};
      auto x2 = std::unique_ptr <patx_nodep_hasattr>
	{new patx_nodep_hasattr {DW_AT_declaration}};
      auto x3 = std::unique_ptr <patx_nodep_not>
	{new patx_nodep_not {std::move (x2)}};
      auto x4 = std::unique_ptr <patx_nodep_and>
	{new patx_nodep_and {std::move (x1), std::move (x3)}};
      x0->append (std::move (x4));
    }
    x0->append (std::unique_ptr <patx_edgep_child>
		{new patx_edgep_child {}});
    x0->append (std::unique_ptr <patx_nodep_tag>
		(new patx_nodep_tag {DW_TAG_formal_parameter}));
    x0->append (std::unique_ptr <patx_edgep_attr>
		(new patx_edgep_attr {DW_AT_type}));
    {
      auto y0 = std::unique_ptr <patx_group>
	{new patx_group {}};
      auto x1 = std::unique_ptr <patx_nodep_tag>
	{new patx_nodep_tag {DW_TAG_const_type}};
      auto x2 = std::unique_ptr <patx_nodep_tag>
	{new patx_nodep_tag {DW_TAG_volatile_type}};
      auto x3 = std::unique_ptr <patx_nodep_tag>
	{new patx_nodep_tag {DW_TAG_typedef}};
      auto x4 = std::unique_ptr <patx_nodep_or>
	{new patx_nodep_or {std::move (x1), std::move (x2)}};
      auto x5 = std::unique_ptr <patx_nodep_or>
	{new patx_nodep_or {std::move (x3), std::move (x4)}};
      y0->append (std::move (x5));
      y0->append (std::unique_ptr <patx_edgep_attr>
		  {new patx_edgep_attr {DW_AT_type}});

      auto y1 = std::unique_ptr <patx_repeat>
	{new patx_repeat {std::move (y0), 0, SIZE_MAX}};
      x0->append (std::move (y1));
    }
    {
      auto x1 = std::unique_ptr <patx_nodep_tag>
	{new patx_nodep_tag {DW_TAG_structure_type}};
      auto x2 = std::unique_ptr <patx_nodep_tag>
	{new patx_nodep_tag {DW_TAG_class_type}};
      auto x3 = std::unique_ptr <patx_nodep_tag>
	{new patx_nodep_tag {DW_TAG_union_type}};
      auto x4 = std::unique_ptr <patx_nodep_or>
	{new patx_nodep_or {std::move (x1), std::move (x2)}};
      auto x5 = std::unique_ptr <patx_nodep_or>
	{new patx_nodep_or {std::move (x3), std::move (x4)}};
      x0->append (std::move (x5));
    }
  }

  assert (argc == 2);
  auto dw = open_dwarf (argv[1]);

  auto ws = std::unique_ptr <wset> {new wset {wset::initial (dw)}};
  ws = x0->evaluate (std::move (ws));

  std::cout << "Result set has " << ws->size () << " elements." << std::endl;
  for (auto const &val: *ws)
    std::cout << "  " << val->format () << std::endl;

  /*
  something
    .match (tag (DW_TAG_subprogram) && ! hasattr (DW_AT_declaration))
    .match (child_edge)
    .match (tag (DW_TAG_formal_parameter);
  */

  return 0;
}
