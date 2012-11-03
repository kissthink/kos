#ifndef _keyboardbuf_h_
#define _keyboardbuf_h_ 1

#include <streambuf>

template<typename _CharT, typename _Traits = std::char_traits<_CharT>>
class keyboardbuf : public std::basic_streambuf<_CharT, _Traits> {
public:
  typedef _CharT					char_type;
  typedef _Traits					traits_type;
  typedef typename traits_type::int_type		int_type;
  typedef typename traits_type::pos_type		pos_type;
  typedef typename traits_type::off_type		off_type;

private:
  // Underlying stdio FILE
  std::__c_file* const _M_file;

  // Last character gotten. This is used when pbackfail is
  // called from basic_streambuf::sungetc()
  int_type _M_unget_buf;

public:
  explicit keyboardbuf(std::__c_file* __f)
    : _M_file(__f), _M_unget_buf(traits_type::eof()) { }

protected:
  int_type syncgetc();
  int_type syncungetc(int_type __c);
  int_type syncputc(int_type __c);

  virtual int_type underflow() {
	  int_type __c = this->syncgetc();
  	return this->syncungetc(__c);
  }

  virtual int_type uflow() {
    // Store the gotten character in case we need to unget it.
    _M_unget_buf = this->syncgetc();
    return _M_unget_buf;
  }

  virtual int_type pbackfail(int_type __c = traits_type::eof()) {
  	int_type __ret;
  	const int_type __eof = traits_type::eof();

  	// Check if the unget or putback was requested
  	if (traits_type::eq_int_type(__c, __eof)) // unget {
	    if (!traits_type::eq_int_type(_M_unget_buf, __eof))
	      __ret = this->syncungetc(_M_unget_buf);
	    else // buffer invalid, fail.
	      __ret = __eof;
	  } else // putback
  	  __ret = this->syncungetc(__c);

  	// The buffered character is no longer valid, discard it.
  	_M_unget_buf = __eof;
  	return __ret;
  }

  virtual std::streamsize xsgetn(char_type* __s, std::streamsize __n) __attribute__((error ("cannot use #func")));

  virtual int_type overflow(int_type __c = traits_type::eof()) {
  	int_type __ret;
  	if (traits_type::eq_int_type(__c, traits_type::eof())) {
	    if (std::fflush(_M_file))
	      __ret = traits_type::eof();
	    else
	      __ret = traits_type::not_eof(__c);
	  } else
  	  __ret = this->syncputc(__c);
	  return __ret;
  }

  virtual std::streamsize xsputn(const char_type* __s, std::streamsize __n);

  virtual int sync() { return std::fflush(_M_file); }

  virtual std::streampos seekoff(std::streamoff __off, std::ios_base::seekdir __dir,
    std::ios_base::openmode = std::ios_base::in | std::ios_base::out) {

  	std::streampos __ret(std::streamoff(-1));
  	int __whence;
  	if (__dir == std::ios_base::beg) __whence = SEEK_SET;
  	else if (__dir == std::ios_base::cur) __whence = SEEK_CUR;
  	else __whence = SEEK_END;
#ifdef _GLIBCXX_USE_LFS
  	if (!fseeko64(_M_file, __off, __whence)) __ret = std::streampos(ftello64(_M_file));
#else
  	if (!fseek(_M_file, __off, __whence)) __ret = std::streampos(std::ftell(_M_file));
#endif
  	return __ret;
  }

  virtual std::streampos seekpos(std::streampos __pos,
	  std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out) {
    return seekoff(std::streamoff(__pos), std::ios_base::beg, __mode);
  }
};

template<> inline keyboardbuf<char>::int_type
keyboardbuf<char>::syncgetc() {
  return std::getc(_M_file);
}

template<> inline keyboardbuf<char>::int_type
keyboardbuf<char>::syncungetc(int_type __c) {
  return std::ungetc(__c, _M_file);
}

template<> inline keyboardbuf<char>::int_type
keyboardbuf<char>::syncputc(int_type __c) {
  return std::putc(__c, _M_file);
}

template<> inline std::streamsize
keyboardbuf<char>::xsgetn(char* __s, std::streamsize __n) {
  std::streamsize __ret = std::fread(__s, 1, __n, _M_file);
  if (__ret > 0) _M_unget_buf = traits_type::to_int_type(__s[__ret - 1]);
  else _M_unget_buf = traits_type::eof();
  return __ret;
}

template<> inline std::streamsize
keyboardbuf<char>::xsputn(const char* __s, std::streamsize __n) {
  return std::fwrite(__s, 1, __n, _M_file);
}

#endif /* _keyboardbuf_h_ */
