// diy argument parser because NIH syndrome or something
#include <type_traits>
#include "autoarray.h"
#include "libstr.h"
#include "errors.h"

// T is a lambda with 0 or 1 arguments
template<typename T>
struct opt {
	constexpr opt(char shortname, const char* longname, T&& cb)
	    : shortname(shortname), longname(longname), callback(cb) {}
	char shortname;
	const char* longname;
	T& callback;
};

extern int pass;

template<typename... T>
autoarray<const char*> parse_opts(int argc, const char* argv[], opt<T>... opts) {
	autoarray<const char*> positional;
	int i;
	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-' || argv[i] == STR "-") {
			positional.append(argv[i]);
			continue;
		}
		if(argv[i] == STR "--") {
			i++;
			break; // treat rest of the args as positional
		}

		bool found = false;
		string helpname;
		const char* inline_arg = nullptr;
		auto call = [&](auto cb) {
			found = true;
			if constexpr (std::is_invocable_v<decltype(cb)>) {
				if(inline_arg) throw_err_null(pass, err_cli_no_arg, helpname.data());
				else cb();
			} else {
				const char* arg = inline_arg ? inline_arg : argv[++i];
				if(!arg || !*arg) throw_err_null(pass, err_cli_missing_arg, helpname.data());
				else cb(arg);
			}
		};

		if(argv[i][1] == '-') {
			// long option
			const char* option = argv[i]+2;
			inline_arg = strchr(option, '=');
			string optname{};
			if(inline_arg) {
				optname.assign(option, inline_arg - option);
				inline_arg++;
			}
			else optname.assign(option);

			helpname = STR "--" + optname;
			// i love this language
			([&] {
				if(opts.longname && optname == opts.longname) call(opts.callback);
			}(), ...);
		} else {
			// short option
			char optname = argv[i][1];
			if(argv[i][2] != 0) inline_arg = argv[i]+2;
			helpname = STR "-" + optname;
			([&] {
				if(opts.shortname && optname == opts.shortname) call(opts.callback);
			}(), ...);
		}
		if(!found) {
			throw_err_null(pass, err_cli_unknown_opt, helpname.data());
		}
	}
	// rest are positional
	for(; i < argc; i++) positional.append(argv[i]);
	return positional;
}
