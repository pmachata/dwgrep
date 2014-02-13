#include "wset.hh"
#include "dwit.hh"

wset
wset::initial (std::shared_ptr <Dwarf> &dw)
{
  wset ws;
  for (all_dies_iterator it {&*dw}; it != all_dies_iterator::end (); ++it)
    {
      Dwarf_Die *die = *it;
      auto val = std::unique_ptr <subgraph> (new subgraph (dw));
      val->add (dieref (die));
      ws.add (std::move (val));
    }
  return ws;
}

wset::wset () {}

void
wset::add (std::unique_ptr <value> &&val)
{
  m_values.emplace_back (std::move (val));
}

size_t
wset::size () const
{
  return m_values.size ();
}

std::vector <std::unique_ptr <value> >::iterator
wset::begin ()
{
  return std::begin (m_values);
}

std::vector <std::unique_ptr <value> >::iterator
wset::end ()
{
  return std::end (m_values);
}

std::vector <std::unique_ptr <value> >::const_iterator
wset::cbegin () const
{
  return m_values.cbegin ();
}

std::vector <std::unique_ptr <value> >::const_iterator
wset::cend () const
{
  return m_values.cend ();
}
