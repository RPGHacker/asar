// data structures for the "table" command

#include <cstdint>

class table_page {
public:
    uint32_t chars[256];
    // bit mask of defined entries
    uint32_t defined[8];
};

class table {
public:
    table();
    table(const table& from);
    table& operator=(const table& from);
    void set_val(int off, uint32_t val);
    // returns either the 32-bit unsigned value or -1 if that codepoint isn't in the
    // table
    int64_t get_val(int off);
    ~table();
    // if set, each undefined char goes to its unicode codepoint
    bool utf8_mode;

private:
    table_page** data[256];
    void clear();
    void copy_from(const table& from);
};

extern table thetable;
