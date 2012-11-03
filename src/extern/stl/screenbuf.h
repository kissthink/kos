#ifndef _screenbuf_h_
#define _screenbuf_h_ 1

#include <ios>
#include <streambuf>

using namespace std;

template<typename _CharT, typename _Traits = char_traits<_CharT>>
class ScreenBuffer : public basic_streambuf<_CharT, _Traits> {
public:
  typedef _CharT														char_type;
  typedef _Traits														traits_type;
  typedef typename traits_type::int_type		int_type;
  typedef typename traits_type::pos_type		pos_type;
  typedef typename traits_type::off_type		off_type;

  typedef basic_streambuf<_CharT, _Traits> BaseClass;

private:
  OutputDevice& odev;

public:
  explicit ScreenBuffer(OutputDevice& o) : odev(o) {}

protected:
  virtual streamsize xsputn(const char_type* s, streamsize n) {
    return odev.write(s, n);
  }

  virtual int sync() {
    return odev.sync();
  }
};

#endif /* _screenbuf_h_ */
