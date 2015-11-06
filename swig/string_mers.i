/****************************************/
/* Iterator of all the mers in a string */
/****************************************/
#ifdef SWIGPYTHON
%exception __next__ {
      $action;
      if(!result) {
        PyErr_SetString(PyExc_StopIteration, "Done");
        SWIG_fail;
      }
    }
%exception next {
      $action;
      if(!result) {
        PyErr_SetString(PyExc_StopIteration, "Done");
        SWIG_fail;
      }
    }
#endif

%{
  class StringMers {
    const char*       m_current;
    const char* const m_last;
    const bool        m_canonical;
    MerDNA            m_m, m_rcm;
    unsigned int      m_filled;

  public:
    StringMers(const char* str, int len, bool canonical)
      : m_current(str)
      , m_last(str + len)
      , m_canonical(canonical)
      , m_filled(0)
    { }

    bool next_mer() {
      if(m_current == m_last)
        return false;

      do {
        int code = jellyfish::mer_dna::code(*m_current);
        ++m_current;
        if(code >= 0) {
          m_m.shift_left(code);
          if(m_canonical)
            m_rcm.shift_right(m_rcm.complement(code));
          m_filled = std::min(m_filled + 1, m_m.k());
        } else
          m_filled = 0;
      } while(m_filled < m_m.k() && m_current != m_last);
      return m_filled == m_m.k();
    }

    const MerDNA* mer() const { return !m_canonical || m_m < m_rcm ? &m_m : &m_rcm; }

    const MerDNA* next_mer__() {
      return next_mer() ? mer() : nullptr;
    }


#ifdef SWIGRUBY
    void each() {
      if(!rb_block_given_p()) return;
      while(next_mer()) {
        auto m = SWIG_NewPointerObj(const_cast<MerDNA*>(mer()), SWIGTYPE_p_MerDNA, 0);
        rb_yield(m);
      }
    }
#endif

#ifdef SWIGPYTHON
    StringMers* __iter__() { return this; }
    const MerDNA* __next__() { return next_mer__(); }
    const MerDNA* next() { return next_mer__(); }
#endif

#ifdef SWIGPERL
    const MerDNA* each() { return next_mer__(); }
#endif

  };

  StringMers* string_mers(char* str, int length) { return new StringMers(str, length, false); }
  StringMers* string_canonicals(char* str, int length) { return new StringMers(str, length, true); }
%}

%apply (char *STRING, int LENGTH) { (char* str, int length) };
%newobject string_mers;
%newobject string_canonicals;
%feature("autodoc", "Get an iterator to the mers in the string");
StringMers* string_mers(char* str, int length);
%feature("autodoc", "Get an iterator to the canonical mers in the string");
StringMers* string_canonicals(char* str, int length);

#ifdef SWIGRUBY
%mixin StringMers "Enumerable";
%init %{
  rb_eval_string("class String\n"
                 "  def mers(&b); it = Jellyfish::string_mers(self); b ? it.each(&b) : it; end\n"
                 "  def canonicals(&b); it = Jellyfish::string_canonicals(self, &b); b ? it.each(&b) : it; end\n"
                 "end");
%}
#endif

/* #ifdef SWIGPERL */
/* // For perl, return an empty array at end of iterator */
/* %typemap(out) const MerDNA* { */
/*   if($1) { */
/*     SWIG_Object m = SWIG_NewPointerObj(const_cast<MerDNA*>($1), SWIGTYPE_p_MerDNA, 0); */
/*     %append_output(m); */
/*   } */
/*  } */
/* #endif */


%feature("autodoc", "Extract k-mers from a sequence string");
class StringMers {
public:
  %feature("autodoc", "Create a k-mers parser from a string. Pass true as a second argument to get canonical mers");
  StringMers(const char* str, int len, bool canonical);

  %feature("autodoc", "Get the next mer. Return false if reached the end of the string.");
  bool next_mer();

  %feature("autodoc", "Return the current mer (or its canonical representation)");
  const MerDNA* mer() const;

#ifdef SWIGRUBY
  %feature("autodoc", "Iterate through all the mers in the string");
  void each();
#endif

#ifdef SWIGPYTHON
  StringMers* __iter__();
  const MerDNA* __next__();
  const MerDNA* next();
#endif

#ifdef SWIGPERL
  MerDNA* each();
#endif
};
