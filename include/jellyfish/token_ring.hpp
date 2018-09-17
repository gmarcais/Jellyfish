/*  This file is part of Jellyfish.

    This work is dual-licensed under 3-Clause BSD License or GPL 3.0.
    You can choose between one of them if you use this work.

`SPDX-License-Identifier: BSD-3-Clause OR  GPL-3.0`
*/

#ifndef __JELLYFISH_TOKEN_RING_HPP__
#define __JELLYFISH_TOKEN_RING_HPP__

#include <vector>
#include <jellyfish/locks_pthread.hpp>

namespace jellyfish {
template<class cond_t = locks::pthread::cond>
class token_ring {
public:
  class token {
    bool   val;
    cond_t cond;
    token* next;
    friend class token_ring;

  public:
    void wait() {
      cond.lock();
      while(!val) { cond.wait(); }
      cond.unlock();
    }

    void pass() {
      next->cond.lock();
      val       = false;
      next->val = true;
      next->cond.signal();
      next->cond.unlock();
    }
  };

protected:
  typedef std::vector<token>            token_list;
  typedef typename token_list::iterator token_iterator;
  token_list tokens;

  void initialize() {
    if(tokens.size() == 0)
      return;

    tokens.front().val = true;
    tokens.back().next = &tokens.front();

    for(size_t i = 1; i < tokens.size(); ++i) {
      tokens[i].val    = false;
      tokens[i-1].next = &tokens[i];
    }
  }

public:
  token_ring(int nb_tokens) :
    tokens(nb_tokens)
  { initialize(); }

  ~token_ring() { }

  token& operator[](int i) { return tokens[i]; }

  void reset() {
    if(tokens.size() == 0)
      return;

    token_iterator it = tokens.begin();
    it->val = true;
    for(++it; it != tokens.end(); ++it)
      it->val = false;
  }
};
} // namespace jellyfish {
#endif
