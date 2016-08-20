#ifndef INCLUDE_FLAGS_H
#define INCLUDE_FLAGS_H

#define flag_set(flags, f) \
    *(flags) |= (1 << (f))

#define flag_clear(flags, f) \
    *(flags) &= ~(1 << (f))

#define flag_test(flags, f) \
    (*(flags) & (1 << (f)))

#endif
