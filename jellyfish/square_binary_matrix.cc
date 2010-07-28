#include "square_binary_matrix.hpp"
#include <iostream>
#include <sstream>

SquareBinaryMatrix::SingularMatrix singular_matrix_ex;
SquareBinaryMatrix::MismatchingSize mismatching_size_ex;

void SquareBinaryMatrix::init_random() {
  uint64_t _mask = mask();
  int i, j;
  int lrm = floorLog2((unsigned long)RAND_MAX);
  for(i = 0; i < size; i++) {
    for(j = 0; j < size; j += lrm)
      columns[i] ^= (uint64_t)random() << j;
    columns[i] &= _mask;
  }
}

SquareBinaryMatrix SquareBinaryMatrix::init_random_inverse() {
  while(true) {
    init_random();
    try {
      return inverse();
    } catch(SquareBinaryMatrix::SingularMatrix e) { }
  }
}

SquareBinaryMatrix SquareBinaryMatrix::transpose() const {
  SquareBinaryMatrix res(size);
  uint64_t *a = res.columns, *b;
  int i, j;

  for(i = size - 1; i >= 0; i--, a++) 
    for(j = size - 1, b = columns; j >= 0; j--, b++)
      *a |= ((*b >> i) & 0x1) << j;

  return res;
}

SquareBinaryMatrix SquareBinaryMatrix::operator*(const SquareBinaryMatrix &other) const {
  int i;
  SquareBinaryMatrix res(size);

  if(size != other.get_size()) 
    throw mismatching_size_ex;
  
  for(i = 0; i < size; i++)
    res[i] = this->times(other[i]);

  return res;
}

SquareBinaryMatrix SquareBinaryMatrix::inverse() const {
  SquareBinaryMatrix pivot(*this);
  SquareBinaryMatrix res(size);
  int i, j;

  res.init_identity();

  // forward elimination
  for(i = 0; i < size; i++) {
    if(!((pivot.columns[i] >> (size - i - 1)) & (uint64_t)0x1)) {
      for(j = i + 1; j < size; j++)
        if((pivot.columns[j] >> (size - i - 1)) & (uint64_t)0x1)
          break;
      if(j >= size)
	throw singular_matrix_ex;
      pivot.columns[i] ^= pivot.columns[j];
      res.columns[i] ^= res.columns[j];
    }

    for(j = i+1; j < size; j++) {
      if((pivot.columns[j] >> (size - i - 1)) & (uint64_t)0x1) {
	pivot.columns[j] ^= pivot.columns[i];
	res.columns[j] ^= res.columns[i];
      }
    }
  }

  // backward elimination
  for(i = size - 1; i > 0; i--) {
    for(j = i - 1; j >= 0; j--) {
      if((pivot.columns[j] >> (size - i - 1)) & (uint64_t)0x1) {
	pivot.columns[j] ^= pivot.columns[i];
	res.columns[j] ^= res.columns[i];
      }
    }
  }
  return res;
}

void SquareBinaryMatrix::print(std::ostream *os) const {
  int i, j;
  uint64_t a, *v;

  *os << size << "x" << size << std::endl;
  for(i = 0, a = msb(); i < size; i++, a >>= 1) {
    for(j = 0, v = columns; j < size; j++, v++)
      *os << ((*v & a) ? 1 : 0);
    *os << std::endl;
  }
}

std::string SquareBinaryMatrix::str() const {
  std::ostringstream os;
  print(&os);
  return os.str();
}

void SquareBinaryMatrix::dump(std::ostream *os) const {
  os->write((char *)&size, sizeof(size));
  os->write((char *)columns, sizeof(uint64_t) * size);
}

void SquareBinaryMatrix::load(std::istream *is) {
  if(columns) {
    delete[] columns;
    columns = NULL;
  }
  is->read((char *)&size, sizeof(size));
  columns = new uint64_t[size];
  is->read((char *)columns, sizeof(uint64_t) * size);
}

size_t SquareBinaryMatrix::read(const char *map) {
  if(columns) {
    delete[] columns;
    columns = NULL;
  }
  memcpy(&size, map, sizeof(size));
  columns = new uint64_t[size];
  memcpy(columns, map + sizeof(size), sizeof(uint64_t) * size);
  return sizeof(size) + sizeof(uint64_t) * size;
}

void SquareBinaryMatrix::print_vector(std::ostream *os, uint64_t v, bool vertical) const {
  uint64_t a;
  for(a = msb(); a; a >>= 1)
    *os << ((v & a) ? 1 : 0) << (vertical ? "\n" : "");
}

std::string SquareBinaryMatrix::str_vector(uint64_t v, bool vertical) const {
  std::ostringstream os;
  print_vector(&os, v, vertical);
  return os.str();
}
