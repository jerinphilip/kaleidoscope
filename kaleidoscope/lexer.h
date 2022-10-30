#include <cstdio>
#include <string>

enum class Token {
  eof = -1,
  def = -2,
  extern_ = -3,
  identifier = -4,
  number = -5,
  init = -6,
  semicolon = -7,
  comment = -8,
  unknown = -9
};

class Parser {
public:
  Token read();
  const std::string &atom() const { return atom_; }

private:
  std::string atom_;
};
