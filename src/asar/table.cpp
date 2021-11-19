#include <cstdlib>
#include <cstdint>
#include <cstring>
#include "table.h"

table::table() {
	memset(data, 0, sizeof(data));
	utf8_mode = false;
}

table::~table() {
	for(int i=0; i<256; i++) {
		if(data[i] != nullptr) {
			for(int j=0; j<256; j++) {
				if(data[i][j] != nullptr) free(data[i][j]);
			}
			free(data[i]);
		}
	}
}

void table::set_val(int off, uint32_t val) {
	if(data[off >> 16] == nullptr) {
		data[off >> 16] = (table_page**)calloc(256,sizeof(void*));
	}
	table_page** thisbank = data[off >> 16];
	if(thisbank[(off >> 8) & 255] == nullptr) {
		thisbank[(off >> 8) & 255] = (table_page*)calloc(1,sizeof(table_page));
	}
	table_page* thispage = thisbank[(off >> 8) & 255];
	int idx = (off & 255) / 32;
	int bit = off % 32;
	thispage->defined[idx] |= 1<<bit;
	thispage->chars[off & 255] = val;
}

int64_t table::get_val(int off) {
	int64_t def = utf8_mode ? off : -1;
	table_page** thisbank = data[off >> 16];
	if(thisbank == nullptr) return def;
	table_page* thispage = thisbank[(off >> 8) & 255];
	if(thispage == nullptr) return def;
	int idx = (off & 255) / 32;
	int bit = off % 32;
	if(((thispage->defined[idx] >> bit) & 1) == 0) return def;
	return thispage->chars[off & 255];
}
