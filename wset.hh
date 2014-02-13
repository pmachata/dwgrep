#ifndef _WSET_H_
#define _WSET_H_

#include <vector>
#include <memory>
#include <libdw.h>

#include "value.hh"

class wset
{
  std::vector <std::unique_ptr <value> > m_values;

public:
  static wset initial (std::shared_ptr <Dwarf> &dw);

  wset ();

  void add (std::unique_ptr <value> &&val);
  size_t size () const;

  std::vector <std::unique_ptr <value> >::iterator begin ();
  std::vector <std::unique_ptr <value> >::iterator end ();

  std::vector <std::unique_ptr <value> >::const_iterator cbegin () const;
  std::vector <std::unique_ptr <value> >::const_iterator cend () const;
};

#endif /* _WSET_H_ */
