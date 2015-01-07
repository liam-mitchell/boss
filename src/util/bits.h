#ifndef __BITS_H_
#define __BITS_H_

#define SET_BITS(bits, mask) ((bits) |= (mask))
#define CLR_BITS(bits, mask) ((bits) &= ~(mask))
#define TST_BITS(bits, mask) (((bits) & (mask)) == (mask))

#define SET_BIT(bits, n) SET_BITS((bits), (1 << (n)))
#define CLR_BIT(bits, n) CLR_BITS((bits), (1 << (n)))
#define TST_BIT(bits, n) TST_BITS((bits), (1 << (n)))

#endif
