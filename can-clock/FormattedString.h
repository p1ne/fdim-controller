#ifndef __FORMATTED_STRING_H_
#define __FORMATTED_STRING_H_

#include <Arduino.h>

class FormattedString {

private:
  String value;

public:
  FormattedString() { };
  FormattedString(const String v) { value = v; }

  operator String () const { return value; }

  void operator = (const String v)  { value = v; }
  void operator = (const FormattedString v)  { value = v; }

  FormattedString operator + (const String v)  { return (value + v); }
  FormattedString operator + (const FormattedString v)  { return (value + v); }

  bool operator == (const FormattedString v) const { return (v == value); }
  bool operator == (const String v) const { return (v == value); }

  bool operator != (const FormattedString v) const { return (v != value); }
  bool operator != (const String v) const { return (v != value); }

  String padLeft(byte length) const
  {
    String str = value;
    str.trim();
    str = str.substring(0,length);
    while ( str.length() < length) {
      str = str + " ";
    }
    return str;
  }

  String padRight(byte length) const
  {
    String str = value;
    str.trim();
    str = str.substring(0,length);
    while ( str.length() < length) {
      str = " " + str;
    }
    return str;
  }

  String padCenter(byte length) const
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

#endif // __FORMATTED_STRING_H_
