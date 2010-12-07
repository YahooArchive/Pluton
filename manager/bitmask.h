#ifndef _BITMASK_H
#define _BITMASK_H

class bitmask {
 public:
  bitmask() : bits(0) {};
  void	set(unsigned int newBit) { bits |= newBit; }
  void	clear(unsigned int newBit) { bits &= ~newBit; }
  bool	isSet(unsigned int newBit) const { return bits & newBit; }
  bool	isAny() const { return bits != 0; }

  typedef struct {
    const char* 	name;
    unsigned int	bitNumber;
  } nameBitMap;

  bool	setByName(const char *, nameBitMap*);
  bool	clearByName(const char *, nameBitMap*);

 private:
  unsigned long	bits;
};

#endif
