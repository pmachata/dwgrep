#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#include <exception>
#include <iostream>
#include <memory>
#include <system_error>

#include <dwarf.h>
#include <libdw.h>
#include <libelf.h>

#include "dwpp.hh"
#include "tree.hh"
#include "parser.hh"
#include "valfile.hh"
#include "cache.hh"

namespace
{
  inline void
  throw_system ()
  {
    throw std::runtime_error
      (std::error_code (errno, std::system_category ()).message ());
  }
}

struct dwgrep_graph::pimpl
{
  parent_cache m_parcache;
  root_cache m_rootcache;

  Dwarf_Off
  find_parent (Dwarf_Die die)
  {
    return m_parcache.find (die);
  }

  bool
  is_root (Dwarf_Die die, Dwarf *dw)
  {
    return m_rootcache.is_root (die, dw);
  }
};

dwgrep_graph::dwgrep_graph (std::shared_ptr <Dwarf> d)
  : m_pimpl {std::make_unique <pimpl> ()}
  , dwarf {d}
{}

dwgrep_graph::~dwgrep_graph ()
{}

Dwarf_Off
dwgrep_graph::find_parent (Dwarf_Die die)
{
  return m_pimpl->find_parent (die);
}

bool
dwgrep_graph::is_root (Dwarf_Die die)
{
  return m_pimpl->is_root (die, &*dwarf);
}

std::shared_ptr <Dwarf>
open_dwarf (std::string const &fn)
{
  int fd = open (fn.c_str (), O_RDONLY);
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

  static option long_options[] = {
    {"quiet", no_argument, nullptr, 'q'},
    {"silent", no_argument, nullptr, 'q'},
    {"no-messages", no_argument, nullptr, 's'},
    {"expr", required_argument, nullptr, 'e'},
    {"count", no_argument, nullptr, 'c'},
    {"with-filename", no_argument, nullptr, 'H'},
    {"no-filename", no_argument, nullptr, 'h'},
    {nullptr, no_argument, nullptr, 0},
  };
  static char const *options = "ce:Hhqs";

  bool quiet = false;
  bool no_messages = false;
  bool show_count = false;
  bool with_filename = false;
  bool no_filename = false;

  std::vector <std::string> to_process;

  tree query;
  bool seen_query = false;

  while (true)
    {
      int c = getopt_long (argc, argv, options, long_options, nullptr);
      if (c == -1)
	break;

      switch (c)
	{
	case 'e':
	  query = parse_query (optarg);
	  seen_query = true;
	  break;

	case 'c':
	  show_count = true;
	  break;

	case 'H':
	  with_filename = true;
	  break;

	case 'h':
	  no_filename = true;
	  break;

	case 'q':
	  quiet = true;
	  break;

	case 's':
	  no_messages = true;
	  break;

	default:
	  std::exit (2);
	}
    }

  argc -= optind;
  argv += optind;

  if (! seen_query)
    {
      query = parse_query (*argv++);
      argc--;
    }

  size_t stk_depth = query.determine_stack_effects ();
  query.simplify ();
  if (! quiet)
    {
      query.dump (std::cerr);
      std::cerr << std::endl;
    }

  if (argc == 0)
    {
      std::cerr << "No input files.\n";
      return 2;
    }
  else
    for (int i = 0; i < argc; ++i)
      to_process.push_back (argv[i]);

  if (to_process.size () > 1)
    with_filename = true;
  if (no_filename)
    with_filename = false;

  bool errors = false;
  bool match = false;
  for (auto const &fn: to_process)
    {
      std::shared_ptr <dwgrep_graph> g;

      try
	{
	  g = std::make_shared <dwgrep_graph> (open_dwarf (fn));
	}
      catch (std::runtime_error e)
	{
	  if (! no_messages)
	    std::cout << "dwgrep: " << fn << ": " << e.what () << std::endl;
	  if (! quiet)
	    errors = true;
	  continue;
	}

      auto program = query.build_exec (nullptr, g, stk_depth);

      uint64_t count = 0;
      while (valfile::uptr result = program->next ())
	{
	  // grep: Exit immediately with zero status if any match
	  // is found, even if an error was detected.
	  if (quiet)
	    std::exit (0);

	  match = true;
	  if (! show_count)
	    {
	      if (with_filename)
		std::cout << fn << ":";
	      auto value = result->get_slot (slot_idx (0));
	      value->show (std::cout);
	      std::cout << "\n";
	    }
	  else
	    ++count;
	}

      if (show_count)
	{
	  if (with_filename)
	    std::cout << fn << ":";
	  std::cout << std::dec << count << std::endl;
	}
    }

  if (errors)
    std::exit (2);

  if (match)
    std::exit (0);
  else
    std::exit (1);
}
