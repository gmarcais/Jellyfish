/******************************/
/* Query output of jellyfish. */
/******************************/
%inline %{
  class QueryMerFile {
    std::unique_ptr<jellyfish::mer_dna_bloom_filter> bf;
    jellyfish::mapped_file                           binary_map;
    std::unique_ptr<binary_query>                    jf;

  public:
    QueryMerFile(const char* path) throw(std::runtime_error) {
      std::ifstream in(path);
      if(!in.good())
        throw std::runtime_error(std::string("Can't open file '") + path + "'");
      jellyfish::file_header header(in);
      jellyfish::mer_dna::k(header.key_len() / 2);
      if(header.format() == "bloomcounter") {
        jellyfish::hash_pair<jellyfish::mer_dna> fns(header.matrix(1), header.matrix(2));
        bf.reset(new jellyfish::mer_dna_bloom_filter(header.size(), header.nb_hashes(), in, fns));
        if(!in.good())
          throw std::runtime_error("Bloom filter file is truncated");
      } else if(header.format() == "binary/sorted") {
        binary_map.map(path);
        jf.reset(new binary_query(binary_map.base() + header.offset(), header.key_len(), header.counter_len(), header.matrix(),
                                  header.size() - 1, binary_map.length() - header.offset()));
      } else {
        throw std::runtime_error(std::string("Unsupported format '") + header.format() + "'");
      }
    }

#ifdef SWIGPERL
    unsigned int get(const MerDNA& m) { return jf ? jf->check(m) : bf->check(m); }
#else
    unsigned int __getitem__(const MerDNA& m) { return jf ? jf->check(m) : bf->check(m); }
#endif
  };
%}

/****************************/
/* Read output of jellyfish */
/****************************/
%inline %{
  class ReadMerFile {
    std::ifstream                  in;
    std::unique_ptr<binary_reader> binary;
    std::unique_ptr<text_reader>   text;

  public:
    ReadMerFile(const char* path) throw(std::runtime_error) :
      in(path)
    {
      if(!in.good())
        throw std::runtime_error(std::string("Can't open file '") + path + "'");
      jellyfish::file_header header(in);
      jellyfish::mer_dna::k(header.key_len() / 2);
      if(header.format() == binary_dumper::format)
        binary.reset(new binary_reader(in, &header));
      else if(header.format() == text_dumper::format)
        text.reset(new text_reader(in, &header));
      else
        throw std::runtime_error(std::string("Unsupported format '") + header.format() + "'");
    }

    bool next2() {
      if(binary) {
        if(binary->next()) return true;
        binary.reset();
      } else if(text) {
        if(text->next()) return true;
        text.reset();
      }
      return false;
    }

    const MerDNA* key() const { return static_cast<const MerDNA*>(binary ? &binary->key() : &text->key()); }
    unsigned long val() const { return binary ? binary->val() : text->val(); }
  };
%}
