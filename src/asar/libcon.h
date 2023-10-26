#pragma once
void libcon_init(int argc, const char * argv[], const char * usage);
const char * libcon_require_filename(const char * desc);
const char * libcon_optional(const char * desc, const char * defval);
const char * libcon_optional_filename(const char * desc, const char * defval);
const char * libcon_option();
const char * libcon_option_value();
bool libcon_question_bool(const char * desc, bool defval);
void libcon_end();
void libcon_badusage();
void libcon_pause();
extern bool libcon_interactive;
