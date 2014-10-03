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

#ifdef SWIGRUBY
%bang MerDNA::randomize();
%bang MerDNA::canonicalize();
%bang MerDNA::reverse_complement();
%bang MerDNA::polyA();
%bang MerDNA::polyC();
%bang MerDNA::polyG();
%bang MerDNA::polyT();
// %predicate MerDNA::is_homopolymer(); // Does not work???
%rename("MerDNA::homopolymer?") MerDNA::is_homopolymer();
%rename("MerDNA::complement") MerDNA::get_reverse_complement();
%rename("MerDNA::canonical") MerDNA::get_canonical();
#endif


%feature("autodoc", "Class representing a mer. All the mers have the same length, which must be set BEFORE instantiating any mers with jellyfish::MerDNA::k(int)");
class MerDNA {
public:
  MerDNA();
  MerDNA(const char*);
  MerDNA(const MerDNA&);

  %feature("autodoc", "Get the length of the k-mers");
  static unsigned int k();
  %feature("autodoc", "Set the length of the k-mers");
  static unsigned int k(unsigned int);

  %feature("autodoc", "Change the mer to a homopolymer of A");
  void polyA();
  %feature("autodoc", "Change the mer to a homopolymer of C");
  void polyC();
  %feature("autodoc", "Change the mer to a homopolymer of G");
  void polyG();
  %feature("autodoc", "Change the mer to a homopolymer of T");
  void polyT();
  %feature("autodoc", "Change the mer to a random one");
  void randomize();
  %feature("autodoc", "Check if the mer is a homopolymer");
  bool is_homopolymer() const;

  %feature("autodoc", "Shift a base to the left and the leftmost base is return . \"ACGT\", shift_left('A') becomes \"CGTA\" and 'A' is returned");
  char shift_left(char);
  %feature("autodoc", "Shift a base to the right and the rightmost base is return . \"ACGT\", shift_right('A') becomes \"AACG\" and 'T' is returned");
  char shift_right(char);

  %feature("autodoc", "Change the mer to its canonical representation");
  void canonicalize();
  %feature("autodoc", "Change the mer to its reverse complement");
  void reverse_complement();
  %feature("autodoc", "Return canonical representation of the mer");
  MerDNA get_canonical() const;
  %feature("autodoc", "Return the reverse complement of the mer");
  MerDNA get_reverse_complement() const;

  %feature("autodoc", "Equality between mers");
  bool operator==(const MerDNA&) const;
  %feature("autodoc", "Lexicographic less-than");
  bool operator<(const MerDNA&) const;
  %feature("autodoc", "Lexicographic greater-than");
  bool operator>(const MerDNA&) const;


  %extend{
    %feature("autodoc", "Duplicate the mer");
    MerDNA dup() const { return MerDNA(*self); }
    %feature("autodoc", "Return string representation of the mer");
    std::string __str__() { return self->to_str(); }
    %feature("autodoc", "Set the mer from a string");
    void set(const char* s) throw(std::length_error) { *static_cast<jellyfish::mer_dna*>(self) = s; }

#ifdef SWIGPERL
    char get_base(unsigned int i) { return (char)self->base(i); }
    void set_base(unsigned int i, char b) { self->base(i) = b; }
#else
    %feature("autodoc", "Get base i (0 <= i < k)");
    char __getitem__(unsigned int i) { return (char)self->base(i); }
    %feature("autodoc", "Set base i (0 <= i < k)");
    void __setitem__(unsigned int i, char b) { self->base(i) = b; }
    //    MerDNA __neg__() const { return self->get_reverse_complement(); }
    %feature("autodoc", "Shift a base to the left and return the mer");
    MerDNA& __lshift__(char b) { self->shift_left(b); return *self; }
    %feature("autodoc", "Shift a base to the right and return the mer");
    MerDNA& __rshift__(char b) { self->shift_right(b); return *self; }
#endif
  }
};
