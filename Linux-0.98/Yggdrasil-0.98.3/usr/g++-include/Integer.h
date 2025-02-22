// This may look like C code, but it is really -*- C++ -*-

/* 
Copyright (C) 1988 Free Software Foundation
    written by Doug Lea (dl@rocky.oswego.edu)

This file is part of the GNU C++ Library.  This library is free
software; you can redistribute it and/or modify it under the terms of
the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.  This library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _Integer_h
#ifdef __GNUG__
#pragma interface
#endif
#define _Integer_h 1

#include <stream.h>

struct IntRep                    // internal Integer representations
{
  unsigned short  len;          // current length
  unsigned short  sz;           // allocated space (0 means static).
  short           sgn;          // 1 means >= 0; 0 means < 0 
  unsigned short  s[1];         // represented as ushort array starting here
};

// True if REP is staticly (or manually) allocated,
// and should not be deleted by an Integer destructor.
#define STATIC_IntRep(rep) ((rep)->sz==0)

extern IntRep*  Ialloc(IntRep*, const unsigned short *, int, int, int);
extern IntRep*  Icalloc(IntRep*, int);
extern IntRep*  Icopy_long(IntRep*, long);
extern IntRep*  Icopy(IntRep*, const IntRep*);
extern IntRep*  Iresize(IntRep*, int);
extern IntRep*  add(const IntRep*, int, const IntRep*, int, IntRep*);
extern IntRep*  add(const IntRep*, int, long, IntRep*);
extern IntRep*  multiply(const IntRep*, const IntRep*, IntRep*);
extern IntRep*  multiply(const IntRep*, long, IntRep*);
extern IntRep*  lshift(const IntRep*, long, IntRep*);
extern IntRep*  lshift(const IntRep*, const IntRep*, int, IntRep*);
extern IntRep*  bitop(const IntRep*, const IntRep*, IntRep*, char);
extern IntRep*  bitop(const IntRep*, long, IntRep*, char);
extern IntRep*  power(const IntRep*, long, IntRep*);
extern IntRep*  div(const IntRep*, const IntRep*, IntRep*);
extern IntRep*  mod(const IntRep*, const IntRep*, IntRep*);
extern IntRep*  div(const IntRep*, long, IntRep*);
extern IntRep*  mod(const IntRep*, long, IntRep*);
extern IntRep*  compl(const IntRep*, IntRep*);
extern IntRep*  abs(const IntRep*, IntRep*);
extern IntRep*  negate(const IntRep*, IntRep*);
extern IntRep*  pow(const IntRep*, long);
extern IntRep*  gcd(const IntRep*, const IntRep* y);
extern int      compare(const IntRep*, const IntRep*);
extern int      compare(const IntRep*, long);
extern int      ucompare(const IntRep*, const IntRep*);
extern int      ucompare(const IntRep*, long);
extern char*    Itoa(const IntRep* x, int base = 10, int width = 0);
extern char*    cvtItoa(const IntRep* x, char* fmt, int& fmtlen, int base,
                        int showbase, int width, int align_right, 
                        char fillchar, char Xcase, int showpos);
extern IntRep*  atoIntRep(const char* s, int base = 10);
extern long     Itolong(const IntRep*);
extern double   Itodouble(const IntRep*);
extern int      Iislong(const IntRep*);
extern int      Iisdouble(const IntRep*);
extern long     lg(const IntRep*);

extern IntRep _ZeroRep, _OneRep, _MinusOneRep;

class Integer
{
protected:
  IntRep*         rep;
public:
                  Integer();
                  Integer(long);
                  Integer(IntRep*);
                  Integer(const Integer&);

                  ~Integer();

  void            operator =  (const Integer&);
  void            operator =  (long);

// unary operations to self

  void            operator ++ ();
  void            operator -- ();
  void            negate();          // negate in-place
  void            abs();             // absolute-value in-place
  void            complement();      // bitwise complement in-place

// assignment-based operations

  void            operator += (const Integer&);
  void            operator -= (const Integer&);
  void            operator *= (const Integer&);
  void            operator /= (const Integer&);
  void            operator %= (const Integer&);
  void            operator <<=(const Integer&);
  void            operator >>=(const Integer&);
  void            operator &= (const Integer&);
  void            operator |= (const Integer&);
  void            operator ^= (const Integer&);

  void            operator += (long);
  void            operator -= (long);
  void            operator *= (long);
  void            operator /= (long);
  void            operator %= (long);
  void            operator <<=(long);
  void            operator >>=(long);
  void            operator &= (long);
  void            operator |= (long);
  void            operator ^= (long);

// (constructive binary operations are inlined below)

#ifdef __GNUG__
  friend Integer operator <? (const Integer& x, const Integer& y); // min
  friend Integer operator >? (const Integer& x, const Integer& y); // max
#endif

// builtin Integer functions that must be friends

  friend long     lg (const Integer&); // floor log base 2 of abs(x)
  friend double   ratio(const Integer& x, const Integer& y);
                  // return x/y as a double

  friend Integer  gcd(const Integer&, const Integer&);
  friend int      even(const Integer&); // true if even
  friend int      odd(const Integer&); // true if odd
  friend int      sign(const Integer&); // returns -1, 0, +1

  friend void     (setbit)(Integer& x, long b);   // set b'th bit of x
  friend void     clearbit(Integer& x, long b); // clear b'th bit
  friend int      testbit(const Integer& x, long b);  // return b'th bit

// procedural versions of operators

  friend void     abs(const Integer& x, Integer& dest);
  friend void     negate(const Integer& x, Integer& dest);
  friend void     complement(const Integer& x, Integer& dest);

  friend int      compare(const Integer&, const Integer&);  
  friend int      ucompare(const Integer&, const Integer&); 
  friend void     add(const Integer& x, const Integer& y, Integer& dest);
  friend void     sub(const Integer& x, const Integer& y, Integer& dest);
  friend void     mul(const Integer& x, const Integer& y, Integer& dest);
  friend void     div(const Integer& x, const Integer& y, Integer& dest);
  friend void     mod(const Integer& x, const Integer& y, Integer& dest);
  friend void     divide(const Integer& x, const Integer& y, 
                         Integer& q, Integer& r);
  friend void     and(const Integer& x, const Integer& y, Integer& dest);
  friend void     or(const Integer& x, const Integer& y, Integer& dest);
  friend void     xor(const Integer& x, const Integer& y, Integer& dest);
  friend void     lshift(const Integer& x, const Integer& y, Integer& dest);
  friend void     rshift(const Integer& x, const Integer& y, Integer& dest);
  friend void     pow(const Integer& x, const Integer& y, Integer& dest);

  friend int      compare(const Integer&, long);  
  friend int      ucompare(const Integer&, long); 
  friend void     add(const Integer& x, long y, Integer& dest);
  friend void     sub(const Integer& x, long y, Integer& dest);
  friend void     mul(const Integer& x, long y, Integer& dest);
  friend void     div(const Integer& x, long y, Integer& dest);
  friend void     mod(const Integer& x, long y, Integer& dest);
  friend void     divide(const Integer& x, long y, Integer& q, long& r);
  friend void     and(const Integer& x, long y, Integer& dest);
  friend void     or(const Integer& x, long y, Integer& dest);
  friend void     xor(const Integer& x, long y, Integer& dest);
  friend void     lshift(const Integer& x, long y, Integer& dest);
  friend void     rshift(const Integer& x, long y, Integer& dest);
  friend void     pow(const Integer& x, long y, Integer& dest);

  friend int      compare(long, const Integer&);  
  friend int      ucompare(long, const Integer&); 
  friend void     add(long x, const Integer& y, Integer& dest);
  friend void     sub(long x, const Integer& y, Integer& dest);
  friend void     mul(long x, const Integer& y, Integer& dest);
  friend void     and(long x, const Integer& y, Integer& dest);
  friend void     or(long x, const Integer& y, Integer& dest);
  friend void     xor(long x, const Integer& y, Integer& dest);

// coercion & conversion

  int             fits_in_long() const;
  int             fits_in_double() const;

                  operator long() const;
                  operator double() const;

  friend char*    Itoa(const Integer& x, int base = 10, int width = 0);
  friend Integer  atoI(const char* s, int base = 10);
  void		  printon(ostream& s, int base = 10, int width = 0) const;
  
  friend istream& operator >> (istream& s, Integer& y);
  friend ostream& operator << (ostream& s, const Integer& y);

// error detection

  int             initialized() const;
  void   error(const char* msg) const;
  int             OK() const;  
};


//  (These are declared inline)

  int      operator == (const Integer&, const Integer&);
  int      operator == (const Integer&, long);
  int      operator != (const Integer&, const Integer&);
  int      operator != (const Integer&, long);
  int      operator <  (const Integer&, const Integer&);
  int      operator <  (const Integer&, long);
  int      operator <= (const Integer&, const Integer&);
  int      operator <= (const Integer&, long);
  int      operator >  (const Integer&, const Integer&);
  int      operator >  (const Integer&, long);
  int      operator >= (const Integer&, const Integer&);
  int      operator >= (const Integer&, long);
  Integer  operator -  (const Integer&);
  Integer  operator ~  (const Integer&);
  Integer  operator +  (const Integer&, const Integer&);
  Integer  operator +  (const Integer&, long);
  Integer  operator +  (long, const Integer&);
  Integer  operator -  (const Integer&, const Integer&);
  Integer  operator -  (const Integer&, long);
  Integer  operator -  (long, const Integer&);
  Integer  operator *  (const Integer&, const Integer&);
  Integer  operator *  (const Integer&, long);
  Integer  operator *  (long, const Integer&);
  Integer  operator /  (const Integer&, const Integer&);
  Integer  operator /  (const Integer&, long);
  Integer  operator %  (const Integer&, const Integer&);
  Integer  operator %  (const Integer&, long);
  Integer  operator << (const Integer&, const Integer&);
  Integer  operator << (const Integer&, long);
  Integer  operator >> (const Integer&, const Integer&);
  Integer  operator >> (const Integer&, long);
  Integer  operator &  (const Integer&, const Integer&);
  Integer  operator &  (const Integer&, long);
  Integer  operator &  (long, const Integer&);
  Integer  operator |  (const Integer&, const Integer&);
  Integer  operator |  (const Integer&, long);
  Integer  operator |  (long, const Integer&);
  Integer  operator ^  (const Integer&, const Integer&);
  Integer  operator ^  (const Integer&, long);
  Integer  operator ^  (long, const Integer&);

  Integer  abs(const Integer&); // absolute value
  Integer  sqr(const Integer&); // square

  Integer  pow(const Integer& x, const Integer& y);
  Integer  pow(const Integer& x, long y);
  Integer  Ipow(long x, long y); // x to the y as Integer 


extern char*    dec(const Integer& x, int width = 0);
extern char*    oct(const Integer& x, int width = 0);
extern char*    hex(const Integer& x, int width = 0);
extern Integer  sqrt(const Integer&); // floor of square root
extern Integer  lcm(const Integer& x, const Integer& y); // least common mult


typedef Integer IntTmp; // for backward compatibility

inline Integer::Integer() :rep(&_ZeroRep) {}

inline Integer::Integer(IntRep* r) :rep(r) {}

inline Integer::Integer(long y) :rep(Icopy_long(0, y)) {}

inline Integer::Integer(const Integer&  y) :rep(Icopy(0, y.rep)) {}

inline Integer::~Integer() { if (rep && !STATIC_IntRep(rep)) delete rep; }

inline void  Integer::operator = (const Integer&  y)
{
  rep = Icopy(rep, y.rep);
}

inline void Integer::operator = (long y)
{
  rep = Icopy_long(rep, y); 
}

inline Integer::operator long() const
{ 
  return Itolong(rep);
}

inline int Integer::initialized() const
{
  return rep != 0;
}

inline int Integer::fits_in_long() const
{
  return Iislong(rep);
}

inline Integer::operator double() const
{ 
  return Itodouble(rep);
}

inline int Integer::fits_in_double() const
{
  return Iisdouble(rep);
}

// procedural versions

inline int compare(const Integer& x, const Integer& y)
{
  return compare(x.rep, y.rep);
}

inline int ucompare(const Integer& x, const Integer& y)
{
  return ucompare(x.rep, y.rep);
}

inline int compare(const Integer& x, long y)
{
  return compare(x.rep, y);
}

inline int ucompare(const Integer& x, long y)
{
  return ucompare(x.rep, y);
}

inline int compare(long x, const Integer& y)
{
  return -compare(y.rep, x);
}

inline int ucompare(long x, const Integer& y)
{
  return -ucompare(y.rep, x);
}

inline void  add(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = add(x.rep, 0, y.rep, 0, dest.rep);
}

inline void  sub(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = add(x.rep, 0, y.rep, 1, dest.rep);
}

inline void  mul(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = multiply(x.rep, y.rep, dest.rep);
}

inline void  div(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = div(x.rep, y.rep, dest.rep);
}

inline void  mod(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = mod(x.rep, y.rep, dest.rep);
}

inline void  and(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = bitop(x.rep, y.rep, dest.rep, '&');
}

inline void  or(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = bitop(x.rep, y.rep, dest.rep, '|');
}

inline void  xor(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = bitop(x.rep, y.rep, dest.rep, '^');
}

inline void  lshift(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = lshift(x.rep, y.rep, 0, dest.rep);
}

inline void  rshift(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = lshift(x.rep, y.rep, 1, dest.rep);
}

inline void  pow(const Integer& x, const Integer& y, Integer& dest)
{
  dest.rep = power(x.rep, long(y), dest.rep); // not incorrect
}

inline void  add(const Integer& x, long y, Integer& dest)
{
  dest.rep = add(x.rep, 0, y, dest.rep);
}

inline void  sub(const Integer& x, long y, Integer& dest)
{
  dest.rep = add(x.rep, 0, -y, dest.rep);
}

inline void  mul(const Integer& x, long y, Integer& dest)
{
  dest.rep = multiply(x.rep, y, dest.rep);
}

inline void  div(const Integer& x, long y, Integer& dest)
{
  dest.rep = div(x.rep, y, dest.rep);
}

inline void  mod(const Integer& x, long y, Integer& dest)
{
  dest.rep = mod(x.rep, y, dest.rep);
}

inline void  and(const Integer& x, long y, Integer& dest)
{
  dest.rep = bitop(x.rep, y, dest.rep, '&');
}

inline void  or(const Integer& x, long y, Integer& dest)
{
  dest.rep = bitop(x.rep, y, dest.rep, '|');
}

inline void  xor(const Integer& x, long y, Integer& dest)
{
  dest.rep = bitop(x.rep, y, dest.rep, '^');
}

inline void  lshift(const Integer& x, long y, Integer& dest)
{
  dest.rep = lshift(x.rep, y, dest.rep);
}

inline void  rshift(const Integer& x, long y, Integer& dest)
{
  dest.rep = lshift(x.rep, -y, dest.rep);
}

inline void  pow(const Integer& x, long y, Integer& dest)
{
  dest.rep = power(x.rep, y, dest.rep);
}

inline void abs(const Integer& x, Integer& dest)
{
  dest.rep = abs(x.rep, dest.rep);
}

inline void negate(const Integer& x, Integer& dest)
{
  dest.rep = negate(x.rep, dest.rep);
}

inline void complement(const Integer& x, Integer& dest)
{
  dest.rep = compl(x.rep, dest.rep);
}

inline void  add(long x, const Integer& y, Integer& dest)
{
  dest.rep = add(y.rep, 0, x, dest.rep);
}

inline void  sub(long x, const Integer& y, Integer& dest)
{
  dest.rep = add(y.rep, 1, x, dest.rep);
}

inline void  mul(long x, const Integer& y, Integer& dest)
{
  dest.rep = multiply(y.rep, x, dest.rep);
}

inline void  and(long x, const Integer& y, Integer& dest)
{
  dest.rep = bitop(y.rep, x, dest.rep, '&');
}

inline void  or(long x, const Integer& y, Integer& dest)
{
  dest.rep = bitop(y.rep, x, dest.rep, '|');
}

inline void  xor(long x, const Integer& y, Integer& dest)
{
  dest.rep = bitop(y.rep, x, dest.rep, '^');
}


// operator versions

inline int operator == (const Integer&  x, const Integer&  y)
{
  return compare(x, y) == 0; 
}

inline int operator == (const Integer&  x, long y)
{
  return compare(x, y) == 0; 
}

inline int operator != (const Integer&  x, const Integer&  y)
{
  return compare(x, y) != 0; 
}

inline int operator != (const Integer&  x, long y)
{
  return compare(x, y) != 0; 
}

inline int operator <  (const Integer&  x, const Integer&  y)
{
  return compare(x, y) <  0; 
}

inline int operator <  (const Integer&  x, long y)
{
  return compare(x, y) <  0; 
}

inline int operator <= (const Integer&  x, const Integer&  y)
{
  return compare(x, y) <= 0; 
}

inline int operator <= (const Integer&  x, long y)
{
  return compare(x, y) <= 0; 
}

inline int operator >  (const Integer&  x, const Integer&  y)
{
  return compare(x, y) >  0; 
}

inline int operator >  (const Integer&  x, long y)
{
  return compare(x, y) >  0; 
}

inline int operator >= (const Integer&  x, const Integer&  y)
{
  return compare(x, y) >= 0; 
}

inline int operator >= (const Integer&  x, long y)
{
  return compare(x, y) >= 0; 
}


inline void  Integer::operator += (const Integer& y)
{
  add(*this, y, *this);
}

inline void  Integer::operator += (long y)
{
  add(*this, y, *this);
}

inline void Integer::operator ++ ()
{
  add(*this, 1, *this);
}


inline void  Integer::operator -= (const Integer& y)
{
  sub(*this, y, *this);
}

inline void  Integer::operator -= (long y)
{
  sub(*this, y, *this);
}

inline void Integer::operator -- ()
{
  add(*this, -1, *this);
}



inline void Integer::operator *= (const Integer& y)
{
  mul(*this, y, *this);
}

inline void Integer::operator *= (long y)
{
  mul(*this, y, *this);
}


inline void  Integer::operator &= (const Integer& y)
{
  and(*this, y, *this);
}

inline void  Integer::operator &= (long y)
{
  and(*this, y, *this);
}

inline void  Integer::operator |= (const Integer& y)
{
  or(*this, y, *this);
}

inline void  Integer::operator |= (long y)
{
  or(*this, y, *this);
}


inline void  Integer::operator ^= (const Integer& y)
{
  xor(*this, y, *this);
}

inline void  Integer::operator ^= (long y)
{
  xor(*this, y, *this);
}



inline void Integer::operator /= (const Integer& y)
{
  div(*this, y, *this);
}

inline void Integer::operator /= (long y)
{
  div(*this, y, *this);
}


inline void Integer::operator %= (const Integer& y)
{
  mod(*this, y, *this);
}

inline void Integer::operator %= (long y)
{
  mod(*this, y, *this);
}


inline void Integer::operator <<= (const Integer&  y)
{
  lshift(*this, y, *this);
}

inline void Integer::operator <<= (long  y)
{
  lshift(*this, y, *this);
}


inline void Integer::operator >>= (const Integer&  y)
{
  rshift(*this, y, *this);
}

inline void  Integer::operator >>= (long y)
{
  rshift(*this, y, *this);
}

#ifdef __GNUG__
inline Integer operator <? (const Integer& x, const Integer& y)
{
  return (compare(x.rep, y.rep) <= 0) ? x : y;
}

inline Integer operator >? (const Integer& x, const Integer& y)
{
  return (compare(x.rep, y.rep) >= 0)?  x : y;
}
#endif


inline void Integer::abs()
{
  ::abs(*this, *this);
}

inline void Integer::negate()
{
  ::negate(*this, *this);
}


inline void Integer::complement()
{
  ::complement(*this, *this);
}


inline int sign(const Integer& x)
{
  return (x.rep->len == 0) ? 0 : ( (x.rep->sgn == 1) ? 1 : -1 );
}

inline int even(const Integer& y)
{
  return y.rep->len == 0 || !(y.rep->s[0] & 1);
}

inline int odd(const Integer& y)
{
  return y.rep->len > 0 && (y.rep->s[0] & 1);
}

inline char* Itoa(const Integer& y, int base, int width)
{
  return Itoa(y.rep, base, width);
}



inline long lg(const Integer& x) 
{
  return lg(x.rep);
}

// constructive operations 

#if defined(__GNUG__) && !defined(NO_NRV)

inline Integer  operator +  (const Integer& x, const Integer& y) return r
{
  add(x, y, r);
}

inline Integer  operator +  (const Integer& x, long y) return r
{
  add(x, y, r);
}

inline Integer  operator +  (long  x, const Integer& y) return r
{
  add(x, y, r);
}

inline Integer  operator -  (const Integer& x, const Integer& y) return r
{
  sub(x, y, r);
}

inline Integer  operator -  (const Integer& x, long y) return r
{
  sub(x, y, r);
}

inline Integer  operator -  (long  x, const Integer& y) return r
{
  sub(x, y, r);
}

inline Integer  operator *  (const Integer& x, const Integer& y) return r
{
  mul(x, y, r);
}

inline Integer  operator *  (const Integer& x, long y) return r
{
  mul(x, y, r);
}

inline Integer  operator *  (long  x, const Integer& y) return r
{
  mul(x, y, r);
}

inline Integer sqr(const Integer& x) return r
{
  mul(x, x, r);
}

inline Integer  operator &  (const Integer& x, const Integer& y) return r
{
  and(x, y, r);
}

inline Integer  operator &  (const Integer& x, long y) return r
{
  and(x, y, r);
}

inline Integer  operator &  (long  x, const Integer& y) return r
{
  and(x, y, r);
}

inline Integer  operator |  (const Integer& x, const Integer& y) return r
{
  or(x, y, r);
}

inline Integer  operator |  (const Integer& x, long y) return r
{
  or(x, y, r);
}

inline Integer  operator |  (long  x, const Integer& y) return r
{
  or(x, y, r);
}

inline Integer  operator ^  (const Integer& x, const Integer& y) return r
{
  xor(x, y, r);
}

inline Integer  operator ^  (const Integer& x, long y) return r
{
  xor(x, y, r);
}

inline Integer  operator ^  (long  x, const Integer& y) return r
{
  xor(x, y, r);
}

inline Integer  operator /  (const Integer& x, const Integer& y) return r
{
  div(x, y, r);
}

inline Integer operator /  (const Integer& x, long y) return r
{
  div(x, y, r);
}

inline Integer operator %  (const Integer& x, const Integer& y) return r
{
  mod(x, y, r);
}

inline Integer operator %  (const Integer& x, long y) return r
{
  mod(x, y, r);
}

inline Integer operator <<  (const Integer& x, const Integer& y) return r
{
  lshift(x, y, r);
}

inline Integer operator <<  (const Integer& x, long y) return r
{
  lshift(x, y, r);
}

inline Integer operator >>  (const Integer& x, const Integer& y) return r;
{
  rshift(x, y, r);
}

inline Integer operator >>  (const Integer& x, long y) return r
{
  rshift(x, y, r);
}

inline Integer pow(const Integer& x, long y) return r
{
  pow(x, y, r);
}

inline Integer Ipow(long x, long y) return r(x)
{
  pow(r, y, r);
}

inline Integer pow(const Integer& x, const Integer& y) return r
{
  pow(x, y, r);
}



inline Integer abs(const Integer& x) return r
{
  abs(x, r);
}

inline Integer operator - (const Integer& x) return r
{
  negate(x, r);
}

inline Integer operator ~ (const Integer& x) return r
{
  complement(x, r);
}

inline Integer  atoI(const char* s, int base) return r
{
  r.rep = atoIntRep(s, base);
}

inline Integer  gcd(const Integer& x, const Integer& y) return r
{
  r.rep = gcd(x.rep, y.rep);
}

#else /* NO_NRV */

inline Integer  operator +  (const Integer& x, const Integer& y) 
{
  Integer r; add(x, y, r); return r;
}

inline Integer  operator +  (const Integer& x, long y) 
{
  Integer r; add(x, y, r); return r;
}

inline Integer  operator +  (long  x, const Integer& y) 
{
  Integer r; add(x, y, r); return r;
}

inline Integer  operator -  (const Integer& x, const Integer& y) 
{
  Integer r; sub(x, y, r); return r;
}

inline Integer  operator -  (const Integer& x, long y) 
{
  Integer r; sub(x, y, r); return r;
}

inline Integer  operator -  (long  x, const Integer& y) 
{
  Integer r; sub(x, y, r); return r;
}

inline Integer  operator *  (const Integer& x, const Integer& y) 
{
  Integer r; mul(x, y, r); return r;
}

inline Integer  operator *  (const Integer& x, long y) 
{
  Integer r; mul(x, y, r); return r;
}

inline Integer  operator *  (long  x, const Integer& y) 
{
  Integer r; mul(x, y, r); return r;
}

inline Integer sqr(const Integer& x) 
{
  Integer r; mul(x, x, r); return r;
}

inline Integer  operator &  (const Integer& x, const Integer& y) 
{
  Integer r; and(x, y, r); return r;
}

inline Integer  operator &  (const Integer& x, long y) 
{
  Integer r; and(x, y, r); return r;
}

inline Integer  operator &  (long  x, const Integer& y) 
{
  Integer r; and(x, y, r); return r;
}

inline Integer  operator |  (const Integer& x, const Integer& y) 
{
  Integer r; or(x, y, r); return r;
}

inline Integer  operator |  (const Integer& x, long y) 
{
  Integer r; or(x, y, r); return r;
}

inline Integer  operator |  (long  x, const Integer& y) 
{
  Integer r; or(x, y, r); return r;
}

inline Integer  operator ^  (const Integer& x, const Integer& y) 
{
  Integer r; xor(x, y, r); return r;
}

inline Integer  operator ^  (const Integer& x, long y) 
{
  Integer r; xor(x, y, r); return r;
}

inline Integer  operator ^  (long  x, const Integer& y) 
{
  Integer r; xor(x, y, r); return r;
}

inline Integer  operator /  (const Integer& x, const Integer& y) 
{
  Integer r; div(x, y, r); return r;
}

inline Integer operator /  (const Integer& x, long y) 
{
  Integer r; div(x, y, r); return r;
}

inline Integer operator %  (const Integer& x, const Integer& y) 
{
  Integer r; mod(x, y, r); return r;
}

inline Integer operator %  (const Integer& x, long y) 
{
  Integer r; mod(x, y, r); return r;
}

inline Integer operator <<  (const Integer& x, const Integer& y) 
{
  Integer r; lshift(x, y, r); return r;
}

inline Integer operator <<  (const Integer& x, long y) 
{
  Integer r; lshift(x, y, r); return r;
}

inline Integer operator >>  (const Integer& x, const Integer& y) 
{
  Integer r; rshift(x, y, r); return r;
}

inline Integer operator >>  (const Integer& x, long y) 
{
  Integer r; rshift(x, y, r); return r;
}

inline Integer pow(const Integer& x, long y) 
{
  Integer r; pow(x, y, r); return r;
}

inline Integer Ipow(long x, long y) 
{
  Integer r(x); pow(r, y, r); return r;
}

inline Integer pow(const Integer& x, const Integer& y) 
{
  Integer r; pow(x, y, r); return r;
}



inline Integer abs(const Integer& x) 
{
  Integer r; abs(x, r); return r;
}

inline Integer operator - (const Integer& x) 
{
  Integer r; negate(x, r); return r;
}

inline Integer operator ~ (const Integer& x) 
{
  Integer r; complement(x, r); return r;
}

inline Integer  atoI(const char* s, int base) 
{
  Integer r; r.rep = atoIntRep(s, base); return r;
}

inline Integer  gcd(const Integer& x, const Integer& y) 
{
  Integer r; r.rep = gcd(x.rep, y.rep); return r;
}

#endif  /* NO_NRV */
#endif /* !_Integer_h */
