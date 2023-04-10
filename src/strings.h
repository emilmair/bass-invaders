#ifndef STRINGS_H
#define STRINGS_H

// "all my homies hate malloc() and free()"
#define INLINE_DECIMAL8(NUM) (char[]) {dd(NUM, 7)+48, dd(NUM, 6)+48, dd(NUM, 5)+48, dd(NUM, 4)+48, dd(NUM, 3)+48, dd(NUM, 2)+48, dd(NUM, 1)+48, dd(NUM, 0)+48, 0}
#define INLINE_DECIMAL4(NUM) (char[]) {dd(NUM, 3)+48, dd(NUM, 2)+48, dd(NUM, 1)+48, dd(NUM, 0)+48, 0}
#define INLINE_DECIMAL3(NUM) (char[]) {dd(NUM, 2)+48, dd(NUM, 1)+48, dd(NUM, 0)+48, 0}
#define INLINE_DECIMAL2(NUM) (char[]) {dd(NUM, 1)+48, dd(NUM, 0)+48, 0}
#define INLINE_DECIMAL1(NUM) (char[]) {dd(NUM, 0)+48, 0}

#endif //STRINGS_H
