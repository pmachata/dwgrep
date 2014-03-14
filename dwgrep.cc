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

#include "dwpp.hh"
#include "tree.hh"
#include "parser.hh"

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

  assert (argc == 3);

  tree t = parse_query (argv[2]);
  t.check ();
  t.dump (std::cout);
  std::cout << std::endl;

  auto dw = open_dwarf (argv[1]);

  return 0;
}
