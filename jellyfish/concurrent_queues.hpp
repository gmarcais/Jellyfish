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

#ifndef __JELLYFISH_CONCURRENT_QUEUES_HPP__
#define __JELLYFISH_CONCURRENT_QUEUES_HPP__

#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
using namespace std;
#include <new>
#include <stdio.h>

/* Concurrent queue. No check whether enqueue would overflow the queue.
 */

template<class Val>
class concurrent_queue {
  Val                   **queue;
  unsigned int          size;
  unsigned int          volatile head;
  unsigned int          volatile tail;
  bool                  volatile closed;
  
public:
  concurrent_queue(unsigned int _size) : size(_size + 1), head(0), tail(0) { 
    queue = new Val *[size];
    memset(queue, 0, sizeof(Val *) * size);
    closed = false;
  }
  ~concurrent_queue() { delete [] queue; }

  void enqueue(Val *v);
  Val *dequeue();
  bool is_closed() { return closed; }
  void close() { closed = true; }
  bool has_space() { return head != tail; }
  bool is_low() { 
    int len = head - tail;
    if(len < 0)
      len += size;
    return (unsigned int)(4*len) <= size;
  }
};

template<class Val>
void concurrent_queue<Val>::enqueue(Val *v) {
  int done = 0;

  while(!done) {
    unsigned int chead = head;
    unsigned int nhead = (chead + 1) % size;

    done = __sync_bool_compare_and_swap(&queue[chead], 0, v);
    __sync_bool_compare_and_swap(&head, chead, nhead);
  }
}

template<class Val>
Val *concurrent_queue<Val>::dequeue() {
  int done = 0;
  Val *res;
  unsigned int chead, ctail, ntail;

  while(!done) {
    ctail = tail;
    chead = head;

    if(ctail != chead) {
      ntail = (ctail + 1) % size;
      done = __sync_bool_compare_and_swap(&tail, ctail, ntail);
    } else {
      return NULL;
    }
  }
  res = queue[ctail];
  queue[ctail] = NULL;
  return res;
}
  
#endif
