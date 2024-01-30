#pragma once

// Should only be called inside assembleblock.cpp
void asinit_65816();
bool asblock_65816(char** word, int numwords, bool fake, int& outlen);
void asend_65816();
void asinit_spc700();
bool asblock_spc700(char** word, int numwords);
void asend_spc700();
void asinit_superfx();
bool asblock_superfx(char** word, int numwords);
void asend_superfx();
