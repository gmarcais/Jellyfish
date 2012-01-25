/* Copyright (c) 2011 Guillaume Marcais
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <jellyfish/yaggo.hpp>

namespace yaggo {
  uint32_t string::as_uint32(bool si_suffix) const {
    std::string err;
    uint32_t res = yaggo::conv_uint<uint32_t>((const char *)this->c_str(), err, si_suffix);
    if(!err.empty()) {
      std::string msg("Invalid conversion of '");
      msg += *this;
      msg += "' to uint32_t: ";
      msg += err;
      throw std::runtime_error(msg);
    }
    return res;
  }

  uint64_t string::as_uint64(bool si_suffix) const {
    std::string err;
    uint64_t res = yaggo::conv_uint<uint64_t>((const char *)this->c_str(), err, si_suffix);
    if(!err.empty()) {
      std::string msg("Invalid conversion of '");
      msg += *this;
      msg += "' to uint64_t: ";
      msg += err;
      throw std::runtime_error(msg);
    }
    return res;
  }

  int32_t string::as_int32(bool si_suffix) const {
    std::string err;
    int32_t res = yaggo::conv_int<int32_t>((const char *)this->c_str(), err, si_suffix);
    if(!err.empty()) {
      std::string msg("Invalid conversion of '");
      msg += *this;
      msg += "' to int32_t: ";
      msg += err;
      throw std::runtime_error(msg);
    }
    return res;
  }

  int64_t string::as_int64(bool si_suffix) const {
    std::string err;
    int64_t res = yaggo::conv_int<int64_t>((const char *)this->c_str(), err, si_suffix);
    if(!err.empty()) {
      std::string msg("Invalid conversion of '");
      msg += *this;
      msg += "' to int64_t: ";
      msg += err;
      throw std::runtime_error(msg);
    }
    return res;
  }

  int string::as_int(bool si_suffix) const {
    std::string err;
    int res = yaggo::conv_int<int>((const char *)this->c_str(), err, si_suffix);
    if(!err.empty()) {
      std::string msg("Invalid conversion of '");
      msg += *this;
      msg += "' to int_t: ";
      msg += err;
      throw std::runtime_error(msg);
    }
    return res;
  }

  long string::as_long(bool si_suffix) const {
    std::string err;
    long res = yaggo::conv_int<long>((const char *)this->c_str(), err, si_suffix);
    if(!err.empty()) {
      std::string msg("Invalid conversion of '");
      msg += *this;
      msg += "' to long_t: ";
      msg += err;
      throw std::runtime_error(msg);
    }
    return res;
  }

  double string::as_double(bool si_suffix) const {
    std::string err;
    double res = yaggo::conv_double((const char *)this->c_str(), err, si_suffix);
    if(!err.empty()) {
      std::string msg("Invalid conversion of '");
      msg += *this;
      msg += "' to double_t: ";
      msg += err;
      throw std::runtime_error(msg);
    }
    return res;
  }

  bool adjust_double_si_suffix(double &res, const char *suffix) {
    if(*suffix == '\0')
      return true;
    if(*(suffix + 1) != '\0')
      return false;

    switch(*suffix) {
    case 'a': res *= 1e-18; break;
    case 'f': res *= 1e-15; break;
    case 'p': res *= 1e-12; break;
    case 'n': res *= 1e-9;  break;
    case 'u': res *= 1e-6;  break;
    case 'm': res *= 1e-3;  break;
    case 'k': res *= 1e3;   break;
    case 'M': res *= 1e6;   break;
    case 'G': res *= 1e9;   break;
    case 'T': res *= 1e12;  break;
    case 'P': res *= 1e15;  break;
    case 'E': res *= 1e18;  break;
    default: return false;
    }
    return true;
  }

  double conv_double(const char *str, std::string &err, bool si_suffix) {
    char *endptr = 0;
    errno = 0;
    double res = strtod(str, &endptr);
    if(errno) {
      err.assign(strerror(errno));
      return (double)0.0;
    }
    bool invalid =
      si_suffix ? !adjust_double_si_suffix(res, endptr) : *endptr != '\0';
    if(invalid) {
      err.assign("Invalid character");
      return (double)0.0;
    }
    return res;
  }
}
