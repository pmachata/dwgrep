#include <sstream>
#include <cassert>
#include <limits>

#include <boost/format.hpp>

#include "value.hh"
#include "dwpp.hh"

v_die::v_die (Dwarf_Off offset)
  : m_offset (offset)
{}

v_die::v_die (Dwarf_Die *die)
  : m_offset (dwarf_dieoffset (die))
{}

Dwarf_Die
v_die::die (std::shared_ptr <Dwarf> &dw) const
{
  return dwpp_offdie (&*dw, m_offset);
}

Dwarf_Off
v_die::offset () const
{
  return m_offset;
}

std::string
v_die::format () const
{
  std::ostringstream ss;
  ss << "[" << std::hex << m_offset << "]";
  return ss.str ();
}

std::unique_ptr <value>
v_die::clone () const
{
  return std::unique_ptr <value> (new v_die { m_offset });
}

pred_result
v_die::lt (value const &that) const
{
  if (auto at2 = dynamic_cast <v_die const *> (&that))
    return pred_result (m_offset < at2->m_offset);
  else
    return pred_result::fail;
}


v_at::v_at (Dwarf_Off offset, int atname)
  : m_dieoffset (offset)
  , m_atname (atname)
{}

int
v_at::atname () const
{
  return m_atname;
}

std::string
v_at::format () const
{
  std::ostringstream ss;
  ss << "[at:" << m_atname << "@" << std::hex << m_dieoffset << "]";
  return ss.str ();
}

std::unique_ptr <value>
v_at::clone () const
{
  return std::unique_ptr <value> (new v_at { m_dieoffset, m_atname });
}

pred_result
v_at::lt (value const &that) const
{
  std::cerr << "Comparison of v_at values is not supported\n";
  return pred_result::fail;
}


std::string
v_sint::format () const
{
  return str (boost::format ("%d") % m_val);
}

std::unique_ptr <value>
v_sint::clone () const
{
  return std::unique_ptr <value> { new v_sint { m_val } };
}

pred_result
v_sint::lt (value const &that) const
{
  if (auto v = dynamic_cast <v_sint const *> (&that))
    return pred_result (m_val < v->m_val);
  else if (dynamic_cast <v_uint const *> (&that))
    {
      if (m_val < 0)
	return pred_result::yes;
      v_uint v2 { uint64_t (m_val) };
      return v2.lt (that);
    }
  else
    return pred_result::fail;
}


std::string
v_uint::format () const
{
  return str (boost::format ("%u") % m_val);
}

std::unique_ptr <value>
v_uint::clone () const
{
  return std::unique_ptr <value> { new v_uint { m_val } };
}

pred_result
v_uint::lt (value const &that) const
{
  if (auto v = dynamic_cast <v_uint const *> (&that))
    return pred_result (m_val < v->m_val);
  else if (dynamic_cast <v_sint const *> (&that))
    {
      if (m_val > std::numeric_limits <int64_t>::max ())
	return pred_result::yes;
      v_sint v2 { int64_t (m_val) };
      return v2.lt (that);
    }
  else
    return pred_result::fail;
}


std::string
v_flt::format () const
{
  return str (boost::format ("%f") % m_val);
}


std::string
v_str::format () const
{
  return m_str;
}

std::unique_ptr <value>
v_str::clone () const
{
  return std::unique_ptr <value> { new v_str { m_str.c_str () } };
}

pred_result
v_str::lt (value const &that) const
{
  if (auto s2 = dynamic_cast <v_str const *> (&that))
    return pred_result (m_str < s2->m_str);
  else
    return pred_result::fail;
}


std::string
v_seq::format () const
{
  std::stringstream ss;
  ss << "(";
  for (auto const &val: m_seq)
    ss << val->format () << ", ";
  ss << ")";
  return ss.str ();
}
