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

#include <jellyfish/fastq_parser.hpp>

namespace jellyfish {
  fastq_parser::fastq_parser(int nb_files, char *argv[], uint_t _mer_len,
                             unsigned int nb_buffers, const bool _qc) :
    mer_len(_mer_len), mapped_files(nb_files, argv, true),
    current_file(mapped_files.begin()),
    rq(nb_buffers), wq(nb_buffers), quality_control(_qc)
  {
    buffers = new seq[nb_buffers];
    memset(buffers, '\0', sizeof(struct seq) * nb_buffers);
    for(unsigned int i = 0; i < nb_buffers; i++)
      wq.enqueue(&buffers[i]);

    current_file->map();
    current_file->will_need();
    map_base = current_file->base();
    current = map_base;
    map_end = current_file->end();
  }

  bool fastq_parser::thread::next_sequence() {
    if(rq->is_low() && !rq->is_closed()) {
      parser->read_sequence();
    }
    while(!(sequence = rq->dequeue())) {
      if(rq->is_closed())
        return false;
      parser->read_sequence();
    }
    return true;
  }

  void fastq_parser::_read_sequence() {
    seq *new_seq;
    int nb_reads;

    // New lines are not allowed in sequence or quals
#define BREAK_AT_END if(current >= map_end) break;
    while((new_seq = wq.dequeue()) != 0) {
      nb_reads = 0;
      while(nb_reads < max_nb_reads && current < map_end) {
        read *cread = &new_seq->reads[nb_reads];

        while(true) {
          // Find & skip header
          while(*current++ != '@')
            BREAK_AT_END;
          BREAK_AT_END;
          while(*current++ != '\n')
            BREAK_AT_END;
          BREAK_AT_END;
          cread->seq_s = current;
        
          // Find & skip qual header
          while(*current++ != '\n')
            BREAK_AT_END;
          BREAK_AT_END;
          if(*current != '+')
            continue;
          cread->seq_e = current - 1;
          while(*current++ != '\n')
            BREAK_AT_END;
          BREAK_AT_END;
          cread->qual_s = current;
          current += (cread->seq_e - cread->seq_s);
          BREAK_AT_END;
          if(*current == '\n') {
            nb_reads++;
            break; // Found a proper sequence/qual pair of lines
          }
        }
      }      
      new_seq->nb_reads = nb_reads;
      if(nb_reads > 0) {
        new_seq->file = &*current_file;
        current_file->inc();
        rq.enqueue(new_seq);
      }
      if(current >= map_end) {
        current_file->unmap();
        if(++current_file == mapped_files.end()) {
          rq.close();
          break;
        }
        current_file->map();
        current_file->will_need();
        map_base = current_file->base();
        current  = map_base;
        map_end  = current_file->end();
      }
    }
  }

  const uint_t fastq_parser::codes[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1,  0, -1,  1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
  };

  const float fastq_parser::proba_codes[256] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.00000000000000000000f, 0.79432823472428149003f, 0.63095734448019324958f,
    0.50118723362727224391f, 0.39810717055349720273f, 0.31622776601683794118f,
    0.25118864315095801309f, 0.19952623149688797355f, 0.15848931924611134314f,
    0.12589254117941672817f, 0.10000000000000000000f, 0.07943282347242813790f,
    0.06309573444801933051f, 0.05011872336272722023f, 0.03981071705534973415f,
    0.03162277660168379134f, 0.02511886431509579437f, 0.01995262314968879874f,
    0.01584893192461113431f, 0.01258925411794167490f, 0.01000000000000000000f,
    0.00794328234724281379f, 0.00630957344480193028f, 0.00501187233627272462f,
    0.00398107170553497342f, 0.00316227766016837939f, 0.00251188643150957944f,
    0.00199526231496887892f, 0.00158489319246111408f, 0.00125892541179416749f,
    0.00100000000000000000f, 0.00079432823472428131f, 0.00063095734448019298f,
    0.00050118723362727253f, 0.00039810717055349735f, 0.00031622776601683794f,
    0.00025118864315095795f, 0.00019952623149688788f, 0.00015848931924611142f,
    0.00012589254117941674f, 0.00010000000000000000f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  };
  const float fastq_parser::one_minus_proba_codes[256] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.00000000000000000000f, 0.20567176527571850997f, 0.36904265551980675042f,
    0.49881276637272775609f, 0.60189282944650279727f, 0.68377223398316200331f,
    0.74881135684904198691f, 0.80047376850311202645f, 0.84151068075388868461f,
    0.87410745882058327183f, 0.90000000000000002220f, 0.92056717652757180659f,
    0.93690426555198069725f, 0.94988127663727273120f, 0.96018928294465022422f,
    0.96837722339831622254f, 0.97488113568490419869f, 0.98004737685031118044f,
    0.98415106807538887956f, 0.98741074588205834939f, 0.98999999999999999112f,
    0.99205671765275715845f, 0.99369042655519801421f, 0.99498812766372723981f,
    0.99601892829446503352f, 0.99683772233983158895f, 0.99748811356849043097f,
    0.99800473768503117356f, 0.99841510680753886575f, 0.99874107458820582384f,
    0.99899999999999999911f, 0.99920567176527574915f, 0.99936904265551984583f,
    0.99949881276637275729f, 0.99960189282944644784f, 0.99968377223398319220f,
    0.99974881135684900979f, 0.99980047376850311736f, 0.99984151068075388658f,
    0.99987410745882054908f, 0.99990000000000001101f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
  };
}
