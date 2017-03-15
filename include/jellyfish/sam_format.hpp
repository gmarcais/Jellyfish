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

#ifndef __JELYFISH_SAM_FORMAT_H__
#define __JELYFISH_SAM_FORMAT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_HTSLIB
#include <htslib/sam.h>

namespace jellyfish {

// Manage a sam_stream
class sam_wrapper {
protected:
  htsFile*   m_fd;
  bam_hdr_t* m_header;
  bam1_t*    m_record;
  uint8_t*   m_seq;
  uint8_t*   m_qual;

public:
  sam_wrapper()
    : m_fd(nullptr)
    , m_header(nullptr)
    , m_record(nullptr)
  { }

  sam_wrapper(const char* path)
    : m_fd(sam_open(path, "r"))
    , m_header(m_fd ? sam_hdr_read(m_fd) : nullptr)
    , m_record(m_header ? bam_init1() : nullptr)
  { }

  // True if stream is good
  bool good() {
    return m_fd && m_header && m_record;
  }

  // Read next record. Return >= 0 if successful. <0 otherwise.
  int next() {
    const auto res = sam_read1(m_fd, m_header, m_record);
    if(res >= 0) {
      m_seq  = bam_get_seq(m_record);
      m_qual = bam_get_qual(m_record);
    }
    return res;
  }

  // Query the content of the record. Valid only after a successful
  // next()
  const bam1_t* record() { return m_record; }
  ssize_t seq_len() const { return m_record->core.l_qseq; }
  static constexpr char decode[16] = {'N', 'A', 'C', 'N', 'G', 'N', 'N', 'N', 'T',
                                      'N', 'N', 'N', 'N', 'N', 'N', 'N' };
  char base(ssize_t i) const { return decode[bam_seqi(m_seq, i)]; }
  char qual(ssize_t i) const { return m_qual[i]; }


  ~sam_wrapper() {
    if(m_record) bam_destroy1(m_record);
    if(m_header) bam_hdr_destroy(m_header);
    if(m_fd) sam_close(m_fd);
  }
};

#else
// Empty type
struct sam_wrapper {};
#endif

}  // jellyfish

#endif // __JELYFISH_SAM_FORMAT_H__
