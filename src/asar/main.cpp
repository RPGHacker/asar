// "because satanism is best defeated by summoning a bigger satan"
//   ~Alcaro, 2019 (discussing Asar)
#include "addr2line.h"
#include "asar.h"
#include "virtualfile.h"
#include "platform/file-helpers.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "macro.h"
#include <ctime>
// randomdude999: remember to also update the .rc files (in res/windows/) when changing this.
// Couldn't find a way to automate this without shoving the version somewhere in the CMake files
const int asarver_maj=2;
const int asarver_min=0;
const int asarver_bug=0;
const bool asarver_beta=true;

#ifdef _I_RELEASE
extern char blockbetareleases[(!asarver_beta)?1:-1];
#endif
#ifdef _I_DEBUG
extern char blockreleasedebug[(asarver_beta)?1:-1];
#endif

unsigned const char * romdata_r;
int romlen_r;

int pass;

int optimizeforbank=-1;
int optimize_dp = optimize_dp_flag::NONE;
int dp_base = 0;
int optimize_address = optimize_address_flag::DEFAULT;

autoarray<callstack_entry> callstack;

bool errored=false;
bool ignoretitleerrors=false;

volatile int recursioncount=0;

virtual_filesystem* filesystem = nullptr;

AddressToLineMapping addressToLineMapping;

int get_version_int()
{
	return asarver_maj * 10000 + asarver_min * 100 + asarver_bug;
}

bool setmapper()
{
	int maxscore=-99999;
	mapper_t bestmap=lorom;
	mapper_t maps[]={lorom, hirom, exlorom, exhirom};
	for (size_t mapid=0;mapid<sizeof(maps)/sizeof(maps[0]);mapid++)
	{
		mapper=maps[mapid];
		int score=0;
		int highbits=0;
		bool foundnull=false;
		for (int i=0;i<21;i++)
		{
			unsigned char c=romdata[snestopc(0x00FFC0+i)];
			if (foundnull && c) score-=4;//according to some documents, NUL terminated names are possible - but they shouldn't appear in the middle of the name
			if (c>=128) highbits++;
			else if (is_upper(c)) score+=3;
			else if (c==' ') score+=2;
			else if (is_digit(c)) score+=1;
			else if (is_lower(c)) score+=1;
			else if (c=='-') score+=1;
			else if (!c) foundnull=true;
			else score-=3;
		}
		if (highbits>0 && highbits<=14) score-=21;//high bits set on some, but not all, bytes = unlikely to be a ROM
		if ((romdata[snestopc(0x00FFDE)]^romdata[snestopc(0x00FFDC)])!=0xFF ||
				(romdata[snestopc(0x00FFDF)]^romdata[snestopc(0x00FFDD)])!=0xFF) score=-99999;//checksum doesn't match up to 0xFFFF? Not a ROM.
		//too lazy to check the real checksum
		if (score>maxscore)
		{
			maxscore=score;
			bestmap=mapper;
		}
	}
	mapper=bestmap;

	//detect oddball mappers
	int mapperbyte=romdata[snestopc(0x00FFD5)];
	int romtypebyte=romdata[snestopc(0x00FFD6)];
	if (mapper==lorom)
	{
		if (mapperbyte==0x23 && (romtypebyte==0x32 || romtypebyte==0x34 || romtypebyte==0x35)) mapper=sa1rom;
	}
	return (maxscore>=0);
}


bool simple_callstacks = true;

// Shortens target_path to a relative path, but only if it resides
// within base_path or a child directory of it.
string shorten_to_relative_path(const char* base_path, const char* target_path)
{
	if (stribegin(target_path, base_path)) target_path += strlen(base_path);
	return target_path;
}

string get_top_level_directory()
{
	string top_level_file_dir;
	for (int i = 0; i < callstack.count; ++i)
	{
		if (callstack[i].type == callstack_entry_type::FILE)
		{
			top_level_file_dir = dir(callstack[i].content);
			break;
		}
	}
	return top_level_file_dir;
}

string generate_call_details_string(const char* current_block, const char* current_call, int indentation, bool add_lines)
{
	string e;
	if (current_block != nullptr || current_call != nullptr)
	{
		string indent;
		if (add_lines) indent += "|";
		for (; indentation > 0; --indentation) indent += " ";

		if (current_block != nullptr) e += STR "\n"+indent+"in block: ["+current_block+"]";
		if (current_call != nullptr) e += STR "\n"+indent+"in macro call: [%"+current_call+"]";
	}
	return e;
}

string get_pretty_filename(const char* current_file)
{
	// RPG Hacker: One could make an argument that we shouldn't shorten paths
	// here, since some IDEs support jumping to files by double-clicking their
	// paths. However, AFAIK, no IDE supports this for Asar yet, and if it's
	// ever desired, we could just make it a command line option. Until then,
	// I think it's more important to optimize for pretty command line display.
	return shorten_to_relative_path(get_top_level_directory(), current_file);
}

string generate_filename_and_line(const char* current_file, int current_line_no)
{
	return STR current_file
		+ (current_line_no>=0?STR ":"+dec(current_line_no+1):"");
}

string format_stack_line(const printable_callstack_entry& entry, int stack_frame_index)
{
	string indent = "\n|   ";
	indent += dec(stack_frame_index);
	indent += ": ";
	// RPG Hacker: We'll probably never have a call stack in the
	// hundreds even, so this very specific, lazy solution suffices.
	if (stack_frame_index < 100) indent += " ";
	if (stack_frame_index < 10) indent += " ";
	return indent
		+ generate_filename_and_line(entry.prettypath, entry.lineno)
		+ entry.details;
}

void push_stack_line(autoarray<printable_callstack_entry>* out, const char* current_file, const char* current_block, const char* current_call, int current_line_no, int indentation, bool add_lines)
{
	printable_callstack_entry new_entry;
	new_entry.fullpath = current_file;
	new_entry.prettypath = get_pretty_filename(current_file);
	new_entry.lineno = current_line_no;
	new_entry.details = generate_call_details_string(current_block, current_call, indentation, add_lines).raw();
	out->append(new_entry);
}

void get_current_line_details(string* location, string* details, bool exclude_block)
{
	const char* current_file = nullptr;
	const char* current_block = nullptr;
	const char* current_call = nullptr;
	int current_line_no = -1;
	for (int i = callstack.count-1; i >= 0 ; --i)
	{
		switch (callstack[i].type)
		{
			case callstack_entry_type::FILE:
				current_file = callstack[i].content;
				if (exclude_block) current_block = nullptr;
				*location = generate_filename_and_line(get_pretty_filename(current_file), current_line_no);
				*details = generate_call_details_string(current_block, current_call, 4, false);
				return;
			case callstack_entry_type::MACRO_CALL:
				if (current_call == nullptr) current_call = callstack[i].content;
				break;
			case callstack_entry_type::LINE:
				if (current_block == nullptr && current_call == nullptr) current_block = callstack[i].content;
				if (current_line_no == -1) current_line_no = callstack[i].lineno;
				break;
			case callstack_entry_type::BLOCK:
				if (current_block == nullptr) current_block = callstack[i].content;
				break;
		}
	}
	*location = "";
	*details = "";
}

void get_full_printable_callstack(autoarray<printable_callstack_entry>* out, int indentation, bool add_lines)
{
	out->reset();
	const char* current_file = nullptr;
	const char* current_block = nullptr;
	const char* current_call = nullptr;
	int current_line_no = -1;
	for (int i = 0; i < callstack.count; ++i)
	{
		switch (callstack[i].type)
		{
			case callstack_entry_type::FILE:
				if (current_file != nullptr)
				{
					push_stack_line(out, current_file, current_block, current_call, current_line_no, indentation, add_lines);
				}
				current_file = callstack[i].content;
				current_block = nullptr;
				current_call = nullptr;
				current_line_no = -1;
				break;
			case callstack_entry_type::MACRO_CALL:
				current_block = nullptr;
				current_call = callstack[i].content;
				break;
			case callstack_entry_type::LINE:
				current_line_no = callstack[i].lineno;
				current_block = callstack[i].content;
				break;
			case callstack_entry_type::BLOCK:
				current_block = callstack[i].content;
				break;
		}
	}
}

string get_full_callstack()
{
	autoarray<printable_callstack_entry> printable_stack;
	get_full_printable_callstack(&printable_stack, 12, true);

	string e;
	if (printable_stack.count > 0)
	{
		e += "\nFull call stack:";
		for (int i = printable_stack.count-1; i >= 0; --i)
		{
			e += format_stack_line(printable_stack[i], i);
		}
	}
	return e;
}

// RPG Hacker: This function essetially replicates classic Asar behavior
// of only printing a single macro call below the current level.
string get_simple_callstack()
{
	int i;
	const char* current_call = nullptr;
	for (i = callstack.count-1; i >= 0 ; --i)
	{
		if (callstack[i].type == callstack_entry_type::MACRO_CALL)
		{
			current_call = callstack[i].content;
			break;
		}
	}

	const char* current_file = nullptr;
	int current_line_no = -1;
	if (current_call != nullptr)
	{
		bool stop = false;
		for (int j = i-1; j >= 0 ; --j)
		{
			switch (callstack[j].type)
			{
				case callstack_entry_type::FILE:
					if (current_file != nullptr)
					{
						stop = true;
						break;
					}
					current_file = callstack[j].content;
					break;
				case callstack_entry_type::MACRO_CALL:
					stop = true;
					break;
				case callstack_entry_type::LINE:
					if (current_line_no == -1) current_line_no = callstack[j].lineno;
					break;
				case callstack_entry_type::BLOCK:
					break;
			}

			if (current_file != nullptr && current_line_no != -1) stop = true;

			if (stop) break;
		}
	}

	string e;
	if (current_call != nullptr && current_file != nullptr)
	{
		e += STR "\n    called from: " + generate_filename_and_line(get_pretty_filename(current_file), current_line_no)
			+ ": [%" + current_call + "]";
	}
	return e;
}

string get_callstack()
{
	if (simple_callstacks)
		return get_simple_callstack();
	else
		return get_full_callstack();
}

asar_error_id vfile_error_to_error_id(virtual_file_error vfile_error)
{
	switch (vfile_error)
	{
	case vfe_doesnt_exist:
		return error_id_file_not_found;
	case vfe_access_denied:
		return error_id_failed_to_open_file_access_denied;
	case vfe_not_regular_file:
		return error_id_failed_to_open_not_regular_file;
	case vfe_unknown:
	case vfe_none:
	case vfe_num_errors:
		return error_id_failed_to_open_file;
	}

	return error_id_failed_to_open_file;
}

virtual_file_error asar_get_last_io_error()
{
	if (filesystem != nullptr)
	{
		return filesystem->get_last_error();
	}

	return vfe_unknown;
}

static bool freespaced;
static int getlenforlabel(snes_label thislabel, bool exists)
{
	unsigned int bank = thislabel.pos>>16;
	unsigned int word = thislabel.pos&0xFFFF;
	unsigned int relaxed_bank = optimizeforbank < 0 ? 0 : optimizeforbank;
	if (!exists)
	{
		return 2;
	}
	else if((optimize_dp == optimize_dp_flag::RAM) && bank == 0x7E && (word-dp_base < 0x100))
	{
		return 1;
	}
	else if(optimize_dp == optimize_dp_flag::ALWAYS && (bank == 0x7E || !(bank & 0x40)) && (word-dp_base < 0x100))
	{
		return 1;
	}
	else if (optimize_address == optimize_address_flag::RAM && bank == 0x7E && word < 0x2000)
	{
		return 2;
	}
	else if (optimize_address == optimize_address_flag::MIRRORS && (bank == relaxed_bank || (!(bank & 0x40) && !(relaxed_bank & 0x40))) && word < 0x2000)
	{
		return 2;
	}
	else if (optimize_address == optimize_address_flag::MIRRORS && !(bank & 0x40) && !(relaxed_bank & 0x40) && word < 0x8000)
	{
		return 2;
	}
	else if (optimizeforbank>=0)
	{
		if (thislabel.freespace_id > 0) return 3;
		else if (bank==(unsigned int)optimizeforbank) return 2;
		else return 3;
	}
	else if (thislabel.freespace_id > 0 || freespaceid > 0)
	{
		// TODO: check whether they're pinned to the same bank
		if (thislabel.freespace_id != freespaceid) return 3;
		else return 2;
	}
	else if ((int)bank != snespos >> 16){ return 3; }
	else { return 2;}
}


bool is_hex_constant(const char* str){
	if (*str=='$')
	{
		str++;
		while(is_xdigit(*str)) {
			str++;
		}
		if(*str=='\0'){
			return true;
		}
	}
	return false;
}

int getlen(const char * orgstr, bool optimizebankextraction)
{
	const char * str=orgstr;
	freespaced=false;

	const char* posneglabel = str;
	string posnegname = posneglabelname(&posneglabel, false);

	if (posnegname.length() > 0)
	{
		if (*posneglabel != '\0') goto notposneglabel;

		if (!pass) return 2;
		snes_label label_data;
		// RPG Hacker: Umm... what kind of magic constant is this?
		label_data.pos = 31415926;
		bool found = labelval(posnegname, &label_data);
		return getlenforlabel(label_data, found);
	}
notposneglabel:
	int len=0;
	while (*str)
	{
		int thislen=0;
		bool maybebankextraction=(str==orgstr);
		if (*str=='$')
		{
			str++;
			int i;
			for (i=0;is_xdigit(str[i]);i++);
			//if (i&1) warn(S dec(i)+"-digit hex value");//blocked in getnum instead
			thislen=(i+1)/2;
			str+=i;
		}
		else if (*str=='%')
		{
			str++;
			int i;
			for (i=0;str[i]=='0' || str[i]=='1';i++);
			//if (i&7) warn(S dec(i)+"-digit binary value");
			thislen=(i+7)/8;
			str+=i;
		}
		else if (str[0]=='\'' && str[2]=='\'')
		{
			thislen=1;
			str+=3;
		}
		else if (is_digit(*str))
		{
			int val=strtol(str, const_cast<char**>(&str), 10);
			if (val>=0) thislen=1;
			if (val>=256) thislen=2;
			if (val>=65536) thislen=3;
		}
		else if (is_ualpha(*str) || *str=='.' || *str=='?')
		{
			snes_label thislabel;
			bool exists=labelval(&str, &thislabel);
			thislen=getlenforlabel(thislabel, exists);
		}
		else str++;
		if (optimizebankextraction && maybebankextraction &&
				(!strcmp(str, ">>16") || !strcmp(str, "/65536") || !strcmp(str, "/$10000")))
					return 1;
		if (thislen>len) len=thislen;
	}
	return len;
}

struct strcompare {
	bool operator() (const char * lhs, const char * rhs) const
	{
		return strcmp(lhs, rhs)<0;
	}
};

struct stricompare {
	bool operator() (const char * lhs, const char * rhs) const
	{
		return stricmp(lhs, rhs)<0;
	}
};

struct sourcefile {
	char *data;
	char** contents;
	int numlines;
};

static assocarr<sourcefile> filecontents;
assocarr<string> defines;
// needs to be separate because defines is reset between parsing arguments and patching
assocarr<string> clidefines;
assocarr<string> builtindefines;

bool validatedefinename(const char * name)
{
	if (!name[0]) return false;
	for (int i = 0;name[i];i++)
	{
		if (!is_ualnum(name[i])) return false;
	}

	return true;
}

void resolvedefines(string& out, const char * start)
{
	recurseblock rec;
	const char * here=start;
	if (!strchr(here, '!'))
	{
		out += here;
		return;
	}
	while (*here)
	{
		if (here[0] == '\\' && here[1] == '\\')
		{
			// allow using \\ as escape sequence
			if (in_macro_def > 0) out += "\\";
			out += "\\";
			here += 2;
		}
		else if (here[0] == '\\' && here[1] == '!')
		{
			// allow using \! to escape !
			if (in_macro_def > 0) out += "\\";
			out+="!";
			here += 2;
		}
		else if (*here=='!')
		{
			bool first=(here==start || (here>=start+4 && here[-1]==' ' && here[-2]==':' && here[-3]==' '));//check if it's the start of a block
			string defname;
			here++;

			int depth = 0;
			for (const char* depth_str = here; *depth_str=='^'; depth_str++)
			{
				depth++;
			}
			here += depth;

			if (depth != in_macro_def)
			{
				out += '!';
				for (int i=0; i < depth; ++i) out += '^';
				if (depth > in_macro_def) asar_throw_error(0, error_type_line, error_id_invalid_depth_resolve, "define", "define", depth, in_macro_def);
				continue;
			}

			if (*here=='{')
			{
				here++;
				string unprocessedname;
				int braces=1;
				while (true)
				{
					if (*here=='{') braces++;
					if (*here=='}') braces--;
					if (!*here) asar_throw_error(0, error_type_line, error_id_mismatched_braces);
					if (!braces) break;
					unprocessedname+=*here++;
				}
				here++;
				resolvedefines(defname, unprocessedname);
				if (!validatedefinename(defname)) asar_throw_error(0, error_type_line, error_id_invalid_define_name);
			}
			else
			{
				while (is_ualnum(*here)) defname+=*here++;
			}

			if (first)
			{
				enum {
					null,
					append,
					expand,
					domath,
					setifnotset,
				} mode;
				if(0);
				else if (stribegin(here,  " = ")) { here+=3; mode=null; }
				else if (stribegin(here, " += ")) { here+=4; mode=append; }
				else if (stribegin(here, " := ")) { here+=4; mode=expand; }
				else if (stribegin(here, " #= ")) { here+=4; mode=domath; }
				else if (stribegin(here, " ?= ")) { here+=4; mode=setifnotset; }
				else goto notdefineset;
				string val;
				if (*here=='"')
				{
					here++;
					while (true)
					{
						if (*here=='"')
						{
							if (!here[1] || here[1]==' ') break;
							else if (here[1]=='"') here++;
							else asar_throw_error(0, error_type_line, error_id_broken_define_declaration);
						}
						val+=*here++;
					}
					here++;
				}
				else
				{
					while (*here && *here!=' ') val+=*here++;
				}
				//if (strqchr(val.data(), ';')) *strqchr(val.data(), ';')=0;
				if (*here && !stribegin(here, " : ")) asar_throw_error(0, error_type_line, error_id_broken_define_declaration);
				// RPG Hacker: Is it really a good idea to normalize
				// the content of defines? That kinda violates their
				// functionality as a string replacement mechanism.
				val.qnormalize();

				// RPG Hacker: throw an error if we're trying to overwrite built-in defines.
				if (builtindefines.exists(defname))
				{
					asar_throw_error(0, error_type_line, error_id_overriding_builtin_define, defname.data());
				}

				switch (mode)
				{
					case null:
					{
						defines.create(defname) = val;
						break;
					}
					case append:
					{
						if (!defines.exists(defname)) asar_throw_error(0, error_type_line, error_id_define_not_found, defname.data());
						string oldval = defines.find(defname);
						val=oldval+val;
						defines.create(defname) = val;
						break;
					}
					case expand:
					{
						string newval;
						resolvedefines(newval, val);
						defines.create(defname) = newval;
						break;
					}
					case domath:
					{
						string newval;
						resolvedefines(newval, val);
						double num= getnumdouble(newval);
						if (foundlabel && !foundlabel_static) asar_throw_error(0, error_type_line, error_id_define_label_math);
						defines.create(defname) = ftostr(num);
						break;
					}
					case setifnotset:
					{
						if (!defines.exists(defname)) defines.create(defname) = val;
						break;
					}
				}
			}
			else
			{
			notdefineset:
				if (!defname) out+="!";
				else
				{
					if (!defines.exists(defname)) asar_throw_error(0, error_type_line, error_id_define_not_found, defname.data());
					else {
						string thisone = defines.find(defname);
						resolvedefines(out, thisone);
					}
				}
			}
		}
		else out+=*here++;
	}
	if (!confirmquotes(out)) { asar_throw_error(0, error_type_null, error_id_mismatched_quotes); out = ""; }
}

bool moreonline;
bool asarverallowed = false;

void assembleline(const char * fname, int linenum, const char * line, int& single_line_for_tracker)
{
	recurseblock rec;
	bool moreonlinetmp=moreonline;
	// randomdude999: redundant, assemblefile already converted the path to absolute
	//string absolutepath = filesystem->create_absolute_path("", fname);
	string absolutepath = fname;
	single_line_for_tracker = 1;
	try
	{
		string out=line;
		out.qreplace(": :", ":  :");
		autoptr<char**> blocks=qsplitstr(out.temp_raw(), " : ");
		moreonline=true;
		for (int block=0;moreonline;block++)
		{
			moreonline=(blocks[block+1] != nullptr);
			try
			{
				string stripped_block = strip_whitespace(blocks[block]);

				callstack_push cs_push(callstack_entry_type::BLOCK, stripped_block);

				assembleblock(stripped_block, single_line_for_tracker);
				checkbankcross();
			}
			catch (errblock&) {}
			if (blocks[block][0]!='\0') asarverallowed=false;
			if(single_line_for_tracker == 1) single_line_for_tracker = 0;
		}
	}
	catch (errline&) {}
	moreonline=moreonlinetmp;
}

int incsrcdepth=0;

// Returns true if a file is protected via
// an "includeonce".
bool file_included_once(const char* file)
{
	for (int i = 0; i < includeonce.count; ++i)
	{
		if (includeonce[i] == file)
		{
			return true;
		}
	}

	return false;
}

autoarray<string> macro_defs;
int in_macro_def=0;

void assemblefile(const char * filename)
{
	incsrcdepth++;
	string absolutepath = filesystem->create_absolute_path(get_current_file_name(), filename);

	if (file_included_once(absolutepath))
	{
		return;
	}

	callstack_push cs_push(callstack_entry_type::FILE, absolutepath);

	sourcefile file;
	file.contents = nullptr;
	file.numlines = 0;
	int startif=numif;
	if (!filecontents.exists(absolutepath))
	{
		char * temp = readfile(absolutepath, "");
		if (!temp)
		{
			asar_throw_error(0, error_type_null, vfile_error_to_error_id(asar_get_last_io_error()), filename);

			return;
		}
		sourcefile& newfile = filecontents.create(absolutepath);
		newfile.contents =split(temp, '\n');
		newfile.data = temp;
		bool in_block_comment = false;
		int block_comment_start = -1;
		string block_comment_start_line;
		for (int i=0;newfile.contents[i];i++)
		{
			newfile.numlines++;
			char * line= newfile.contents[i];
			if(in_block_comment) {
				char * end = strstr(line, "]]");
				if(!end) {
					*line = 0;
					continue;
				}
				line = end+2;
				in_block_comment = false;
			}
redo_line:
			char * comment = strqchr(line, ';');
			if(comment) {
				if(comment[1] == '[' && comment[2] == '[') {
					in_block_comment = true;
					block_comment_start = i;
					block_comment_start_line = line;
					// this is a little messy because a multiline comment could
					// end right on the line where it started. so if we find
					// the end, cut the comment out of the line and recheck for
					// more comments.
					char * end = strstr(line, "]]");
					if(end) {
						memmove(comment, end+2, strlen(end+2)+1);
						in_block_comment = false;
						goto redo_line;
					}
				}
				*comment = 0;
			}
			if (!confirmquotes(line)) { callstack_push cs_push(callstack_entry_type::LINE, line, i); asar_throw_error(0, error_type_null, error_id_mismatched_quotes); line[0] = '\0'; }
			newfile.contents[i] = strip_whitespace(line);
		}
		if(in_block_comment) {
			callstack_push cs_push(callstack_entry_type::LINE, block_comment_start_line, block_comment_start);
			asar_throw_error(0, error_type_null, error_id_unclosed_block_comment);
		}
		for(int i=0;newfile.contents[i];i++)
		{
			char* line = newfile.contents[i];
			if(!*line) continue;
			for (int j=1;line[strlen(line) - 1] == ',' && newfile.contents[i+j];j++)
			{
				// not using strcat because the source and dest overlap here
				char* otherline = newfile.contents[i+j];
				char* line_end = line + strlen(line);
				while(*otherline) *line_end++ = *otherline++;
				*line_end = '\0';
				static char nullstr[]="";
				newfile.contents[i+j]=nullstr;
			}
		}
		file = newfile;
	} else { // filecontents.exists(absolutepath)
		file = filecontents.find(absolutepath);
	}
	asarverallowed=true;
	for (int i=0;file.contents[i] && i<file.numlines;i++)
	{
		string connectedline;
		int skiplines = getconnectedlines<char**>(file.contents, i, connectedline);

		bool was_loop_end = do_line_logic(connectedline, absolutepath, i);
		i += skiplines;

		// if a loop ended on this line, should it run again?
		if (was_loop_end && whilestatus[numif].cond)
			i = whilestatus[numif].startline - 1;
	}
	while (in_macro_def > 0)
	{
		asar_throw_error(0, error_type_null, error_id_unclosed_macro, macro_defs[in_macro_def-1].data());
		if (!pass && in_macro_def == 1) endmacro(false);
		in_macro_def--;
		macro_defs.remove(in_macro_def);
	}
	if (numif!=startif)
	{
		numif=startif;
		numtrue=startif;
		asar_throw_error(0, error_type_null, error_id_unclosed_if);
	}
	incsrcdepth--;
}

// RPG Hacker: At some point, this should probably be merged
// into assembleline(), since the two names just cause
// confusion otherwise.
// return value is "did a loop end on this line"
bool do_line_logic(const char* line, const char* filename, int lineno)
{
	int prevnumif = numif;
	int single_line_for_tracker = 1;
	try
	{
		string current_line;
		if (numif==numtrue || (numtrue+1==numif && stribegin(line, "elseif ")))
		{
			callstack_push cs_push(callstack_entry_type::LINE, line, lineno);
			string tmp=replace_macro_args(line);
			tmp.qnormalize();
			resolvedefines(current_line, tmp);
		}
		else current_line=line;

		callstack_push cs_push(callstack_entry_type::LINE, current_line, lineno);

		if (stribegin(current_line, "macro ") && numif==numtrue)
		{
			// RPG Hacker: Slight redundancy here with code that is
			// also in startmacro(). Could improve this for Asar 2.0.
			string macro_name = current_line.data()+6;
			char * startpar=strqchr(macro_name.data(), '(');
			if (startpar) *startpar=0;
			macro_defs.append(macro_name);

			// RPG Hacker: I think it would make more logical sense
			// to have this ++ after the if, but hat breaks compatibility
			// with at least one test, and it generally leads to more
			// errors being output after a broken macro declaration.
			in_macro_def++;
			if (!pass)
			{
				if (in_macro_def == 1) startmacro(current_line.data()+6);
				else tomacro(current_line);
			}
		}
		else if (!stricmp(current_line, "endmacro") && numif==numtrue)
		{
			if (in_macro_def == 0) asar_throw_error(0, error_type_line, error_id_misplaced_endmacro);
			else
			{
				in_macro_def--;
				macro_defs.remove(in_macro_def);
				if (!pass)
				{
					if (in_macro_def == 0) endmacro(true);
					else tomacro(current_line);
				}
			}
		}
		else if (in_macro_def > 0)
		{
			if (!pass) tomacro(current_line);
		}
		else
		{
			assembleline(filename, lineno, current_line, single_line_for_tracker);
		}
	}
	catch (errline&) {}
	return (numif != prevnumif || single_line_for_tracker == 3)
		&& (whilestatus[numif].iswhile || whilestatus[numif].is_for);
}


void parse_std_includes(const char* textfile, autoarray<string>& outarray)
{
	char* content = readfilenative(textfile);

	if (content != nullptr)
	{
		char* pos = content;

		while (pos[0] != '\0')
		{
			string stdinclude;

			do
			{
				if (pos[0] != '\r' && pos[0] != '\n')
				{
					stdinclude += pos[0];
				}
				pos++;
			} while (pos[0] != '\0' && pos[0] != '\n');

			strip_whitespace(stdinclude);

			if (stdinclude != "")
			{
				if (!path_is_absolute(stdinclude))
				{
					stdinclude = dir(textfile) + stdinclude;
				}
				outarray.append(normalize_path(stdinclude));
			}
		}

		free(content);
	}
}

void parse_std_defines(const char* textfile)
{

	// RPG Hacker: add built-in defines.
	// (They're not really standard defines, but I was lazy and this was
	// one convenient place for doing it).
	builtindefines.create("assembler") = "asar";
	builtindefines.create("assembler_ver") = dec(get_version_int());
	builtindefines.create("assembler_time") = dec(time(nullptr));

	if(textfile == nullptr) return;

	char* content = readfilenative(textfile);

	if (content != nullptr)
	{
		char* pos = content;
		while (*pos != 0) {
			string define_name;
			string define_val;

			while (*pos != '=' && *pos != '\n') {
				define_name += *pos;
				pos++;
			}
			if (*pos != 0 && *pos != '\n') pos++; // skip =
			while (*pos != 0 && *pos != '\n') {
				define_val += *pos;
				pos++;
			}
			if (*pos != 0)
				pos++; // skip \n
			// clean define_name
			strip_whitespace(define_name);
			define_name.strip_prefix('!'); // remove leading ! if present

			if (define_name == "")
			{
				if (define_val == "")
				{
					continue;
				}

				asar_throw_error(pass, error_type_null, error_id_stddefines_no_identifier);
			}

			if (!validatedefinename(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_invalid, "stddefines.txt", define_name.data());

			// clean define_val
			const char* defval = define_val.data();
			string cleaned_defval;

			if (*defval == 0) {
				// no value
				if (clidefines.exists(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Std define", define_name.data());
				clidefines.create(define_name) = "";
				continue;
			}

			while (*defval == ' ' || *defval == '\t') defval++; // skip whitespace in beginning
			if (*defval == '"') {
				defval++; // skip opening quote
				while (*defval != '"' && *defval != 0)
					cleaned_defval += *defval++;

				if (*defval == 0) {
					asar_throw_error(pass, error_type_null, error_id_mismatched_quotes);
				}
				defval++; // skip closing quote
				while (*defval == ' ' || *defval == '\t') defval++; // skip whitespace
				if (*defval != 0 && *defval != '\n')
					asar_throw_error(pass, error_type_null, error_id_stddefine_after_closing_quote);

				if (clidefines.exists(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Std define", define_name.data());
				clidefines.create(define_name) = cleaned_defval;
				continue;
			}
			else
			{
				// slightly hacky way to remove trailing whitespace
				const char* defval_end = strchr(defval, '\n'); // slightly hacky way to get end of string or newline
				if (!defval_end) defval_end = strchr(defval, 0);
				defval_end--;
				while (*defval_end == ' ' || *defval_end == '\t') defval_end--;
				cleaned_defval = string(defval, (int)(defval_end - defval + 1));

				if (clidefines.exists(define_name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Std define", define_name.data());
				clidefines.create(define_name) = cleaned_defval;
				continue;
			}

		}
		free(content);
	}
}

bool checksum_fix_enabled = true;
bool force_checksum_fix = false;

#define cfree(x) free((void*)x)
static void clearmacro(const string & key, macrodata* & macro)
{
	(void)key;
	freemacro(macro);
}

static void clearfile(const string & key, sourcefile& filecontent)
{
	(void)key;
	cfree(filecontent.data);
	cfree(filecontent.contents);
}
#undef cfree

static void adddefine(const string & key, string & value)
{
	if (!defines.exists(key)) defines.create(key) = value;
}

static string symbolfile;

static void printsymbol_wla(const string& key, snes_label& label)
{
	string line = hex((label.pos & 0xFF0000)>>16, 2)+":"+hex(label.pos & 0xFFFF, 4)+" "+key+"\n";
	symbolfile += line;
}

static void printsymbol_nocash(const string& key, snes_label& label)
{
	string line = hex(label.pos & 0xFFFFFF, 8)+" "+key+"\n";
	symbolfile += line;
}

string create_symbols_file(string format, uint32_t romCrc){
	format = lower(format);
	symbolfile = "";
	if(format == "wla")
	{
		symbolfile =  "; wla symbolic information file\n";
		symbolfile += "; generated by asar\n";

		symbolfile += "\n[labels]\n";
		labels.each(printsymbol_wla);

		symbolfile += "\n[source files]\n";
		const autoarray<AddressToLineMapping::FileInfo>& addrToLineFileList = addressToLineMapping.getFileList();
		for (int i = 0; i < addrToLineFileList.count; ++i)
		{
			char addrToFileListStr[256];
			snprintf(addrToFileListStr, 256, "%.4x %.8x %s\n",
				i,
				addrToLineFileList[i].fileCrc,
				addrToLineFileList[i].filename.data()
			);
			symbolfile += addrToFileListStr;
		}

		symbolfile += "\n[rom checksum]\n";
		{
			char romCrcStr[32];
			snprintf(romCrcStr, 32, "%.8x\n",
				romCrc
			);
			symbolfile += romCrcStr;
		}

		symbolfile += "\n[addr-to-line mapping]\n";
		const autoarray<AddressToLineMapping::AddrToLineInfo>& addrToLineInfo = addressToLineMapping.getAddrToLineInfo();
		for (int i = 0; i < addrToLineInfo.count; ++i)
		{
			char addrToLineStr[32];
			snprintf(addrToLineStr, 32, "%.2x:%.4x %.4x:%.8x\n",
				(addrToLineInfo[i].addr & 0xFF0000) >> 16,
				addrToLineInfo[i].addr & 0xFFFF,
				addrToLineInfo[i].fileIdx & 0xFFFF,
				addrToLineInfo[i].line & 0xFFFFFFFF
			);
			symbolfile += addrToLineStr;
		}

	}
	else if (format == "nocash")
	{
		symbolfile = ";no$sns symbolic information file\n";
		symbolfile += ";generated by asar\n";
		symbolfile += "\n";
		labels.each(printsymbol_nocash);
	}
	return symbolfile;
}


bool in_top_level_file()
{
	int num_files = 0;
	for (int i = callstack.count-1; i >= 0; --i)
	{
		if (callstack[i].type == callstack_entry_type::FILE)
		{
			num_files++;
			if (num_files > 1) break;
		}
	}
	return (num_files <= 1);
}

const char* get_current_file_name()
{
	for (int i = callstack.count-1; i >= 0; --i)
	{
		if (callstack[i].type == callstack_entry_type::FILE)
			return callstack[i].content.raw();
	}
	return nullptr;
}

int get_current_line()
{
	for (int i = callstack.count-1; i >= 0; --i)
	{
		if (callstack[i].type == callstack_entry_type::LINE) return callstack[i].lineno;
	}
	return -1;
}

const char* get_current_block()
{
	for (int i = callstack.count-1; i >= 0; --i)
	{
		if (callstack[i].type == callstack_entry_type::LINE || callstack[i].type == callstack_entry_type::BLOCK) return callstack[i].content.raw();
	}
	return nullptr;
}


void reseteverything()
{
	string str;
	labels.reset();
	defines.reset();
	builtindefines.each(adddefine);
	clidefines.each(adddefine);
	structs.reset();

	macros.each(clearmacro);
	macros.reset();

	filecontents.each(clearfile);
	filecontents.reset();

	writtenblocks.reset();

	optimizeforbank=-1;
	optimize_dp = optimize_dp_flag::NONE;
	dp_base = 0;
	optimize_address = optimize_address_flag::DEFAULT;

	closecachedfiles();

	incsrcdepth=0;
	label_counter = 0;
	errored = false;
	checksum_fix_enabled = true;
	force_checksum_fix = false;

	in_macro_def = 0;

	#ifndef ASAR_SHARED
		free(const_cast<unsigned char*>(romdata_r));
	#endif

	callstack.reset();
	simple_callstacks = true;
#undef free
}
