%module jellyfish
%naturalvar; // Use const reference instead of pointers
%include "std_string.i"
%include "exception.i"
%include "std_except.i"

%{
#include <fstream>
#include <stdexcept>
#undef die
#include <jellyfish/mer_dna.hpp>
#include <jellyfish/file_header.hpp>
#include <jellyfish/mer_dna_bloom_counter.hpp>
#include <jellyfish/jellyfish.hpp>
#undef die

  class MerDNA : public jellyfish::mer_dna {
 public:
  MerDNA() = default;
  MerDNA(const char* s) : jellyfish::mer_dna(s) { }
  MerDNA(const MerDNA& m) : jellyfish::mer_dna(m) { }
    MerDNA& operator=(const jellyfish::mer_dna& m) { *static_cast<jellyfish::mer_dna*>(this) = m; return *this; }
  };
%}


/**********************************/
/* MerDNA proxy class for mer_dna */
/**********************************/
%bang MerDNA::randomize();
%bang MerDNA::canonicalize();
%bang MerDNA::reverse_complement();
%bang MerDNA::polyA();
%bang MerDNA::polyC();
%bang MerDNA::polyG();
%bang MerDNA::polyT();
%predicate MerDNA::is_homopolymer(); // Does not work???

class MerDNA {
public:
  MerDNA();
  MerDNA(const char*);
  MerDNA(const MerDNA&);

  static unsigned int k();
  static unsigned int k(unsigned int);

  void polyA();
  void polyC();
  void polyG();
  void polyT();
  void randomize();
  bool is_homopolymer() const;

  char shift_left(char);
  char shift_right(char);

  void canonicalize();
  void reverse_complement();
  MerDNA get_canonical() const;
  MerDNA get_reverse_complement() const;

  bool operator==(const MerDNA&) const;
  bool operator<(const MerDNA&) const;
  bool operator>(const MerDNA&) const;


  %extend{
    MerDNA dup() const { return MerDNA(*self); }
    std::string __str__() { return self->to_str(); }
    void set(const char* s) throw(std::length_error) { *static_cast<jellyfish::mer_dna*>(self) = s; }

#ifdef SWIGPERL
    char get_base(unsigned int i) { return (char)self->base(i); }
    void set_base(unsigned int i, char b) { self->base(i) = b; }
#else
    char __getitem__(unsigned int i) { return (char)self->base(i); }
    void __setitem__(unsigned int i, char b) { self->base(i) = b; }
    //    MerDNA __neg__() const { return self->get_reverse_complement(); }
    MerDNA& __lshift__(char b) { self->shift_left(b); return *self; }
    MerDNA& __rshift__(char b) { self->shift_right(b); return *self; }
#endif
  }
};

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
