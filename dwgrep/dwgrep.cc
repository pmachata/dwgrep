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

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>
#include <functional>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <libintl.h>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include "libzwerg.hh"
#include "libzwerg-dw.h"
#include "libzwerg-elf.h"
#include "options.hh"
#include "libzwerg/std-memory.hh"
#include "libzwerg/strip.hh"
#include "version.h"
#include "libzwerg/flag_saver.hh"

std::unique_ptr <option[]>
gen_options (std::vector <ext_option> const &ext_opts)
{
  size_t sz = ext_opts.size ();
  auto ret = std::make_unique <option[]> (sz + 1);

  for (size_t i = 0; i < sz; ++i)
    // Slice off the extra stuff.
    ret[i] = ext_opts[i];

  ret[sz] = (struct option) {0, 0, 0, 0};
  return ret;
}

std::string
gen_shopts (std::vector <ext_option> const &ext_opts)
{
  std::string ret;
  for (auto const &ext_opt: ext_opts)
    {
      auto shopt = ext_opt.shopt ();
      if (std::find (ret.begin (), ret.end (), shopt[0]) == ret.end ())
	ret += shopt;
    }
  return ret;
}

void
show_help (std::vector <ext_option> const &ext_opts)
{
  std::cout << "Usage:\n  dwgrep FILE... -e PATTERN\n\n"
	    << "Options:\n";

  std::map <int, std::pair <std::vector <std::string>, std::string>> opts
    = merge_options (ext_opts);

  for (auto const &opt: opts)
    {
      std::cout << "  ";
      bool seen = false;
      for (auto const &l: opt.second.first)
	{
	  std::cout << (seen ? ", " : "") << l;
	  seen = true;
	}

      std::string ds = strip (opt.second.second, " \t\n");
      auto period = ds.find_first_of (".");
      while (ds.length () > period && ! std::isspace (ds[period + 1]))
        period = ds.find_first_of (".", period + 1);
      std::cout << "\n\t" << ds.substr (0, period) << "\n";
    }
}

class dumper
{
  zw_vocabulary const &m_voc;

public:
  explicit dumper (zw_vocabulary const &voc)
    : m_voc (voc)
  {}

  enum class format
    {
      full,
      brief,
      inner_brief,
      header,
    };

  void dump_value (std::ostream &os, zw_value const &val, format fmt);

private:
  void dump_const (std::ostream &os, zw_value const &val, format fmt);
  void dump_charp (std::ostream &os, char const *buf, size_t len, format fmt);
  void dump_string (std::ostream &os, zw_value const &val, format fmt);
  void dump_seq (std::ostream &os, zw_value const &val, format fmt);
  void dump_dwarf (std::ostream &os, zw_value const &val, format fmt);
  void dump_cu (std::ostream &os, zw_value const &val, format fmt);
  void dump_die (std::ostream &os, zw_value const &val, format fmt);
  void dump_attr (std::ostream &os, zw_value const &val, format fmt);
  void dump_llelem (std::ostream &os, zw_value const &val, format fmt);
  void dump_llop (std::ostream &os, zw_value const &val, format fmt);
  void dump_aset (std::ostream &os, zw_value const &val, format fmt);
  void dump_elfsym (std::ostream &os, zw_value const &val, format fmt);
  void dump_elf (std::ostream &os, zw_value const &val, format fmt);
  void dump_elfscn (std::ostream &os, zw_value const &val, format fmt);
  void dump_strtab_entry (std::ostream &os, zw_value const &val, format fmt);
  void dump_named_constant (std::ostream &os, unsigned cst, zw_cdom const &dom);
};

void
dumper::dump_const (std::ostream &os, zw_value const &val, format fmt)
{
  std::unique_ptr <zw_value, zw_deleter> str
       {fmt == format::full ? zw_value_const_format (&val, zw_throw_on_error {})
	: zw_value_const_format_brief (&val, zw_throw_on_error {})};
  dump_value (os, *str.get (), format::full);
}

void
dumper::dump_charp (std::ostream &os, char const *buf, size_t len, format fmt)
{
  if (fmt == format::full)
    os << std::string {buf, len};
  else
    {
      os << '"';
      for (size_t i = 0; i < len; ++i)
	switch (buf[i])
	  {
#define ESCAPE(L, E) case L: os << E; break

	    ESCAPE (0, "\\0");
	    ESCAPE ('"', "\\");
	    ESCAPE ('\\', "\\\\");
	    ESCAPE ('\a', "\\a");
	    ESCAPE ('\b', "\\b");
	    ESCAPE ('\t', "\\t");
	    ESCAPE ('\n', "\\n");
	    ESCAPE ('\v', "\\v");
	    ESCAPE ('\f', "\\f");
	    ESCAPE ('\r', "\\r");

#undef ESCAPE

	  default:
	    if (isprint (buf[i]))
	      os << buf[i];
	    else
	      {
		ios_flag_saver ifs {os};
		os << "\\x" << std::hex << std::setw (2)
		   << (unsigned) (unsigned char) buf[i];
	      }
	  }
      os << '"';
    }
}

void
dumper::dump_string (std::ostream &os, zw_value const &val, format fmt)
{
  size_t len;
  char const *buf = zw_value_str_str (&val, &len);
  dump_charp (os, buf, len, fmt);
}

void
dumper::dump_seq (std::ostream &os, zw_value const &val, format)
{
  os << "[";
  for (size_t n = zw_value_seq_length (&val), i = 0; i < n; ++i)
    {
      if (i > 0)
	os << ", ";
      zw_value const *emt = zw_value_seq_at (&val, i);
      assert (emt != nullptr);
      dump_value (os, *emt, format::brief);
    }
  os << "]";
}

void
dumper::dump_dwarf (std::ostream &os, zw_value const &val, format fmt)
{
  char const *name = zw_value_dwarf_name (&val);
  if (fmt == format::header)
    // Full format actually just dumps the file without decoration.
    dump_charp (os, name, strlen (name), format::full);
  else
    {
      os << "<Dwarf ";
      dump_charp (os, name, strlen (name), format::brief);
      os << '>';
    }
}

void
dumper::dump_cu (std::ostream &os, zw_value const &val, format)
{
  ios_flag_saver ifs {os};
  os << "<CU " << std::hex << std::showbase << zw_value_cu_offset (&val) << ">";
}

void
exec_query_on (zw_stack const &stack, zw_query const &q,
	       std::function <void (zw_stack const &)> cb)
{
  for (std::unique_ptr <zw_result, zw_deleter> result
	{zw_query_execute (&q, &stack, zw_throw_on_error {})};
       auto out = zw_result_next (*result); )
    cb (*out);
}

void
exec_query_on (zw_value const &val, zw_query const &q,
	       std::function <void (zw_stack const &)> cb)
{
  std::unique_ptr <zw_stack, zw_deleter> stack
	{zw_stack_init (zw_throw_on_error {})};
  zw_stack_push (stack.get (), &val, zw_throw_on_error {});

  return exec_query_on (*stack, q, cb);
}

void
dumper::dump_die (std::ostream &os, zw_value const &val, format fmt)
{
  Dwarf_Die die = zw_value_die_die (&val);
  std::unique_ptr <zw_value, zw_deleter> tag
	{zw_value_init_const_u64 (dwarf_tag (&die), zw_cdom_dw_tag (), 0,
				  zw_throw_on_error {})};

  std::string tag_name = [&] ()
    {
      std::unique_ptr <zw_value, zw_deleter> tmp
	{zw_value_const_format_brief (tag.get (), zw_throw_on_error {})};

      size_t sz;
      auto buf = zw_value_str_str (tmp.get (), &sz);
      return std::string {buf, sz};
    } ();

  {
    ios_flag_saver ifs {os};
    os << '[' << std::hex << dwarf_dieoffset (&die) << ']'
       << (fmt == format::full ? '\t' : ' ') << tag_name;
  }

  if (fmt == format::full)
    {
      static std::unique_ptr <zw_query, zw_deleter> query
	{zw_query_parse (&m_voc, "raw attribute cooked", zw_throw_on_error {})};
      exec_query_on (val, *query,
		     [&] (zw_stack const &stk) -> void {
		       assert (zw_stack_depth (&stk) == 1);
		       dump_attr (os << "\n\t", *zw_stack_at (&stk, 0),
				  format::brief);
		     });
    }
}

void
dumper::dump_attr (std::ostream &os, zw_value const &val, format fmt)
{
  static std::unique_ptr <zw_query, zw_deleter> query
	{zw_query_parse (&m_voc, "[value] swap label", zw_throw_on_error {})};
  exec_query_on (val, *query,
		 [&] (zw_stack const &stk) -> void
		 {
		   assert (zw_stack_depth (&stk) == 2);
		   dump_value (os, *zw_stack_at (&stk, 0), format::brief);

		   zw_value const *values = zw_stack_at (&stk, 1);
		   assert (zw_value_is_seq (values));

		   switch (size_t n = zw_value_seq_length (values))
		     {
		     case 0:
		       os << "\t<no value>";
		       break;
		     case 1:
		       dump_value (os << "\t",
				   *zw_value_seq_at (values, 0),
				   format::brief);
		       break;
		     default:
		       for (size_t i = 0; i < n; ++i)
			 {
			   os << (fmt == format::brief ? "\n\t\t" : "\n\t");
			   dump_value (os, *zw_value_seq_at (values, i),
				       format::brief);
			 }
		       break;
		     }
		 });
}

void
dumper::dump_llelem (std::ostream &os, zw_value const &val, format fmt)
{
  {
    ios_flag_saver ifs {os};
    os << std::hex << std::showbase
       << zw_value_llelem_low (&val) << ".."
       << zw_value_llelem_high (&val) << ":";
  }

  static std::unique_ptr <zw_query, zw_deleter> query
	{zw_query_parse (&m_voc, "[elem]",
			 zw_throw_on_error {})};
  exec_query_on (val, *query,
		 [&] (zw_stack const &stk) -> void
		 {
		   assert (zw_stack_depth (&stk) == 2);
		   zw_value const *ops = zw_stack_at (&stk, 0);
		   assert (zw_value_is_seq (ops));

		   if (size_t n = zw_value_seq_length (ops))
		     for (size_t i = 0; i < n; ++i)
		       {
			 if (i > 0)
			   os << ", ";

			 dump_value (os, *zw_value_seq_at (ops, i),
				     format::brief);
		       }
		   else
		     os << "<empty location expression>";
		 });
}

void
dumper::dump_llop (std::ostream &os, zw_value const &val, format fmt)
{
  // We could get offset and label through zw_value_llop_op, but to
  // get values reliably and without duplication, we'd need to go
  // through Zwerg query "value" anyway.  So just do it all it one go.
  static std::unique_ptr <zw_query, zw_deleter> query
	{zw_query_parse (&m_voc, "[offset, label, value]",
			 zw_throw_on_error {})};

  exec_query_on (val, *query,
		 [&] (zw_stack const &stk) -> void
		 {
		   assert (zw_stack_depth (&stk) == 2);
		   zw_value const *properties = zw_stack_at (&stk, 0);
		   assert (zw_value_is_seq (properties));

		   zw_value const *off = zw_value_seq_at (properties, 0);
		   dump_value (os, *off, format::brief);
		   os << ' ';

		   zw_value const *lbl = zw_value_seq_at (properties, 1);
		   dump_value (os, *lbl, format::brief);

		   size_t nppts = zw_value_seq_length (properties);
		   for (size_t j = 2; j < nppts; ++j)
		     {
		       os << ' ';
		       dump_value (os, *zw_value_seq_at (properties, j),
				   format::inner_brief);
		     }
		 });
}

void
dumper::dump_aset (std::ostream &os, zw_value const &val, format fmt)
{
  ios_flag_saver ifs {os};
  os << std::hex << std::showbase;
  if (size_t n = zw_value_aset_length (&val))
    for (size_t i = 0; i < n; ++i)
      {
	if (i > 0)
	  os << ", ";
	zw_aset_pair p = zw_value_aset_at (&val, i);
	os << p.start << ".." << (p.start + p.length);
      }
  else
    os << "<empty range>";
}

void
dumper::dump_named_constant (std::ostream &os, unsigned v, zw_cdom const &dom)
{
  std::unique_ptr <zw_value, zw_deleter> cst
	{zw_value_init_const_u64 (v, &dom, 0, zw_throw_on_error {})};
  std::unique_ptr <zw_value, zw_deleter> str
	{zw_value_const_format_brief (cst.get (), zw_throw_on_error {})};

  dump_string (os, *str, format::full);
}

void
dumper::dump_elfsym (std::ostream &os, zw_value const &val, format fmt)
{
  GElf_Sym sym = zw_value_elfsym_symbol (&val);
  os << zw_value_elfsym_symidx (&val) << ":\t";
  {
    ios_flag_saver ifs {os};
    os << std::hex << std::showbase << std::setfill ('0') << std::setw (16)
       << std::internal << sym.st_value << ' ';
  }
  {
    ios_flag_saver ifs {os};
    os << std::setw (6) << sym.st_size;
  }

  zw_value const *dw = zw_value_elfsym_dwarf (&val, zw_throw_on_error {});
  zw_machine const *machine = zw_value_dwarf_machine (dw, zw_throw_on_error {});

  dump_named_constant (os << ' ', GELF_ST_TYPE (sym.st_info),
		       *zw_cdom_elfsym_stt (machine));
  dump_named_constant (os << '\t', GELF_ST_BIND (sym.st_info),
		       *zw_cdom_elfsym_stb (machine));
  dump_named_constant (os << '\t', GELF_ST_VISIBILITY (sym.st_other),
		       *zw_cdom_elfsym_stv ());

  os << '\t' << zw_value_elfsym_name (&val);
}

void
dumper::dump_elf (std::ostream &os, zw_value const &val, format fmt)
{
  zw_value const *dw = zw_value_elf_dwarf (&val, zw_throw_on_error {});
  char const *name = zw_value_dwarf_name (dw);

  os << "<Elf ";
  dump_charp (os, name, strlen (name), format::brief);
  os << '>';
}

void
dumper::dump_elfscn (std::ostream &os, zw_value const &val, format fmt)
{
  os << "<ElfScn xxx";
  os << '>';
}

void
dumper::dump_strtab_entry (std::ostream &os, zw_value const &val, format fmt)
{
  {
    ios_flag_saver ifs {os};
    os << "[" << std::hex << std::setw (5)
       << zw_value_strtab_entry_offset (&val) << "]  ";
  }
  os << '"' << zw_value_strtab_entry_string (&val) << '"';
}

void
dumper::dump_value (std::ostream &os, zw_value const &val, format fmt)
{
  bool brackets;
  if (fmt == format::inner_brief)
    {
      brackets = true;
      fmt = format::brief;
    }
  else
    brackets = false;

  if (brackets)
    os << "<";

  if (zw_value_is_const (&val))
    dump_const (os, val, fmt);
  else if (zw_value_is_str (&val))
    dump_string (os, val, fmt);
  else if (zw_value_is_seq (&val))
    dump_seq (os, val, fmt);
  else if (zw_value_is_dwarf (&val))
    dump_dwarf (os, val, fmt);
  else if (zw_value_is_cu (&val))
    dump_cu (os, val, fmt);
  else if (zw_value_is_die (&val))
    dump_die (os, val, fmt);
  else if (zw_value_is_attr (&val))
    dump_attr (os, val, fmt);
  else if (zw_value_is_llelem (&val))
    dump_llelem (os, val, fmt);
  else if (zw_value_is_llop (&val))
    dump_llop (os, val, fmt);
  else if (zw_value_is_aset (&val))
    dump_aset (os, val, fmt);
  else if (zw_value_is_elfsym (&val))
    dump_elfsym (os, val, fmt);
  else if (zw_value_is_elf (&val))
    dump_elf (os, val, fmt);
  else if (zw_value_is_elfscn (&val))
    dump_elfscn (os, val, fmt);
  else if (zw_value_is_strtab_entry (&val))
    dump_strtab_entry (os, val, fmt);
  else
    os << (/* assert (false), */"<unknown value type>");

  if (brackets)
    os << ">";
}

namespace
{
  std::ostream &
  error_message (bool no_messages)
  {
    static std::ofstream sink;
    std::ostream &ret = no_messages ? sink : std::cerr;
    return ret;
  }

  std::ostream &
  error_message (bool no_messages, int verbosity, bool &errors)
  {
    if (verbosity >= 0)
      errors = true;
    return error_message (no_messages);
  }

  std::vector <std::unique_ptr <zw_value, zw_deleter>>
  parse_arg_literal (std::string arg)
  {
    std::unique_ptr <zw_value, zw_deleter> value
	{zw_value_init_str_len (arg.c_str (), arg.length (), 0,
				zw_throw_on_error {})};
    std::vector <std::unique_ptr <zw_value, zw_deleter>> ret;
    ret.emplace_back (std::move (value));
    return ret;
  }

  std::vector <std::unique_ptr <zw_value, zw_deleter>>
  parse_arg_eval (zw_vocabulary const &voc, std::string arg)
  {
    std::vector <std::unique_ptr <zw_value, zw_deleter>> ret;

    try
      {
	std::unique_ptr <zw_query, zw_deleter> query
	    {zw_query_parse_len (&voc, arg.c_str (), arg.length (),
				 zw_throw_on_error {})};
	std::unique_ptr <zw_stack, zw_deleter> empty
	    {zw_stack_init (zw_throw_on_error {})};

	exec_query_on (*empty, *query,
		       [&] (zw_stack const &stk) -> void
		       {
			 if (zw_stack_depth (&stk) == 0)
			   throw std::runtime_error ("empty stack yielded");

			 zw_value const &tos = *zw_stack_at (&stk, 0);
			 size_t pos = zw_value_pos (&tos);
			 std::unique_ptr <zw_value, zw_deleter> value
			    {zw_value_clone (&tos, pos, zw_throw_on_error {})};
			 ret.push_back (std::move (value));
		       });
      }
    catch (std::runtime_error const &e)
      {
	std::stringstream ss;
	ss << "When parsing argument `" << arg << "': "
	   << e.what ();
	throw std::runtime_error (ss.str ());
      }

    return ret;
  }

  std::unique_ptr <zw_value, zw_deleter>
  try_open_dwarf (const char *fn, size_t pos, bool no_messages)
  {
    try
      {
	std::unique_ptr <zw_value, zw_deleter> ret
	  {zw_value_init_dwarf (fn, pos, zw_throw_on_error {})};
	return ret;
      }
    catch (std::runtime_error const& e)
      {
	error_message (no_messages)
	  << "dwgrep: " << fn << ": " << e.what () << std::endl;
      }
    catch (...)
      {
	error_message (no_messages)
	  << "dwgrep: " << fn << ": Unknown error" << std::endl;
      }
    return nullptr;
  }
}

int
main(int argc, char *argv[])
try
  {
    setlocale (LC_ALL, "");
    textdomain ("dwgrep");

    std::unique_ptr <option[]> long_options = gen_options (ext_options);
    std::string options = gen_shopts (ext_options);

    int verbosity = 0;
    bool no_messages = false;
    bool show_count = false;
    bool with_header = false;
    bool no_header = false;

    std::unique_ptr <zw_vocabulary, zw_deleter> voc
	{zw_vocabulary_init (zw_throw_on_error {})};

    zw_vocabulary_add (voc.get (), zw_vocabulary_core (zw_throw_on_error {}),
		       zw_throw_on_error {});
    zw_vocabulary_add (voc.get (), zw_vocabulary_dwarf (zw_throw_on_error {}),
		       zw_throw_on_error {});
    zw_vocabulary_add (voc.get (), zw_vocabulary_elf (zw_throw_on_error {}),
		       zw_throw_on_error {});

    bool query_specified = false;
    std::string query_str;

    // Outer vector has one element per argument.
    // Inner vector has one element per value yielded by that argument expr.
    typedef std::vector <std::unique_ptr <zw_value, zw_deleter>> arg_val_vec_t;
    std::vector <arg_val_vec_t> args;

    while (true)
      {
	int c = getopt_long (argc, argv, options.c_str (),
			     long_options.get (), nullptr);
	if (c == -1)
	  break;

	switch (c)
	  {
	  case 'e':
	    query_str += optarg;
	    query_specified = true;
	    break;

	  case 'c':
	    show_count = true;
	    break;

	  case 'H':
	    with_header = true;
	    break;

	  case 'h':
	    no_header = true;
	    break;

	  case 'q':
	    verbosity = -1;
	    break;

	  case 's':
	    no_messages = true;
	    break;

	  case 'f':
	    {
	      auto buf_to_string = [] (std::istream &is)
		{
		  return std::string {std::istreambuf_iterator <char> {is},
				      std::istreambuf_iterator <char> {}};
		};

	      if (strcmp (optarg, "-") != 0)
		{
		  std::ifstream ifs {optarg};
		  if (ifs.fail ())
		    {
		      std::cerr << "Error: can't open script file `"
				<< optarg << "'.\n";
		      return 2;
		    }
		  query_str += buf_to_string (ifs);
		}
	      else
		query_str += buf_to_string (std::cin);
	      query_specified = true;
	      break;
	    }

          case 'a':
	    args.push_back (parse_arg_literal (optarg));
	    break;

	  default:
	    if (c == help)
	      {
		show_help (ext_options);
		return 0;
	      }
	    else if (c == version)
	      {
		std::cout << "dwgrep "
			  << DWGREP_VERSION_MAJOR << "."
			  << DWGREP_VERSION_MINOR << std::endl;
		return 0;
	      }
            else if (c == longarg)
              {
                args.push_back (parse_arg_eval (*voc, optarg));
		break;
              }

	    return 2;
	  }
      }

    argc -= optind;
    argv += optind;

    std::unique_ptr <zw_query, zw_deleter> query {
	[&] ()
	  {
	    if (query_specified)
	      return zw_query_parse_len (voc.get (), query_str.c_str (),
					 query_str.length (),
					 zw_throw_on_error {});
	    else
	      {
		if (argc == 0)
		  throw std::runtime_error ("No query specified.");

		argc--;
		return zw_query_parse (&*voc, *argv++, zw_throw_on_error {});
	      }
	  } ()};

    std::vector <std::string> file_args;
    if (argc > 0)
      {
	std::vector <std::unique_ptr <zw_value, zw_deleter>> dwvs;
	for (int i = 0; i < argc; ++i)
	  if (std::unique_ptr <zw_value, zw_deleter> dwv
	      = try_open_dwarf (argv[i], dwvs.size (), no_messages))
	    {
	      dwvs.emplace_back (std::move (dwv));
	      file_args.push_back (argv[i]);
	    }

	// Done before we started.
	if (dwvs.empty ())
	  return 1;

	args.emplace (args.begin (), std::move (dwvs));
      }

    size_t iterations = 1;
    for (auto const &arg: args)
      iterations *= arg.size ();

    if (iterations > 1)
      with_header = true;
    if (no_header)
      with_header = false;

    std::vector <arg_val_vec_t::const_iterator> arg_its;
    for (auto const &arg: args)
      arg_its.push_back (arg.begin ());

    bool errors = false;
    bool match = false;
    while (true)
      {
	std::unique_ptr <zw_stack, zw_deleter> stack
	    {zw_stack_init (zw_throw_on_error {})};

	for (auto const &arg_it: arg_its)
	  {
	    zw_value const &cur = *arg_it->get ();
	    size_t pos = zw_value_pos (&cur);
	    std::unique_ptr <zw_value, zw_deleter> value
		{zw_value_clone (&cur, pos, zw_throw_on_error {})};
	    zw_stack_push_take (stack.get (), value.release (),
				zw_throw_on_error {});
	  }

	dumper dump {*voc};

	std::string header = [&] ()
	  {
	    std::stringstream ss;
	    bool seen = false;
	    for (size_t i = 0; i < args.size (); ++i)
	      {
		zw_value const &cur = *arg_its[i]->get ();

		// Always show the first argument if it refers to a file name
		// given on the command line.
		if ((i == 0 && file_args.size () > 0) || args[i].size () > 1)
		  {
		    if (seen)
		      ss << ',';
		    dump.dump_value (ss, cur, dumper::format::header);
		    seen = true;
		  }
	      }
	    if (! seen)
	      ss << "<no-file>";
	    return ss.str ();
	  } ();

	try
	  {
	    std::unique_ptr <zw_result, zw_deleter> result
		{zw_query_execute (query.get (), stack.get (),
				   zw_throw_on_error {})};

	    uint64_t count = 0;
	    while (auto out = zw_result_next (*result))
	      {
		// grep: Exit immediately with zero status if any match
		// is found, even if an error was detected.
		if (verbosity < 0)
		  return 0;

		zw_stack &stk = *out.get ();
		match = true;
		if (! show_count)
		  {
		    if (with_header)
		      std::cout << header << ":\n";
		    if (zw_stack_depth (&stk) > 1)
		      std::cout << "---\n";
		    for (size_t i = 0, n = zw_stack_depth (&stk);
			 i < n; ++i)
		      {
			auto const *val = zw_stack_at (&stk, i);
			assert (val != nullptr);
			dump.dump_value (std::cout, *val,
					 dumper::format::full);
			std::cout << std::endl;
		      }
		  }
		else
		  ++count;
	      }

	    if (show_count)
	      {
		if (with_header)
		  std::cout << header << ":";
		std::cout << std::dec << count << std::endl;
	      }
	  }
	catch (std::runtime_error const &e)
	  {
	    error_message (no_messages, verbosity, errors)
	      << "dwgrep: " << header << ": " << e.what () << std::endl;
	  }
	catch (...)
	  {
	    error_message (no_messages, verbosity, errors)
	      << "dwgrep: " << header << ": Unknown error" << std::endl;
	  }

	// Bump argument list.
	bool next = false;
	for (size_t ri = 0; ri < args.size (); ++ri)
	  {
	    size_t i = args.size () - 1 - ri;
	    if (++arg_its[i] == args[i].end ())
	      arg_its[i] = args[i].begin ();
	    else
	      {
		next = true;
		break;
	      }
	  }
	if (! next)
	  break;
      }

    if (errors)
	return 2;

    return match ? 0 : 1;
  }
catch (std::runtime_error const& e)
  {
    std::cerr << "dwgrep: " << e.what () << std::endl;
    return 2;
  }
catch (...)
  {
    std::cerr << "dwgrep: Unknown error" << std::endl;
    return 2;
  }
