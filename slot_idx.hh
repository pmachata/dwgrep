#ifndef _SLOT_IDX_H_
#define _SLOT_IDX_H_

#include <iosfwd>

enum slot_idx: unsigned {};

std::ostream &operator<< (std::ostream &o, slot_idx idx);

#endif /* _SLOT_IDX_H_ */
