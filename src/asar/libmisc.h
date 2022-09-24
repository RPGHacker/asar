#pragma once
inline int min(int a, int b) { return a > b ? b : a; }

inline unsigned bitround(unsigned v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
