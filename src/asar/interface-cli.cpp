#include "asar.h"
#include "assocarr.h"
#include "libstr.h"
#include "libcon.h"
#include "libsmw.h"
#include "errors.h"
#include "warnings.h"
#include "platform/file-helpers.h"
#include "virtualfile.h"
#include "interface-shared.h"
#include "assembleblock.h"
#include "asar_math.h"

#ifdef TIMELIMIT
# if defined(linux)
#  include <sys/resource.h>
#  include <signal.h>
# elif defined(_WIN32)
//WARNING: The Windows equivalent of SIGXCPU, job limits, is very poorly suited for short-running
// tasks like this; it's only checked approximately every seven seconds on the machine I tested on,
// and it kills the process instantly once this happens. (Additionally, due to an implementation
// quirk, it'll bug up if you ask for anything above about seven minutes, so don't do that.)
#  include <windows.h>
# else
#  error Time limits not configured for this OS.
# endif
#endif

extern const char asarver[];

void print(const char * str)
{
	puts(str);
}

static FILE * errloc=stderr;
static int errnum=0;

static int max_num_errors = 20;

void error_interface(int errid, int whichpass, const char * e_)
{
	errored = true;
	if (pass == whichpass)
	{
		errnum++;
		// don't show current block if the error came from an error command
		bool show_block = (thisblock && (errid != error_id_error_command));
		fputs(STR getdecor() + "error: (" + get_error_name((asar_error_id)errid) + "): " + e_ + (show_block ? (STR" [" + thisblock + "]") : STR "") + "\n", errloc);
		if (errnum == max_num_errors + 1) asar_throw_error(pass, error_type_fatal, error_id_limit_reached, max_num_errors);
	}
}

static bool werror=false;
static bool warned=false;

void warn(int errid, const char * e_)
{
	// don't show current block if the warning came from a warn command
	bool show_block = (thisblock && (errid != warning_id_warn_command));
	fputs(STR getdecor()+"warning: (" + get_warning_name((asar_warning_id)errid) + "): " + e_ + (show_block ? (STR" [" + thisblock + "]") : STR "") + "\n", errloc);
	warned=true;
}

#ifdef TIMELIMIT
#if defined(linux)
void onsigxcpu(int ignored)
{
	error<errnull>(pass, "Time limit exceeded.");
	exit(1);
}
#elif defined(_WIN32)
//null
#endif
#endif



int main(int argc, char * argv[])
{
#ifdef TIMELIMIT
#if defined(linux)
	rlimit lim;
	lim.rlim_cur=TIMELIMIT;
	lim.rlim_max=RLIM_INFINITY;
	setrlimit(RLIMIT_CPU, &lim);
	signal(SIGXCPU, onsigxcpu);
#elif defined(_WIN32)
	HANDLE hjob=CreateJobObject(NULL, nullptr);
	AssignProcessToJobObject(hjob, GetCurrentProcess());
	JOBOBJECT_BASIC_LIMIT_INFORMATION jbli;
	jbli.LimitFlags=JOB_OBJECT_LIMIT_PROCESS_TIME;
	jbli.PerProcessUserTimeLimit.LowPart=10*1000*1000*TIMELIMIT;
	jbli.PerProcessUserTimeLimit.HighPart=0;
	SetInformationJobObject(hjob, JobObjectBasicLimitInformation, &jbli, sizeof(jbli));
#endif
#endif
#define pause(sev) do { if (pause>=pause_##sev) libcon_pause(); } while(0)

	enum {
		pause_no,
		pause_err,
		pause_warn,
		pause_yes,
	} pause=pause_no;

	enum cmdlparam
	{
		cmdlparam_none,

		cmdlparam_addincludepath,
		cmdlparam_adddefine,

		cmdlparam_count
	};

	try
	{
		romdata_r = nullptr;
		string version=STR"Asar "+dec(asarver_maj)+"."+dec(asarver_min)+((asarver_bug>=10 || asarver_min>=10)?".":"")+
				dec(asarver_bug)+(asarver_beta?"pre":"")+", originally developed by Alcaro, maintained by Asar devs.\n"+
				"Source code: https://github.com/RPGHacker/asar\n";
		char * myname=argv[0];
		if (strrchr(myname, '/')) myname=strrchr(myname, '/')+1;
		//char * dot=strrchr(myname, '.');
		//if (dot) *dot='\0';
		if (!strncasecmp(myname, "xkas", strlen("xkas"))) {
			// RPG Hacker: no asar_throw_Warning() here, because we didn't have a chance to disable warnings yet.
			// Also seems like warning aren't even registered at this point yet.
			puts("Warning: xkas support is being deprecated and will be removed in the next release of asar!!!");
			puts("(this was triggered by renaming asar.exe to xkas.exe, which activated a compatibility feature.)");
			errloc=stdout;
		}
		//if (dot) *dot='.';
		libcon_init(argc, argv,
			"[options] asm_file [rom_file]\n\n"
			"Supported options:\n\n"
			" --version         \n"
			"                   Display version information.\n\n"
			" -v, --verbose     \n"
			"                   Enable verbose mode.\n\n"
			" --symbols=<none/wla/nocash>\n"
			"                   Specifies the format of the symbols output file. (Default is none for no symbols file)\n\n"
			" --symbols-path=<filename>\n"
			"                   Override the default path to the symbols output file. The default is the ROM's base name with an\n"
			"                   extension of '.sym'.\n\n"
			" --no-title-check\n"
			"                   Disable verifying ROM title. (Note that irresponsible use will likely corrupt your ROM)\n\n"
			" --pause-mode=<never/on-error/on-warning/always>\n"
			"                   Specify when Asar should pause the application. (Never, on error, on warning or always)\n\n"
			" --fix-checksum=<on/off>\n"
			"                   Override Asar's checksum generation, allowing you to manually enable/disable generating a checksum\n\n"
			" -I<path>          \n"
			" --include <path>  \n"
			"                   Add an include search path to Asar.\n\n"
			" -D<def>[=<val>]   \n"
			" --define <def>[=<val>]\n"
			"                   Add a define (optionally with a value) to Asar.\n\n"
			" -werror           \n"
			"                   Treat warnings as errors.\n\n"
			" -w<name>          \n"
			"                   Enable a specific warning.\n\n"
			" -wno<name>        \n"
			"                   Disable a specific warning.\n\n"
			" --error-limit=<N> \n"
			"                   Stop after encountering this many errors, instead of the default 20\n\n"
			);
		ignoretitleerrors=false;
		string par;
		bool verbose=libcon_interactive;
		string symbols="";
		string symfilename="";

		autoarray<string> includepaths;
		autoarray<const char*> includepath_cstrs;

		while ((par=libcon_option()))
		{
			cmdlparam postprocess_param = cmdlparam_none;
			const char* postprocess_arg = nullptr;

#define checkstartmatch(arg, stringliteral) (!strncmp(arg, stringliteral, strlen(stringliteral)))

			if (par=="--no-title-check") ignoretitleerrors=true;
			else if (par == "-v" || par=="--verbose") verbose=true;
			else if (checkstartmatch(par, "--symbols="))
			{
				if (par == "--symbols=none") symbols = "";
				else if (par=="--symbols=wla") symbols="wla";
				else if (par=="--symbols=nocash") symbols="nocash";
				else libcon_badusage();
			}
			else if (checkstartmatch(par, "--symbols-path=")) {
				symfilename=((const char*)par) + strlen("--symbols-path=");
			}
			else if (checkstartmatch(par, "--error-limit="))
			{
				char* out;
				long lim = strtol((const char*)par + strlen("--error-limit="), &out, 10);
				max_num_errors = lim;
			}
			else if (par=="--version")
			{
				puts(version);
				return 0;
			}
			else if (checkstartmatch(par, "--pause-mode="))
			{
				if (par=="--pause-mode=never") pause=pause_no;
				else if (par=="--pause-mode=on-error") pause=pause_err;
				else if (par=="--pause-mode=on-warning") pause=pause_warn;
				else if (par=="--pause-mode=always") pause=pause_yes;
				else libcon_badusage();
			}
			else if(checkstartmatch(par, "--fix-checksum=")) {
				if(par=="--fix-checksum=on") {
					force_checksum_fix = true;
					checksum_fix_enabled = true;
				} else if(par=="--fix-checksum=off") {
					force_checksum_fix = true;
					checksum_fix_enabled = false;
				} else libcon_badusage();
			}
			else if (checkstartmatch(par, "-I"))
			{
				postprocess_param = cmdlparam_addincludepath;
				postprocess_arg = ((const char*)par) + strlen("-I");
			}
			else if (checkstartmatch(par, "-D"))
			{
				postprocess_param = cmdlparam_adddefine;
				postprocess_arg = ((const char*)par) + strlen("-D");
			}
			else if (par == "--include")
			{
				postprocess_arg = libcon_option_value();
				if (postprocess_arg != nullptr)
				{
					postprocess_param = cmdlparam_addincludepath;
				}
			}
			else if (par == "--define")
			{
				postprocess_arg = libcon_option_value();
				if (postprocess_arg != nullptr)
				{
					postprocess_param = cmdlparam_adddefine;
				}
			}
			else if (checkstartmatch(par, "-w"))
			{
				const char* w_param = ((const char*)par) + strlen("-w");

				if (checkstartmatch(w_param, "error"))
				{
					werror = true;
				}
				else if (checkstartmatch(w_param, "no"))
				{
					asar_warning_id warnid = parse_warning_id_from_string(w_param + strlen("no"), pass);

					if (warnid != warning_id_end)
					{
						set_warning_enabled(warnid, false);
					}
					else
					{
						asar_throw_error(pass, error_type_null, error_id_invalid_warning_id, "-wno", (int)(warning_id_start + 1), (int)(warning_id_end - 1));
					}
				}
				else
				{
					asar_warning_id warnid = parse_warning_id_from_string(w_param, pass);

					if (warnid != warning_id_end)
					{
						set_warning_enabled(warnid, true);
					}
					else
					{
						asar_throw_error(pass, error_type_null, error_id_invalid_warning_id, "-w", (int)(warning_id_start + 1), (int)(warning_id_end - 1));
					}
				}

			}
			else libcon_badusage();

			if (postprocess_param == cmdlparam_addincludepath)
			{
				includepaths.append(postprocess_arg);
			}
			else if (postprocess_param == cmdlparam_adddefine)
			{
				if (strchr(postprocess_arg, '=') != nullptr)
				{
					// argument contains value, not only name
					const char* eq_loc = strchr(postprocess_arg, '=');
					string name = string(postprocess_arg, (int)(eq_loc - postprocess_arg));
					name = strip_whitespace(name);
					name = strip_prefix(name, '!', false); // remove leading ! if present

					if (!validatedefinename(name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_invalid, "command line defines", name.data());

					if (clidefines.exists(name)) {
						asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Command line define", name.data());
						pause(err);
						return 1;
					}
					clidefines.create(name) = eq_loc + 1;
				}
				else
				{
					// argument doesn't have a value, only name
					string name = postprocess_arg;
					name = strip_whitespace(name);
					name = strip_prefix(name, '!', false); // remove leading ! if present

					if (!validatedefinename(name)) asar_throw_error(pass, error_type_null, error_id_cmdl_define_invalid, "command line defines", name.data());

					if (clidefines.exists(name)) {
						asar_throw_error(pass, error_type_null, error_id_cmdl_define_override, "Command line define", name.data());
						pause(err);
						return 1;
					}
					clidefines.create(name) = "";
				}
			}
		}
		if (verbose)
		{
			puts(version);
		}
		string asmname=libcon_require_filename("Enter patch name:");
		string romname=libcon_optional_filename("Enter ROM name:", nullptr);
		//char * outname=libcon_optional_filename("Enter output ROM name:", nullptr);
		libcon_end();
		if (!strchr(asmname, '.') && !file_exists(asmname)) asmname+=".asm";
		if (!romname)
		{
			string romnametmp = get_base_name(asmname);
			if (file_exists(romnametmp+".sfc")) romname=romnametmp+".sfc";
			else if (file_exists(romnametmp+".smc")) romname=romnametmp+".smc";
			else romname=STR romnametmp+".sfc";
		}
		else if (!strchr(romname, '.') && !file_exists(romname))
		{
			if (file_exists(STR romname+".sfc")) romname+=".sfc";
			else if (file_exists(STR romname+".smc")) romname+=".smc";
		}
		if (!file_exists(romname))
		{
			FILE * f=fopen(romname, "wb");
			if (!f)
			{
				asar_throw_error(pass, error_type_fatal, error_id_create_rom_failed);
			}
			fclose(f);
		}
		if (!openrom(romname, false))
		{
			thisfilename= nullptr;
			asar_throw_error(pass, error_type_null, openromerror);
			pause(err);
			return 1;
		}
		//check if the ROM title and checksum looks sane
		if (romlen>=32768 && !ignoretitleerrors)
		{
			bool validtitle=setmapper();
			if (!validtitle)
			{
				string title;
				for (int i=0;i<21;i++)
				{
					unsigned char c=romdata[snestopc(0x00FFC0+i)];
					if (c==7) c=14;
					if (c==8) c=27;//to not generate more hard-to-print characters than needed
					if (c==9) c=26;//random characters are picked in accordance with the charset Windows-1252, but they should be garbage in all charsets
					if (c=='\r') c=17;
					if (c=='\n') c=25;
					if (c=='\0') c=155;
					title+=(char)c;
				}
				if (libcon_interactive)
				{
					if (!libcon_question_bool(STR"Warning: The ROM title appears to be \""+title+"\", which looks like garbage. "
							"Is this your ROM title? (Note that inproperly answering \"yes\" will crash your ROM.)", false))
					{
						puts("Assembling aborted. snespurify should be able to fix your ROM.");
						return 1;
					}
				}
				else
				{
					puts(STR"Error: The ROM title appears to be \""+title+"\", which looks like garbage. "
								"If this is the ROM title, add --no-title-check to the command line options. If the ROM title is something else, use snespurify on your ROM.");
					pause(err);
					return 1;
				}
			}
		}

		string stdincludespath = STR dir(argv[0]) + "stdincludes.txt";
		parse_std_includes(stdincludespath, includepaths);

		for (int i = 0; i < includepaths.count; ++i)
		{
			includepath_cstrs.append((const char*)includepaths[i]);
		}

		size_t includepath_count = (size_t)includepath_cstrs.count;
		virtual_filesystem new_filesystem;
		new_filesystem.initialize(&includepath_cstrs[0], includepath_count);
		filesystem = &new_filesystem;

		string stddefinespath = STR dir(argv[0]) + "stddefines.txt";
		parse_std_defines(stddefinespath);

		for (pass=0;pass<3;pass++)
		{
			//pass 1: find which bank all labels are in, for label optimizations
			//  freespaces are listed as above 0xFFFFFF, to find if it's in the ROM or if it's dynamic
			//pass 2: find where exactly all labels are
			//pass 3: assemble it all
			initstuff();
			assemblefile(asmname, true);
			finishpass();
		}

		closecachedfiles(); // this needs the vfs so do it before destroying it
		new_filesystem.destroy();
		filesystem = nullptr;

		if (werror && warned) asar_throw_error(pass, error_type_null, error_id_werror);
		if (checksum_fix_enabled) fixchecksum();
		//if (pcpos>romlen) romlen=pcpos;
		if (errored)
		{
			puts("Errors were detected while assembling the patch. Assembling aborted. Your ROM has not been modified.");
			closerom(false);
			reseteverything();
			pause(err);
			return 1;
		}
		if (warned)
		{
			if (libcon_interactive)
			{
				if (!libcon_question_bool("One or more warnings were detected while assembling the patch. "
																	"Do you want insert the patch anyways? (Default: yes)", true))
				{
					puts("ROM left unchanged.");
					closerom(false);
					reseteverything();
					return 1;
				}
			}
			else
			{
				if (verbose) puts("Assembling completed, but one or more warnings were detected.");
				pause(warn);
			}
		}
		else
		{
			if (verbose) puts("Assembling completed without problems.");
			pause(yes);
		}
		unsigned int romCrc = closerom();
		if (symbols)
		{
			if (!symfilename) symfilename = get_base_name(romname)+".sym";
			string contents = create_symbols_file(symbols, romCrc);
			FILE * symfile = fopen(symfilename, "wt");
			if (!symfile)
			{
				puts(STR"Failed to create symbols file: \"" + symfilename + "\".");
				pause(err);
				return 1;
			}
			else
			{
				fputs(contents, symfile);
				fclose(symfile);
			}
		}
		reseteverything();
	}
	catch(errfatal&)
	{
		puts("A fatal error was detected while assembling the patch. Assembling aborted. Your ROM has not been modified.");
		closerom(false);
		reseteverything();
		pause(err);
		return 1;
	}
	return 0;
}
