#include "asar.h"
#include "assocarr.h"
#include "libstr.h"
#include "libcon.h"
#include "libsmw.h"
#include "platform/file-helpers.h"
#include "virtualfile.hpp"

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

extern bool errored;
//extern int aborterrors;

extern bool checksum;

extern const char asarver[];

//extern char romtitle[30];
//extern bool stdlib;

string getdecor();

extern const char * thisfilename;

extern assocarr<string> defines;//these two are useless for cli, but they may be useful for other stuff
extern assocarr<unsigned int> labels;

void assemblefile(const char * filename, bool toplevel);

extern const char * thisfilename;
extern int thisline;
extern const char * thisblock;

extern virtual_filesystem* filesystem;

void print(const char * str)
{
	puts(str);
}

FILE * errloc=stderr;
static int errnum=0;

template<typename t> void error(int neededpass, const char * e_)
{
	try
	{
		errored=true;
		if (pass==neededpass)
		{
			errnum++;
			fputs(S getdecor()+"error: "+e_+(thisblock?(S" ["+thisblock+"]"):"")+"\n", errloc);
			if (errnum==20+1) error<errfatal>(pass, "Over 20 errors detected. Aborting.");
		}
		t err;
		throw err;
	}
	catch (errnull&) {}
}

void (*shutupgcc1)(int, const char*)=error<errnull>;
void (*shutupgcc2)(int, const char*)=error<errblock>;
void (*shutupgcc3)(int, const char*)=error<errline>;
void (*shutupgcc4)(int, const char*)=error<errfatal>;

void initmathcore();

void initstuff();
void finishpass();

void reseteverything();

bool werror=false;
bool warned=false;

void warn(const char * e_)
{
	fputs(S getdecor()+"warning: "+e_+"\n", errloc);
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

bool setmapper();

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
	HANDLE hjob=CreateJobObject(NULL, NULL);
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
		initmathcore();
		string version=S"Asar "+dec(asarver_maj)+"."+dec(asarver_min)+((asarver_bug>=10 || asarver_min>=10)?".":"")+
				dec(asarver_bug)+(asarver_beta?"pre":"")+", originally developed by Alcaro, maintained by Asar devs.\n"+
				"Source code: https://github.com/RPGHacker/asar";
		char * myname=argv[0];
		if (strrchr(myname, '/')) myname=strrchr(myname, '/')+1;
		//char * dot=strrchr(myname, '.');
		//if (dot) *dot='\0';
		if (!strncasecmp(myname, "xkas", strlen("xkas"))) {
			// can we use warn here?
			puts("Warning: xkas support is being deprecated and will be removed in a future version of Asar. Please use an older version of Asar (<=1.50) if you need it.");
			puts("(this was triggered by renaming asar.exe to xkas.exe, which activated a compatibility feature.)");
			errloc=stdout;
		}
		//if (dot) *dot='.';
		libcon_init(argc, argv,
			"[options] asm_file [rom_file]\n"
			"Supported options:\n"
			" --version         Display version information\n"
			" -v, --verbose     Enable verbose mode\n"
			" --no-title-check  Disable verifying ROM title (note that irresponsible use will likely corrupt your ROM)\n"
			" --pause-mode=<never/on-error/on-warning/always>\n"
			"                   Specify when Asar should pause the application (never, on error, on warning or always)\n"
			" -werror           Treat warnings as errors\n"
			);
		ignoretitleerrors=false;
		string par;
		bool verbose=libcon_interactive;
		bool printed_version=false;

		autoarray<string> includepaths;
		autoarray<const char*> includepath_cstrs;

		while ((par=libcon_option()))
		{
			cmdlparam postprocess_param = cmdlparam_none;
			const char* postprocess_arg = nullptr;

#define checkstartmatch(arg, stringliteral) (!strncmp(arg, stringliteral, strlen(stringliteral)))

			if (par=="-werror") werror=true;
			else if (par=="--no-title-check") ignoretitleerrors=true;
			else if (par == "-v" || par=="--verbose") verbose=true;
			else if (par=="--version")
			{
				if (!printed_version)
				{
					puts(version);
					printed_version = true;
					// RPG Hacker: ...why?!?
					// Keep finding these useless exit cases in all kinds of applications.
					//return 0;
				}
			}
			else if (checkstartmatch(par, "--pause-mode="))
			{
				if (par=="--pause-mode=never") pause=pause_no;
				else if (par=="--pause-mode=on-error") pause=pause_err;
				else if (par=="--pause-mode=on-warning") pause=pause_warn;
				else if (par=="--pause-mode=always") pause=pause_yes;
				else libcon_badusage();
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
			else libcon_badusage();

			if (postprocess_param == cmdlparam_addincludepath)
			{
				string& addpath = includepaths.append(postprocess_arg);
				includepath_cstrs.append((const char*)addpath);
			}
			else if (postprocess_param == cmdlparam_adddefine)
			{

			}
		}
		if (verbose && !printed_version)
		{
			puts(version);
			printed_version = true;
		}
		string asmname=libcon_require_filename("Enter patch name:");
		string romname=libcon_optional_filename("Enter ROM name:", NULL);
		//char * outname=libcon_optional_filename("Enter output ROM name:", NULL);
		libcon_end();
		if (!strchr(asmname, '.') && !file_exists(asmname)) asmname+=".asm";
		if (!romname)
		{
			char * romnametmp=(char*)malloc(sizeof(char)*256);
			strcpy(romnametmp, asmname);
			if (strrchr(romnametmp, '.')) *strrchr(romnametmp, '.')=0;
			if (file_exists(S romnametmp+".sfc")) romname=S romnametmp+".sfc";
			else if (file_exists(S romnametmp+".smc")) romname=S romnametmp+".smc";
			else romname=S romnametmp+".sfc";
		}
		else if (!strchr(romname, '.') && !file_exists(romname))
		{
			if (file_exists(S romname+".sfc")) romname+=".sfc";
			else if (file_exists(S romname+".smc")) romname+=".smc";
		}
		if (!file_exists(romname))
		{
			FILE * f=fopen(romname, "wb");
			if (!f)
			{
				error<errfatal>(pass, "Couldn't create ROM.");
			}
			fclose(f);
		}
		if (!openrom(romname, false))
		{
			thisfilename=NULL;
			error<errnull>(pass, openromerror);
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
					if (!libcon_question_bool(S"Warning: The ROM title appears to be \""+title+"\", which looks like garbage. "
							"Is this your ROM title? (Note that inproperly answering \"yes\" will crash your ROM.)", false))
					{
						puts("Assembling aborted. snespurify should be able to fix your ROM.");
						return 1;
					}
				}
				else
				{
					puts(S"Error: The ROM title appears to be \""+title+"\", which looks like garbage. "
								"If this is the ROM title, add --no-title-check to the command line options. If the ROM title is something else, use snespurify on your ROM.");
					pause(err);
					return 1;
				}
			}
		}
		romdata_r=(unsigned char*)malloc((size_t)romlen);
		romlen_r=romlen;
		memcpy((void*)romdata_r, romdata, (size_t)romlen);//recently allocated, dead

		int includepath_count = includepath_cstrs.count;
		virtual_filesystem new_filesystem;
		new_filesystem.initialize(&includepath_cstrs[0], (size_t)includepath_count);
		filesystem = &new_filesystem;

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

		new_filesystem.destroy();
		filesystem = nullptr;


		if (werror && warned) error<errnull>(pass, "One or more warnings was detected with werror on.");
		if (checksum) fixchecksum();
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
		closerom();
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

