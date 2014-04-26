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
#ifdef SWIGRUBY
%mixin ReadMerFile "Enumerable";
#endif

#ifdef SWIGPYTHON
// Convert output of __next__() or next()
%exception __next__ {
  $action;
  if(!result.first) {
    PyErr_SetString(PyExc_StopIteration, "Done");
    SWIG_fail;
  }
 }
%typemap(out) std::pair<const MerDNA*, uint64_t> {
  SWIG_Object m = SWIG_NewPointerObj(const_cast<MerDNA*>(($1).first), SWIGTYPE_p_MerDNA, 0);
  SWIG_Object c = SWIG_From(unsigned long)(($1).second);
  %append_output(m);
  %append_output(c);
 }
#endif

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

    bool next_mer() {
      if(binary) {
        if(binary->next()) return true;
        binary.reset();
      } else if(text) {
        if(text->next()) return true;
        text.reset();
      }
      return false;
    }

    const MerDNA* mer() const { return static_cast<const MerDNA*>(binary ? &binary->key() : &text->key()); }
    unsigned long count() const { return binary ? binary->val() : text->val(); }

#ifdef SWIGRUBY
    void each() {
      if(!rb_block_given_p()) return;
      while(next_mer()) {
        auto m = SWIG_NewPointerObj(const_cast<MerDNA*>(mer()), SWIGTYPE_p_MerDNA, 0);
        auto c = SWIG_From_unsigned_SS_long(count());
        rb_yield(rb_ary_new3(2, m, c));
      }
    }
#endif

#ifdef SWIGPYTHON
    ReadMerFile* __iter__() { return this; }
    std::pair<const MerDNA*, uint64_t> __next__() {
      std::pair<const MerDNA*, uint64_t> res((const MerDNA*)0, 0);
      if(next_mer()) {
        res.first  = mer();
        res.second = count();
      }
      return res;
    }
    std::pair<const MerDNA*, uint64_t> next() { return __next__(); }
#endif
  };
%}
