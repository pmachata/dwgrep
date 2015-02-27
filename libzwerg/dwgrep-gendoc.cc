/*
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
#include <algorithm>
#include <dlfcn.h>
#include <iomanip>
#include <iterator>
#include <sstream>

#include "libzwergP.hh"
#include "libzwerg.hh"
#include "builtin.hh"
#include "init.hh"
#include "builtin-dw.hh"
#include "docstring.hh"
#include "flag_saver.hh"

static std::map <std::string, std::vector <std::string>>
split_pfx_recursively (std::map <std::string, std::vector <std::string>> d,
		       size_t sz)
{
  std::map <std::string, std::vector <std::string>> keep;
  std::map <std::string, std::vector <std::string>> nd;

  while (! d.empty ())
    {
      auto emt = std::move (*d.begin ());
      d.erase (d.begin ());

      std::map <std::string, std::vector <std::string>> sd;
      size_t nn = emt.first.length () + 1;
      for (auto &w: emt.second)
	sd[w.substr (0, nn)].push_back (std::move (w));

      // The division is pointless unless it makes sense for all
      // resulting groups.  Otherwise we'd end up producing splits
      // like DW_OP_* and DW_OP_GNU_*, where the former covers the
      // latter.  Reject such divisions.
      if (! std::all_of (sd.begin (), sd.end (),
			 [sz] (std::pair <std::string,
					  std::vector <std::string>> const &p)
			 {
			   return p.second.size () > sz / 10
			     && p.second.size () > 3;
			 }))
	{
	  std::vector <std::string> rest;
	  while (! sd.empty ())
	    {
	      auto semt = std::move (*sd.begin ());
	      sd.erase (sd.begin ());
	      rest.insert (rest.end (),
			   std::make_move_iterator (semt.second.begin ()),
			   std::make_move_iterator (semt.second.end ()));
	    }

	  assert (! rest.empty ());
	  assert (keep.find (emt.first) == keep.end ());
	  keep[std::move (emt.first)] = std::move (rest);
	}

      else
	while (! sd.empty ())
	  {
	    auto semt = std::move (*sd.begin ());
	    sd.erase (sd.begin ());

	    assert (nd.find (semt.first) == nd.end ());
	    nd[std::move (semt.first)] = std::move (semt.second);
	  }
    }

#if 0
  for (auto const &e: nd)
    std::cout << "rc " << e.first << " " << e.second.size () << std::endl;
#endif

  if (! nd.empty ())
    nd = split_pfx_recursively (std::move (nd), sz);

  nd.insert (std::make_move_iterator (keep.begin ()),
	     std::make_move_iterator (keep.end ()));

  return std::move (nd);
}

static std::vector <std::string>
split_pfx (std::vector <std::string> list)
{
  size_t sz = list.size ();
  auto split = split_pfx_recursively ({{"", std::move (list)}}, sz);
  std::vector <std::string> ret;
  for (auto &emt: split)
    {
      std::string h = std::move (emt.first);
      h += "*";
      ret.push_back (std::move (h));
    }
  return std::move (ret);
}

static std::vector <std::pair <std::vector <std::string>, std::string>>
deduplicate (std::vector <std::pair <std::string, std::string>> entries)
{
  entries.erase
    (std::remove_if (entries.begin (), entries.end (),
		     [] (std::pair <std::string, std::string> const &p)
		     {
		       return p.second == "@hide";
		     }),
     entries.end ());

  auto dentries = doc_deduplicate (entries);

  // If the de-duplication found too many duplicates and there are
  // overlong headings, try extract commonalities and describe the
  // full list in body.
  for (auto &dentry: dentries)
    if (dentry.first.size () > 3)
      {
	std::vector <std::string> nh = split_pfx (dentry.first);
	assert (nh.size () > 0);
	if (nh[0] == "*")
	  continue;

	std::stringstream ss;
	ss << "This section describes the following words: ";
	bool seen = false;
	for (auto &e: dentry.first)
	  {
	    ss << (seen ? ", ``" : "``") << std::move (e) << "``";
	    seen = true;
	  }
	ss << ".\n\n" << std::move (dentry.second);
	dentry.first = std::move (nh);
	dentry.second = ss.str ();
      }

  for (auto &dentry: dentries)
    std::sort (dentry.first.begin (), dentry.first.end ());

  std::sort (dentries.begin (), dentries.end (),
	     [] (std::pair <std::vector <std::string>, std::string> const &p1,
		 std::pair <std::vector <std::string>, std::string> const &p2)
	     {
	       return p1.first < p2.first;
	     });

  return dentries;
}

int
main (int argc, char const **argv)
{
  if (argc != 3)
    {
      std::cerr << "usage: " << argv[0] << " <vocabulary handle> <title>\n";
      return -1;
    }

  std::string handle = argv[1];
  std::string title = argv[2];

  void *sym = dlsym (RTLD_DEFAULT, handle.c_str ());
  if (sym == nullptr)
    {
      std::cerr << "Couldn't find a vocabulary with handle " << handle << "\n"
		<< dlerror () << std::endl;
      return -1;
    }

  {
    ios_flag_saver s {std::cout};
    std::cout << ".. _" << handle << ":\n\n"
	      << title << "\n"
	      << std::setw (title.length ()) << std::setfill ('=') << '='
	      << "\n\n";
  }

  std::cout << ".. include:: " << handle << "-blurb.txt\n\n";

  std::vector <std::pair <std::string, std::string>> types;

  {
    std::vector <std::pair <uint8_t, char const *>> tnames
      = value_type::get_names ();
    std::vector <std::pair <uint8_t, std::string>> docstrings
      = value_type::get_docstrings ();

    assert (tnames.size () == docstrings.size ());

    std::transform (tnames.begin (), tnames.end (), docstrings.begin (),
		    std::back_inserter (types),
		    [&] (std::pair <uint8_t, char const *> tn,
			 std::pair <uint8_t, std::string> const &ds)
		    {
		      assert (tn.first == ds.first);
		      return std::make_pair (tn.second, ds.second);
		    });
  }

  std::vector <std::pair <std::string, std::string>> entries;

  auto bis = [&] ()
    {
      return reinterpret_cast <decltype (&zw_vocabulary_core)> (sym)
		(zw_throw_on_error {})->m_voc->get_builtins ();
    } ();

  std::transform (bis.begin (), bis.end (), std::back_inserter (entries),
		  [] (std::pair <std::string,
				 std::shared_ptr <builtin const>> const &bi)
		  {
		    return std::make_pair (bi.first, bi.second->docstring ());
		  });

  // Drop non-local types, i.e. drop those that are _not_ present
  // among ENTRIES.  The reason is that ENTRIES include only those
  // type words (T_CONST etc.) that are actually defined in this
  // module.
  types.erase
    (std::remove_if (types.begin (), types.end (),
		     [&] (std::pair <std::string, std::string> const &t)
		     {
		       return std::find_if
			 (entries.begin (), entries.end (),
			  [&] (std::pair <std::string, std::string> const &e)
			  {
			    return e.first == t.first;
			  }) == entries.end ();
		     }),
     types.end ());

  // Drop entries that are documented as types, i.e. drop those that
  // _are_ present among TYPES.
  entries.erase
    (std::remove_if (entries.begin (), entries.end (),
		     [&] (std::pair <std::string, std::string> const &t)
		     {
		       return std::find_if
			 (types.begin (), types.end (),
			  [&] (std::pair <std::string, std::string> const &e)
			  {
			    return e.first == t.first;
			  }) != types.end ();
		     }),
     entries.end ());

  auto dd_entries = deduplicate (entries);

  // Find words applicable to individual types.
  for (auto &t: types)
    if (t.second != "@hide")
      {
	std::vector <std::string> applicable;

	for (auto const &bi: bis)
	  if (auto *obi = dynamic_cast <overloaded_builtin const *>
				(bi.second.get ()))
	    for (auto const &ovld: obi->get_overload_tab ()->get_overloads ())
	      {
		auto const &types = std::get <0> (ovld).get_types ();
		if (std::any_of (types.begin (), types.end (),
				 [&t] (value_type const &vt) {
				   return vt.name () == t.first;
				 }))
		  {
		    std::string word = bi.first;

		    // Look if it's covered by any of the word groups
		    // (e.g. ?DW_AT_*).
		    for (auto const &g: dd_entries)
		      for (auto const &h: g.first)
			if (! h.empty () && *h.rbegin () == '*'
			    && h.compare (0, h.size () - 1,
					  word, 0, h.size () - 1) == 0)
			  {
			    word = h;
			    goto done;
			  }

		  done:
		    if (std::find (applicable.begin (), applicable.end (),
				   word) == applicable.end ())
		      applicable.push_back (word);
		    break;
		  }
	      }

	if (! applicable.empty ())
	  {
	    std::stringstream ss;
	    bool seen = false;
	    for (auto const &a: applicable)
	      {
		if (! seen)
		  ss << "\nApplicable words\n................\n";
		else
		  ss << ", ";

		ss << ":ref:`\\" << a << " <" << handle << " " << a << ">`";
		seen = true;
	      }

	    ss << "\n\n";
	    t.second += ss.str ();
	  }
      }

  std::cout << format_entry_map (deduplicate (types), '-', handle + " type");
  std::cout << format_entry_map (std::move (dd_entries), '-', handle);
}
