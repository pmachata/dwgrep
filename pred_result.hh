#ifndef _PRED_RESULT_H_
#define _PRED_RESULT_H_

enum class pred_result
  {
    no = false,
    yes = true,
    fail,
  };

inline pred_result
operator! (pred_result other)
{
  switch (other)
    {
    case pred_result::no:
      return pred_result::yes;
    case pred_result::yes:
      return pred_result::no;
    case pred_result::fail:
      return pred_result::fail;
    }
  abort ();
}

inline pred_result
operator&& (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) && bool (b) ? pred_result::yes : pred_result::no;
}

inline pred_result
operator|| (pred_result a, pred_result b)
{
  if (a == pred_result::fail || b == pred_result::fail)
    return pred_result::fail;
  else
    return bool (a) || bool (b) ? pred_result::yes : pred_result::no;
}

#endif /* _PRED_RESULT_H_ */
