// "because satanism is best defeated by summoning a bigger satan"
//   ~Alcaro, 2019 (discussing Asar)
#include "addr2line.h"
#include "asar.h"
#include "unicode.h"
#include "virtualfile.h"
#include "platform/file-helpers.h"
#include "assembleblock.h"
#include "asar_math.h"
#include "macro.h"
#include <algorithm>
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
int optimize_dp = optimize_dp_flag::ALWAYS;
int dp_base = 0;
int optimize_address = optimize_address_flag::MIRRORS;

std::vector<callstack_entry> callstack;

bool errored=false;
bool ignoretitleerrors=false;

int recursioncount=0;

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
static string shorten_to_relative_path(const char* base_path, const char* target_path)
{
	if (stribegin(target_path, base_path)) target_path += strlen(base_path);
	return target_path;
}

static string get_top_level_directory()
{
	string top_level_file_dir;
	for (size_t i = 0; i < callstack.size(); ++i)
	{
		if (callstack[i].type == callstack_entry_type::FILE)
		{
			top_level_file_dir = dir(callstack[i].content);
			break;
		}
	}
	return top_level_file_dir;
}

static string generate_call_details_string(const char* current_block, const char* current_call, int indentation, bool add_lines)
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

static string get_pretty_filename(const char* current_file)
{
	// RPG Hacker: One could make an argument that we shouldn't shorten paths
	// here, since some IDEs support jumping to files by double-clicking their
	// paths. However, AFAIK, no IDE supports this for Asar yet, and if it's
	// ever desired, we could just make it a command line option. Until then,
	// I think it's more important to optimize for pretty command line display.
	return shorten_to_relative_path(get_top_level_directory(), current_file);
}

static string generate_filename_and_line(const char* current_file, int current_line_no)
{
	return STR current_file
		+ (current_line_no>=0?STR ":"+dec(current_line_no+1):"");
}

static string format_stack_line(const printable_callstack_entry& entry, int stack_frame_index)
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

static void push_stack_line(autoarray<printable_callstack_entry>* out, const char* current_file, const char* current_block, const char* current_call, int current_line_no, int indentation, bool add_lines)
{
	printable_callstack_entry new_entry;
	new_entry.fullpath = current_file;
	new_entry.prettypath = get_pretty_filename(current_file);
	new_entry.lineno = current_line_no;
	new_entry.details = generate_call_details_string(current_block, current_call, indentation, add_lines);
	out->append(new_entry);
}

void get_current_line_details(string* location, string* details, bool exclude_block)
{
	const char* current_file = nullptr;
	const char* current_block = nullptr;
	const char* current_call = nullptr;
	int current_line_no = -1;
	for (int i = (int)callstack.size()-1; i >= 0 ; --i)
	{
		switch (callstack[i].type)
		{
			case callstack_entry_type::FILE:
				current_file = callstack[i].content;
				if (exclude_block) current_block = nullptr;
				*location = generate_filename_and_line(get_pretty_filename(current_file), current_line_no);
				*location += ": ";
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
	for (size_t i = 0; i < callstack.size(); ++i)
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

static string get_full_callstack()
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
static string get_simple_callstack()
{
	int i;
	const char* current_call = nullptr;
	for (i = (int)callstack.size()-1; i >= 0 ; --i)
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

void throw_vfile_error(int whichpass, virtual_file_error vfile_error, const char* filename)
{
	switch (vfile_error)
	{
	case vfe_doesnt_exist:
		throw_err_block(whichpass, err_file_not_found, filename);
	case vfe_access_denied:
		throw_err_block(whichpass, err_failed_to_open_file_access_denied, filename);
	case vfe_not_regular_file:
		throw_err_block(whichpass, err_failed_to_open_not_regular_file, filename);
	case vfe_unknown:
	case vfe_none:
	case vfe_num_errors:
		throw_err_block(whichpass, err_failed_to_open_file, filename);
	}

	throw_err_block(whichpass, err_failed_to_open_file, filename);
}

virtual_file_error asar_get_last_io_error()
{
	if (filesystem != nullptr)
	{
		return filesystem->get_last_error();
	}

	return vfe_unknown;
}

int getlenforlabel(int labelpos, int label_fs_id, bool exists)
{
	int bank = labelpos>>16;
	unsigned int word = labelpos&0xFFFF;
	bool lbl_is_freespace = label_fs_id > 0;
	// bank number for mirroring considerations.
	// this is the number of a bank which has the same "layout" as the current bank.
	unsigned int relaxed_bank;
	// nonnegative if we know that DBR is pointing at a certain bank
	int cur_effective_bank = -1;
	if(optimizeforbank >= 0) {
		cur_effective_bank = relaxed_bank = optimizeforbank;
	} else {
		if(freespaceid == 0) {
			cur_effective_bank = relaxed_bank = snespos >> 16;
		} else {
			int target_bank = freespaces[freespaceid].bank;
			if(target_bank == -2) relaxed_bank = 0;
			else if(target_bank == -1) relaxed_bank = 0x40;
			else cur_effective_bank = relaxed_bank = target_bank;
		}
	}
	// hirom has non-mirrored sram in 6000-7fff, so optimize mirrors shouldn't cover it
	unsigned int mirror_bound = (mapper == hirom || mapper == exhirom) ? 0x6000 : 0x8000;

	if(lbl_is_freespace) {
		bank = freespaces[label_fs_id].bank;
	}

	if (!exists)
	{
		return 2;
	}
	else if((optimize_dp == optimize_dp_flag::RAM) && bank == 0x7E && (word-dp_base < 0x100) && !lbl_is_freespace)
	{
		return 1;
	}
	else if(optimize_dp == optimize_dp_flag::ALWAYS && (bank == 0x7E || !(bank & 0x40)) && (word-dp_base < 0x100) && !lbl_is_freespace)
	{
		return 1;
	}
	else if (
		// if we should optimize ram accesses...
		(optimize_address == optimize_address_flag::RAM || optimize_address == optimize_address_flag::MIRRORS)
		// and we're in a bank with ram mirrors... (optimizeforbank=0x7E is checked later)
		&& !(relaxed_bank & 0x40)
		// and the label is in low RAM
		&& bank == 0x7E && word < 0x2000 && !lbl_is_freespace)
	{
		return 2;
	}
	else if (
		// if we should optimize mirrors...
		optimize_address == optimize_address_flag::MIRRORS
		// we're in a bank with ram mirrors...
		&& !(relaxed_bank & 0x40)
		// and the label is in a mirrored section
		&& !(bank & 0x40) && word < mirror_bound && !lbl_is_freespace)
	{
		return 2;
	}
	else if (optimizeforbank>=0)
	{
		// if optimizing for a specific bank:
		// if the label is in freespace, then bank is the freespace's bank, and
		// if that's non-negative, then the freespace is forced to that bank.
		// if the freespace isn't forced to a specific bank, then bank is
		// negative, so this equality will never hold.
		if (bank == optimizeforbank) return 2;
		else return 3;
	}

	// check if the label is pinned to the current freespace.
	// can only be checked after pass 0, as freespace pin targets aren't
	// computed before then.
	// This codepath also handles the case of labels that are in the current freespace.
	if(pass > 0 && lbl_is_freespace && freespaceid > 0) {
		int fs_pin_1 = freespaces[label_fs_id].pin_target_id;
		int fs_pin_2 = freespaces[freespaceid].pin_target_id;
		if(fs_pin_1 == fs_pin_2) return 2;
	}

	if (bank >= 0 && bank == cur_effective_bank) return 2;
	else return 3;
}
int getlenforlabel(snes_label thislabel, bool exists) {
	return getlenforlabel(thislabel.pos, thislabel.freespace_id, exists);
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

struct sourceline {
	char* line;
	int lineno;
};

struct sourcefile {
	char *data;
	autoarray<sourceline> lines;
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
				if (depth > in_macro_def) throw_err_line(0, err_invalid_depth_resolve, "define", "define", depth, in_macro_def);
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
					if (!*here) throw_err_line(0, err_mismatched_braces);
					if (!braces) break;
					unprocessedname+=*here++;
				}
				here++;
				resolvedefines(defname, unprocessedname);
				if (!validatedefinename(defname)) throw_err_line(0, err_invalid_define_name);
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
							else throw_err_line(0, err_broken_define_declaration);
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
				if (*here && !stribegin(here, " : ")) throw_err_line(0, err_broken_define_declaration);
				// RPG Hacker: Is it really a good idea to normalize
				// the content of defines? That kinda violates their
				// functionality as a string replacement mechanism.
				//val.qnormalize();

				// RPG Hacker: throw an error if we're trying to overwrite built-in defines.
				if (builtindefines.exists(defname))
				{
					throw_err_line(0, err_overriding_builtin_define, defname.data());
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
						if (!defines.exists(defname)) throw_err_line(0, err_define_not_found, defname.data());
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
						math_val num = parse_math_expr(newval)->evaluate_static();
						string num_str;
						if(num.m_type == math_val_type::string) num_str = num.get_str();
						else if(num.m_type == math_val_type::floating) num_str = ftostr(num.get_double());
						else num_str = dec(num.get_integer());
						defines.create(defname) = std::move(num_str);
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
					if (!defines.exists(defname)) throw_err_line(0, err_define_not_found, defname.data());
					else {
						string thisone = defines.find(defname);
						resolvedefines(out, thisone);
					}
				}
			}
		}
		else out+=*here++;
	}
}

bool moreonline;
bool asarverallowed = false;

void assembleline(const char * fname, int linenum, const string& line, int& single_line_for_tracker)
{
	recurseblock rec;
	bool moreonlinetmp=moreonline;
	single_line_for_tracker = 1;
	try
	{
		string out=line;
		autoptr<char**> blocks=qsplitstr(out.temp_raw(), " : ");
		moreonline=true;
		for (int block_i=0;moreonline;block_i++)
		{
			moreonline=(blocks[block_i+1] != nullptr);
			try
			{
				// it's possible that our input looks something like:
				// nop : : nop
				// nop : : : : : nop
				// also, it's possible that there were empty blocks at the start or end of the line:
				// : nop :
				// after qsplit, we still need to deal with possibly a single ": " from a preceding empty block,
				// and if it's the last block, possibly a following " :".
				char* thisblock = strip_whitespace(blocks[block_i]);
				// if the block starts with ": "
				if(thisblock[0] == ':' && thisblock[1] == ' ') {
					thisblock++;
					while(*thisblock == ' ') thisblock++;
				}
				// if the block is a single :, skip that too.
				if(thisblock[0] == ':' && thisblock[1] == 0) thisblock++;

				int len_blk = strlen(thisblock);
				// last block - strip trailing " :" if present.
				if(!moreonline && len_blk >= 2 && thisblock[len_blk-2] == ' ' && thisblock[len_blk-1] == ':') {
					thisblock[len_blk - 2] = 0;
				}

				callstack_push cs_push(callstack_entry_type::BLOCK, thisblock);

				assembleblock(thisblock, single_line_for_tracker);
				checkbankcross();
			}
			catch (errblock&) {}
			if (blocks[block_i][0]!='\0') asarverallowed=false;
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
int cur_logical_lineno;

void assemblefile(const char * filename)
{
	incsrcdepth++;
	string absolutepath = filesystem->create_absolute_path(get_current_file_name(), filename);

	if (file_included_once(absolutepath))
	{
		return;
	}

	// don't do this yet; we want "file not found" errors to show the location
	// that called assemblefile
	//callstack_push cs_push(callstack_entry_type::FILE, absolutepath);

	int startif=numif;
	if (!filecontents.exists(absolutepath))
	{
		char * temp = readfile(absolutepath, "");
		if (!temp)
		{
			throw_vfile_error(0, asar_get_last_io_error(), filename);
			return;
		}
		callstack_push cs_push(callstack_entry_type::FILE, absolutepath);

		sourcefile& newfile = filecontents.create(absolutepath);
		newfile.data = temp;
		char *inp = temp, *outp = temp, *linestartp = temp;
		enum class state_t {
			line_start, // eat whitespace
			line,
			quote,
			linecomment,
			blockcomment,
			blockcomment_start,
		};
		int lineno = 0;
		bool skip_ws = false;
		state_t state = state_t::line_start;
		bool done = false;
		// invariant: always outp <= inp
		for(;!done; inp++) {
			// for this lineno accounting to work, changing inp must only be
			// done when we are sure that we're not skipping over a newline
			if(*inp == '\n') lineno++;
			if(!*inp) {
				// inject a \n as the last char of the file, to work around stupid users making files that don't have a trailing \n
				*inp = '\n';
				done=true;
			}
			switch(state) {
				case state_t::line_start:
					if(isspace(*inp)) continue; // also catches \n
					if(*inp == ';') {
						// hacky
						if(inp[1] == '[' && inp[2] == '[') {
							inp += 2;
							state = state_t::blockcomment_start;
						}
						else state = state_t::linecomment;
						continue;
					}
					state = state_t::line;
					newfile.lines.append({outp, lineno});
					linestartp = outp;
					// fall through
				case state_t::line:
					if(*inp == '\n') {
						goto endoftheline;
					} else {
						if(skip_ws && isspace(*inp)) continue;
						skip_ws = false;
						if(*inp == ';') {
							if(inp[1] == '[' && inp[2] == '[')
								inp += 2, state = state_t::blockcomment;
							else state = state_t::linecomment;
							continue;
						}
						*outp++ = *inp;
						if(*inp == '"') state = state_t::quote;
						if(*inp == '\'') {
							inp++;
							if(!*inp || *inp == '\n') {
								callstack_push cs_push(callstack_entry_type::LINE, "", lineno); // TODO
								throw_err_null(0, err_mismatched_quotes);
								// forget this line
								newfile.lines.reset(newfile.lines.count-1);
								state = state_t::line_start;
								continue;
							}
							*outp++ = *inp++;
							while((*(unsigned char*)inp & 0xc0) == 0x80) *outp++ = *inp++; // utf8 continuation bytes
							if(*inp != '\'') {
								callstack_push cs_push(callstack_entry_type::LINE, "", lineno); // TODO
								throw_err_null(0, err_mismatched_quotes);
								// forget this line
								newfile.lines.reset(newfile.lines.count-1);
								state = state_t::line_start;
								while(*inp&&*inp!='\n') inp++; // skip the rest of the line
								continue;
							}
							*outp++ = *inp;
						}
					}
					break;
				case state_t::quote:
					if(*inp == '\n') {
						string templine(linestartp, outp-linestartp);
						callstack_push cs_push(callstack_entry_type::LINE, templine, lineno);
						throw_err_null(0, err_mismatched_quotes);
						// forget this line
						newfile.lines.reset(newfile.lines.count-1);
						state = state_t::line_start;
						continue;
					}
					*outp++ = *inp;
					if(*inp == '"') state = state_t::line;
					break;
				case state_t::linecomment:
					if(*inp == '\n') goto endoftheline;
					break;
				case state_t::blockcomment:
					if(*inp == ']' && inp[1] == ']') {
						state = state_t::line;
						inp++;
					}
					break;
				case state_t::blockcomment_start:
					if(*inp == ']' && inp[1] == ']') {
						state = state_t::line_start;
						inp++;
					}
					break;
			}
			continue;
endoftheline:
			// find last non-ws char
			char* tempp = outp-1;
			while(tempp >= linestartp && isspace(*tempp)) tempp--;
			if(tempp >= linestartp) {
				if(*tempp == '\\') {
					// line joiner
					outp = tempp; // remove the '\'
					state = state_t::line; skip_ws = true;
					continue;
				} else if(*tempp == ',') {
					// comma line joiner
					outp = tempp+1;
					state = state_t::line; skip_ws = true;
					continue;
				}
			}
			// otherwise, normal line end
			tempp[1] = 0; // trims any trailing whitespace
			outp = tempp+2; // tempp is at most outp-1, so this is at most outp++
			state = state_t::line_start;
		}
		if(state == state_t::blockcomment || state == state_t::blockcomment_start) {
			callstack_push cs_push(callstack_entry_type::LINE, "", 0); // TODO
			throw_err_null(0, err_unclosed_block_comment);
			state = state_t::line_start;
		}
		// TODO we can reach this if the last line of input ends with \ or ,
		if(state != state_t::line_start) throw_err_fatal(0, err_internal_error, "state machine broke");
	}
	sourcefile& file = filecontents.find(absolutepath);
	// previous callstack_push got dropped by the end of the if scope
	callstack_push cs_push(callstack_entry_type::FILE, absolutepath);
	asarverallowed=true;
	for (int i=0;i<file.lines.count;i++)
	{
		sourceline& l = file.lines[i];
		string connectedline(l.line);
		cur_logical_lineno = i;

		bool was_loop_end = do_line_logic(connectedline, absolutepath, l.lineno);

		// if a loop ended on this line, should it run again?
		if (was_loop_end && whilestatus[numif].cond)
			i = whilestatus[numif].startline - 1;
	}
	while (in_macro_def > 0)
	{
		throw_err_null(0, err_unclosed_macro, macro_defs[in_macro_def-1].data());
		if (!pass && in_macro_def == 1) endmacro(false);
		in_macro_def--;
		macro_defs.remove(in_macro_def);
	}
	if (numif!=startif)
	{
		numif=startif;
		numtrue=startif;
		throw_err_null(0, err_unclosed_if);
	}
	incsrcdepth--;
}

// RPG Hacker: At some point, this should probably be merged
// into assembleline(), since the two names just cause
// confusion otherwise.
// return value is "did a loop end on this line"
bool do_line_logic(const string& line, const char* filename, int lineno)
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
			if (!confirmquotes(current_line)) throw_err_line(0, err_mismatched_quotes);
		}
		else current_line=line;

		callstack_push cs_push(callstack_entry_type::LINE, current_line, lineno);

		if (stribegin(current_line, "macro ") && numif==numtrue)
		{
			// RPG Hacker: Slight redundancy here with code that is
			// also in startmacro(). Could improve this for Asar 2.0.
			string macro_name = current_line.data()+6;
			char * startpar=strqchr(macro_name.raw(), '(');
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
			if (in_macro_def == 0) throw_err_line(0, err_misplaced_endmacro);
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
	return (numif < prevnumif || single_line_for_tracker == 3)
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
				if(*pos == '\r') { pos++; continue; }
				define_name += *pos;
				pos++;
			}
			if (*pos != 0 && *pos != '\r' && *pos != '\n') pos++; // skip =
			while (*pos != 0 && *pos != '\n') {
				if(*pos == '\r') { pos++; continue; }
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

				throw_err_null(pass, err_stddefines_no_identifier);
			}

			if (!validatedefinename(define_name)) throw_err_null(pass, err_cmdl_define_invalid, "stddefines.txt", define_name.data());

			// clean define_val
			const char* defval = define_val.data();
			string cleaned_defval;

			if (*defval == 0) {
				// no value
				if (clidefines.exists(define_name)) throw_err_null(pass, err_cmdl_define_override, "Std define", define_name.data());
				clidefines.create(define_name) = "";
				continue;
			}

			while (*defval == ' ' || *defval == '\t') defval++; // skip whitespace in beginning
			if (*defval == '"') {
				defval++; // skip opening quote
				while (*defval != '"' && *defval != 0)
					cleaned_defval += *defval++;

				if (*defval == 0) {
					throw_err_null(pass, err_mismatched_quotes);
				}
				defval++; // skip closing quote
				while (*defval == ' ' || *defval == '\t') defval++; // skip whitespace
				if (*defval != 0 && *defval != '\n')
					throw_err_null(pass, err_stddefine_after_closing_quote);

				if (clidefines.exists(define_name)) throw_err_null(pass, err_cmdl_define_override, "Std define", define_name.data());
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

				if (clidefines.exists(define_name)) throw_err_null(pass, err_cmdl_define_override, "Std define", define_name.data());
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
}
#undef cfree

static void adddefine(const string & key, string & value)
{
	if (!defines.exists(key)) defines.create(key) = value;
}

string create_symbols_file(string format, uint32_t romCrc){
	string symbolfile;

	std::vector<std::pair<unsigned int, const char*>> all_labels;
	labels.each([&all_labels](const string& key, snes_label& label) {
		all_labels.push_back(std::make_pair(label.pos, key.data()));
	});
	std::sort(all_labels.begin(), all_labels.end(),
		[](auto& l, auto& r) {
			if (l.first == r.first)
				return strcmp(l.second, r.second) < 0;
			return l.first < r.first;
		});

	format = lower(format);
	if(format == "wla")
	{
		symbolfile =  "; wla symbolic information file\n";
		symbolfile += "; generated by asar\n";

		symbolfile += "\n[labels]\n";
		for (auto& p : all_labels) {
			char buffer[10];
			std::snprintf(buffer, sizeof(buffer), "%02X:%04X ", (p.first >> 16) & 0xFF, p.first & 0xFFFF);

			symbolfile += buffer;
			symbolfile += p.second;
			symbolfile += "\n";
		}

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
		for (auto& p : all_labels) {
			char buffer[10];
			std::snprintf(buffer, sizeof(buffer), "%08X ", p.first & 0xFFFFFF);
			
			symbolfile += buffer;
			symbolfile += p.second;
			symbolfile += "\n";
		}
	}
	return symbolfile;
}


bool in_top_level_file()
{
	int num_files = 0;
	for (int i = (int)callstack.size()-1; i >= 0; --i)
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
	for (int i = (int)callstack.size()-1; i >= 0; --i)
	{
		if (callstack[i].type == callstack_entry_type::FILE)
			return callstack[i].content;
	}
	return nullptr;
}

int get_current_line()
{
	for (int i = (int)callstack.size()-1; i >= 0; --i)
	{
		if (callstack[i].type == callstack_entry_type::LINE) return callstack[i].lineno;
	}
	return -1;
}

const char* get_current_block()
{
	for (int i = (int)callstack.size()-1; i >= 0; --i)
	{
		if (callstack[i].type == callstack_entry_type::LINE || callstack[i].type == callstack_entry_type::BLOCK) return callstack[i].content;
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
	optimize_dp = optimize_dp_flag::ALWAYS;
	dp_base = 0;
	optimize_address = optimize_address_flag::MIRRORS;

	closecachedfiles();

	incsrcdepth=0;
	label_counter = 0;
	errored = false;
	checksum_fix_enabled = true;
	force_checksum_fix = false;

	in_macro_def = 0;

	callstack.clear();
	simple_callstacks = true;
}
