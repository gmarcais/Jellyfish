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

// Simple growing array. Never shrinks. No construction or destruction
// of objects in array, only copy with pushback() methods
namespace jellyfish {
  template<typename T>
  class simple_growing_array {
    size_t  _capacity;
    size_t  _size;
    T      *_data;
  public:
    explicit simple_growing_array(size_t capacity = 100) : 
      _capacity(capacity / 2), _size(0), _data(0) { resize(); }

    virtual ~simple_growing_array() {
      free(_data);
    }

    void push_back(const T &x) {
      if(_size >= _capacity)
        resize();
      _data[_size++] = x;
    }

    void reset() { _size = 0; }

    size_t size() const { return _size; }
    bool empty() const { return _size == 0; }
    const T * begin() const { return _data; }
    const T * end() const { return _data + _size; }
    
  private:
    define_error_class(SimpleGrowingArrayError);
    void resize() {
      _capacity *= 2;
      void * ndata = realloc(_data, sizeof(T) * _capacity);
      if(ndata == 0) {
        free(ndata);
        _data = 0;
        _capacity = _capacity / 2;
        eraise(SimpleGrowingArrayError) << "Out of memory" << err::no;
      }
      _data = (T*)ndata;
    }
  };
}
