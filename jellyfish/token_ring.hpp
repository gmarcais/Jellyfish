/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __JELLYFISH_TOKEN_RING_HPP__
#define __JELLYFISH_TOKEN_RING_HPP__

template<typename cond_t>
class token_ring {
public:
  class token {
    token  *next;
    bool    val;
    cond_t  cond;

    token(token *_next, bool _val) :
      next(_next), val(_val) {}
    friend class token_ring;

  public:
    bool is_active() { return val; }
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

private:
  token *first, *last;
  cond_t cond;

public:
  token_ring() : 
    first(0), last(0)
  { }

  ~token_ring() {
    if(!first)
      return;

    while(first != last) {
      token *del = first;
      first = first->next;
      delete del;
    }
    delete last;
  }

  void reset() {
    if(!first)
      return;

    token *c = first;
    while(c != last) {
      c->val = false;
      c = c->next;
    }
    last->val = false;
    first->val = true;
  }


  token *new_token() { 
    token *nt = new token(first, first == 0);
    if(first) {
      last->next = nt;
      last = nt;
    } else {
      first = last = nt;
      nt->next = nt;
    }
    return nt;
  }
};

#endif
