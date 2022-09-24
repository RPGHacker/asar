#include "libcon.h"

#include <csignal>

#include "libstr.h"
#include "unicode.h"

static const char* progname;
static const char** args;
static int argsleft;
bool libcon_interactive;
static const char* usage;

static volatile bool confirmclose = true;
void libcon_pause() {
    if (confirmclose) {
        confirmclose = false;
#if defined(_WIN32)
        system("pause");
#else
        printf("Press Enter to continue");
        getchar();
#endif
        confirmclose = true;
    }
}

void libcon_badusage() {
    printf("usage: %s %s", progname, usage);
    exit(1);
}

static const char* getarg(bool tellusage, const char* defval = nullptr) {
    if (!argsleft) {
        if (tellusage) libcon_badusage();
        return defval;
    }
    args++;
    argsleft--;
    return args[0];
}

void u8_fgets(char* buffer, int buffer_size, FILE* handle) {
#if defined(windows)
    // RPG Hacker: Using buffer_size * 2 here to account for potential surrogate pairs.
    // The idea is that our buffer here should be able to at least hold the same amount
    // of characters as the old ANSI version would have supported.
    int num_chars = buffer_size * 2;
    wchar_t* w_buf = (wchar_t*)malloc(num_chars * sizeof(wchar_t));
    (void)fgetws(w_buf, num_chars, stdin);
    string u8_str;
    if (utf16_to_utf8(&u8_str, w_buf)) {
        strncpy(buffer, u8_str, buffer_size);
        buffer[buffer_size - 1] = '\0';
    }
    free(w_buf);
#else
    (void)fgets(buffer, buffer_size, handle);
#endif
}

static const char* requirestrfromuser(const char* question, bool filename) {
    confirmclose = false;
    char* rval = (char*)malloc(256);
    *rval = 0;
    while (!strchr(rval, '\n') || *rval == '\n') {
        *rval = 0;
        printf("%s ", question);
        u8_fgets(rval, 250, stdin);
    }
    *strchr(rval, '\n') = 0;
    confirmclose = true;
#ifdef _WIN32
    if (filename && rval[0] == '"' && rval[2] == ':') {
        char* rvalend = strchr(rval, '\0');
        if (rvalend[-1] == '"') rvalend[-1] = '\0';
        return rval + 1;
    }
#endif
    return rval;
}

static const char* requeststrfromuser(const char* question, bool filename,
                                      const char* defval) {
    confirmclose = false;
    char* rval = (char*)malloc(256);
    *rval = 0;
    printf("%s ", question);
    u8_fgets(rval, 250, stdin);
    char* eol = strchr(rval, '\n');
    if (!eol) {
        printf("Unexpected end of input");
        exit(-1);
    }
    *eol = 0;
    confirmclose = true;
    if (!*rval) return defval;
#ifdef _WIN32
    if (filename && rval[0] == '"' && rval[2] == ':') {
        char* rvalend = strchr(rval, '\0');
        if (rvalend[-1] == '"') rvalend[-1] = '\0';
        return rval + 1;
    }
#endif
    return rval;
}

void libcon_init(int argc, const char** argv, const char* usage_) {
    progname = argv[0];
    args = argv;
    argsleft = argc - 1;
    usage = usage_;
    libcon_interactive = (!argsleft);
#if defined(_WIN32)
    if (libcon_interactive) atexit(libcon_pause);
#endif
}

const char* libcon_require_filename(const char* desc) {
    if (libcon_interactive)
        return requirestrfromuser(desc, true);
    else
        return getarg(true);
}

const char* libcon_optional(const char* desc, const char* defval) {
    if (libcon_interactive)
        return requeststrfromuser(desc, false, defval);
    else
        return getarg(false, defval);
}

const char* libcon_optional_filename(const char* desc, const char* defval) {
    if (libcon_interactive)
        return requeststrfromuser(desc, true, defval);
    else
        return getarg(false, defval);
}

const char* libcon_option() {
    if (!libcon_interactive && argsleft && args[1][0] == '-') return getarg(false);
    return nullptr;
}

const char* libcon_option_value() {
    if (!libcon_interactive) return getarg(false);
    return nullptr;
}

bool libcon_question_bool(const char* desc, bool defval) {
    if (!libcon_interactive) return defval;
    while (true) {
        const char* answer = requeststrfromuser(desc, false, defval ? "y" : "n");
        if (!stricmp(answer, "y") || !stricmp(answer, "yes")) return true;
        if (!stricmp(answer, "n") || !stricmp(answer, "no")) return false;
    }
}

void libcon_end() {
    if (!libcon_interactive && argsleft) libcon_badusage();
}
