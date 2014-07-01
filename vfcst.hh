#ifndef _VFCST_H_
#define _VFCST_H_

#include "constant.hh"

// Named constants for dwgrep "type" operator.
enum class slot_type_id
  {
    T_CLOSURE,
    T_CONST,
    T_FLOAT,
    T_STR,
    T_SEQ,
    T_NODE,
    T_ATTR,
  };

// A domain for slot type constants.
extern constant_dom const &slot_type_dom;
extern constant_dom const &pos_dom;

#endif /* _VFCST_H_ */
