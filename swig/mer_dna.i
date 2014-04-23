/**********************************/
/* MerDNA proxy class for mer_dna */
/**********************************/
%{
  class MerDNA : public jellyfish::mer_dna {
  public:
    MerDNA() = default;
    MerDNA(const char* s) : jellyfish::mer_dna(s) { }
    MerDNA(const MerDNA& m) : jellyfish::mer_dna(m) { }
    MerDNA& operator=(const jellyfish::mer_dna& m) { *static_cast<jellyfish::mer_dna*>(this) = m; return *this; }
  };
%}

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
