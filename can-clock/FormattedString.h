#include <Arduino.h>

class FormattedString : String {

private:
  String value;

public:
  FormattedString() { };
  FormattedString(String v) : value (v) { }
  operator FormattedString () { return value; }

  using String::operator=;
  using String::operator==;
  using String::operator!=;

  String padLeft(byte length)
  {
    String str = value;
    str.trim();
    str = str.substring(0,length);
    while ( str.length() < length) {
      str = str + " ";
    }
    return str;
  }

  String padRight(byte length)
  {
    String str = value;
    str.trim();
    str = str.substring(0,length);
    while ( str.length() < length) {
      str = " " + str;
    }
    return str;
  }

  String padCenter(byte length)
  {
    String str = value;
    str.trim();
    str = str.substring(0,length);
    while ( str.length() < length) {
      str = (str.length() % 2) ? str + " " : " " +str;
    }
    return str;
  }
};
