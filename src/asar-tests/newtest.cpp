#include <algorithm>
#include <charconv>
#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>

#include "asardll.h"

namespace fs = std::filesystem;

struct Testcase {
	std::string name;
	std::string filepath;
	std::string contents;
};

struct TestResult {
	bool skipped;
	bool failed;
	std::string fail_reason;
};

// i love the c++ programming language
std::vector<std::string> split_spaces(std::string in) {
    std::vector<std::string> result;
    std::stringstream ss{in};

    for(std::string item; getline(ss, item, ' '); ) {
		if(!item.empty()) result.push_back(item);
    }
    return result;
}

size_t parse_num(std::string in, int base) {
	size_t result{};
	auto begin = in.data();
	auto end = begin + in.size();
	auto [ptr, ec] = std::from_chars(begin, end, result, base);
	if(ec != std::errc() || ptr != end) {
		throw std::invalid_argument("Invalid number " + in);
	}
	return result;
}

TestResult run_testcase(std::vector<uint8_t> base_rom, Testcase& testcase) {
	std::vector<uint8_t> expected_rom;
	std::vector<uint8_t> output_rom;

	std::vector<std::string> expected_errors;
	std::vector<std::string> expected_warnings;
	std::vector<std::string> expected_prints;
	std::vector<std::string> expected_error_prints;
	std::vector<std::string> expected_warn_prints;
	
	size_t num_iterations = 1;
	size_t rompos = 0;

	// read test case line by line
	std::istringstream asm_ss(testcase.contents);
	// if there is a byte order mark at the beginning, skip it
	if(testcase.contents.substr(0, 3) == "\xEF\xBB\xBF") {
		asm_ss.ignore(3);
	}
	for(std::string line; std::getline(asm_ss, line); ) {
		// remove trailing \r, artifact of windows newlines here
		if(line.back() == '\r') line.erase(line.back());
		if(line.substr(0, 2) == ";`") {
			auto words = split_spaces(line.substr(2));
			for(std::string& w : words) {
				if(w == "+") {
					expected_rom = base_rom;
					output_rom = base_rom;
				} else if(w == "skip") {
					return TestResult { true, false, "" };
				} else if(w[0] == '#') {
					num_iterations = parse_num(w.substr(1), 10);
				} else if(w.substr(0, 4) == "errE") {
					expected_errors.push_back(w.substr(3));
				} else if(w.substr(0, 5) == "warnW") {
					expected_warnings.push_back(w.substr(4));
				} else {
					size_t num = parse_num(w, 16);
					if(w.size() == 2) {
						if(rompos >= expected_rom.size()) expected_rom.resize(rompos+1);
						expected_rom[rompos++] = num;
					} else if(w.size() == 5 || w.size() == 6) {
						rompos = num;
					} else {
						throw std::invalid_argument("Unexpected command " + w);
					}
					if(rompos > expected_rom.size()) expected_rom.resize(rompos);
				}
			}
		// do we even want to verify these here???
		// they again feel more like api tests
		} else if(line.substr(0, 3) == ";P>") {
			expected_prints.push_back(line.substr(3));
		} else if(line.substr(0, 3) == ";W>") {
			expected_warn_prints.push_back(line.substr(3));
		} else if(line.substr(0, 3) == ";E>") {
			expected_error_prints.push_back(line.substr(3));
		} else {
			// just a line of the patch
		}
	}

	// actually do the patch
	patchparams pp;
	memset(&pp, 0, sizeof(pp));
	pp.structsize = (int)sizeof(pp);
	pp.patchloc = testcase.filepath.c_str();
	static const size_t max_rom_size = 16777216u;
	int output_rom_size = (int)output_rom.size();
	output_rom.resize(max_rom_size);
	pp.romdata = (char *)output_rom.data();
	pp.buflen = max_rom_size;
	pp.romlen = &output_rom_size;
	pp.override_checksum_gen = true;
	pp.generate_checksum = false;

	std::vector<std::string> output_errors;
	std::vector<std::string> output_warns;
	std::vector<std::string> output_prints;

	bool asar_errored = false;

	for(size_t iter = 0; iter < num_iterations; iter++) {
		asar_patch(&pp);

		int numerr;
		auto errors = asar_geterrors(&numerr);
		for(int i = 0; i < numerr; i++) {
			output_errors.push_back(errors[i].errname);
			asar_errored = true;
			// if we wanted to support ;E> then this would be the place to
			// check for Eassertion_failed and Eerror_command
		}

		int numwarn;
		auto warnings = asar_getwarnings(&numwarn);
		for(int i = 0; i < numwarn; i++) {
			output_warns.push_back(warnings[i].errname);
		}

		int numprint;
		auto prints = asar_getprints(&numprint);
		for(int i = 0; i < numprint; i++) {
			output_prints.push_back(prints[i]);
		}
	}

	output_rom.resize(output_rom_size);

	bool ok = true;
	std::ostringstream fail_reason;

	auto check_expect_lists =
		[&](std::vector<std::string> output,
			std::vector<std::string> expected,
			std::string name) {
		if(output != expected) {
			ok = false;
			if(expected.empty()) {
				fail_reason << "Expected no " + name + ", got " + name + ": ";
			} else {
				fail_reason << "Expected " + name + ": ";
				for(size_t i = 0; i < expected.size(); i++) {
					fail_reason << expected[i] << (i == expected.size()-1 ? "\n" : ", ");
				}

				fail_reason << "Actual " + name + ":   ";
			}
			for(size_t i = 0; i < output.size(); i++) {
				fail_reason << output[i] << (i == output.size()-1 ? "\n" : ", ");
			}
		}
	};

	check_expect_lists(output_errors, expected_errors, "errors");
	check_expect_lists(output_warns, expected_warnings, "warnings");
	check_expect_lists(output_prints, expected_prints, "prints");

	// don't check this if we already have errors,
	// as in that case the output rom is blank anyways
	if(!asar_errored && output_rom != expected_rom) {
		ok = false;
		if(output_rom.size() != expected_rom.size()) {
			fail_reason << "ROM size mismatch, expected 0x" <<
				std::hex << expected_rom.size() <<
				", got 0x" << output_rom.size() << "\n";
		} else {
			std::vector<std::pair<size_t, size_t>> mismatches;
			for(size_t pos = 0; pos < output_rom.size(); pos++) {
				if(output_rom[pos] != expected_rom[pos]) {
					// merge together consecutive mismatches, but don't do more than 32 bytes at a time
					if(!mismatches.empty() &&
							mismatches.back().first + mismatches.back().second == pos &&
							mismatches.back().second < 32) {
						mismatches.back().second++;
					} else {
						mismatches.push_back(std::make_pair(pos, 1));
					}
				}
			}
			for(auto m : mismatches) {
				fail_reason << "ROM data mismatch at ";
				fail_reason << "0x" << std::hex << std::setw(0) << m.first << ":\n";
				fail_reason << "Expected:";
				for(size_t i = m.first; i < m.first+m.second; i++)
					fail_reason << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)expected_rom[i];
				fail_reason << "\nActual:  ";
				for(size_t i = m.first; i < m.first+m.second; i++)
					fail_reason << ' ' << std::setfill('0') << std::setw(2) << (unsigned int)output_rom[i];
				fail_reason << '\n';
			}
		}
	}

	if(ok) {
		return TestResult { false, false, "" };
	} else {
		return TestResult { false, true, fail_reason.str() };
	}
}

int main(int argc, char *argv[])
{
	std::vector<std::string> args(argv+1, argv + argc);

	bool verbose = false;
	if(!args.empty() && args[0] == "--verbose") {
		verbose = true;
		args.erase(args.begin());
	}

	if(args.size() < 3) {
		std::cout << "Usage: newtest [--verbose] <asar_dll> <tests_directory> <dummy_rom> [testcases...]\n";
		std::cout << "\n\
Manually specify testcases by giving their filenames relative to\n\
tests_directory. If none are specified, runs all testcases in\n\
tests_directory.\n";
		return 1;
	}

	std::ifstream rom_file(args[2], std::ios::binary);
	if(!rom_file) {
		std::cerr << "failed to open rom file\n";
		return 1;
	}
	std::vector<uint8_t> rom_buffer{
		std::istreambuf_iterator<char>(rom_file),
		std::istreambuf_iterator<char>()};

	if(rom_buffer.size() != 512*1024) {
		std::cerr << "unexpected rom file size, expected 512kb\n";
		return 1;
	}

	// can't use fs::path here since this expects a char*,
	// but path on windows is wchar*
	if(!asar_init_with_dll_path(args[0].c_str())) {
		std::cerr << "couldn't load Asar library\n";
		return 1;
	}

	fs::path tests_path(args[1]);
	std::vector<fs::path> test_files;
	if(args.size() == 3) {
		// if no tests were given on the commandline, run all tests
		for(auto& file : fs::directory_iterator(tests_path)) {
			if(fs::is_regular_file(file.status())) {
				if(file.path().extension().generic_string() == ".asm") {
					test_files.push_back(file.path());
				}
			}
		}
		std::sort(test_files.begin(), test_files.end());
	} else {
		// otherwise run the specified tests
		for(size_t i = 3; i < args.size(); i++) {
			test_files.push_back(tests_path / args[i]);
		}
	}

	// read in the test files
	std::vector<Testcase> tests;
	for(auto& file : test_files) {
		std::ifstream test_file(file);
		std::stringstream test_file_contents;
		test_file_contents << test_file.rdbuf();
		tests.push_back(Testcase {
			file.filename().generic_string(),
			fs::absolute(file).generic_string(),
			test_file_contents.str()
		});
	}

	std::cout << "running " << tests.size() << " tests\n";
	size_t num_total = 0;
	size_t num_passed = 0;
	size_t num_skipped = 0;
	size_t num_failed = 0;
	std::vector<std::string> fail_msgs;
	for(auto& t : tests) {
		if(verbose) {
			std::cout << "\nrunning " << t.name << "... " << std::flush;
		}
		auto res = run_testcase(rom_buffer, t);
		if(res.failed) {
			num_failed++;
			std::cout << 'F';
			fail_msgs.push_back(t.name + " failed:\n" + res.fail_reason);
		} else {
			if(res.skipped) {
				num_skipped++;
				std::cout << 's';
			} else {
				num_passed++;
				std::cout << '.';
			}
		}
		num_total++;
		if(num_total % 40 == 0) std::cout << '\n';
		std::cout << std::flush;
	}
	std::cout << "\n";

	asar_close();
	for(auto& s : fail_msgs) {
		std::cout << s << "\n";
	}
	std::cout << num_passed << "/" << (num_passed + num_failed)
		<< " tests passed (" << num_skipped << " skipped)\n";
	if(!fail_msgs.empty()) {
		return 1;
	} else {
		return 0;
	}
}
