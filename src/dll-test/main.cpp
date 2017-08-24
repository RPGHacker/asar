#include "../asardll.h"

#include <cstdio>
#include <vector>
#include <iostream>
#include <fstream>


std::vector<char> rom_data;
static const std::size_t max_supported_rom_size = 16 * 1024 * 1024;


int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
	{
		std::printf("usage: %s [patch_path] <[input_rom_path]>\n", argv[0]);
		return -1;
	}

	{
		std::ifstream input_patch_file(argv[1], std::ifstream::binary);
		if (!input_patch_file.good())
		{
			std::printf("unable to open patch file '%s' - aborting\n", argv[1]);
			return -1;
		}
		input_patch_file.close();
	}

	// Reserve at least 16 MB for the ROM - do we ever need more than that at all?
	rom_data.resize(max_supported_rom_size);
	std::fill(rom_data.begin(), rom_data.end(), 0); // probably not needed, but just to be safe

	int rom_size = 0;

	if (argc > 2)
	{
		std::ifstream input_rom_file(argv[2], std::ifstream::ate | std::ifstream::binary);

		if (input_rom_file.is_open() && input_rom_file.good())
		{
			std::ifstream::pos_type total_size = input_rom_file.tellg();
			std::ifstream::pos_type data_size = total_size;
			std::ifstream::pos_type start_pos = 0;

			// Some kind of hack: If this comparison is true, we assume the input ROM is headered and skip the first 512 bytes
			if (total_size % 0x8000 == 512)
			{
				data_size -= 512;
				start_pos += 512;
			}

			if (data_size > max_supported_rom_size)
			{
				std::printf("trying to read a ROM of size %zu, but our maximum supported size is %zu\n", (size_t)data_size, max_supported_rom_size);
				return -1;
			}

			input_rom_file.seekg(start_pos);
			input_rom_file.read(&rom_data[0], data_size);

			input_rom_file.close();

			rom_size = (int)data_size;
		}
		else
		{
			std::printf("unable to open ROM file '%s' - not reading any data\n", argv[2]);
		}
	}

	if (!asar_init())
	{
		std::printf("initializing Asar lib failed!\n");
		return -1;
	}

	if (!asar_patch(argv[1], &rom_data[0], rom_data.size(), &rom_size))
	{
		std::printf("applying patch file '%s' failed with the following errors:\n", argv[1]);

		int error_count = 0;
		const errordata* error_data = asar_geterrors(&error_count);

		for (int i = 0; i < error_count; ++i)
		{
			std::printf("%s\n", error_data[i].fullerrdata);
		}

		return -1;
	}

	int numwrittenblocks=0;
	const writtenblockdata * writtenblocks = asar_getwrittenblocks(&numwrittenblocks);

	for (int i = 0; i < numwrittenblocks; ++i)
	{
		std::printf("$%02x:%04x (h%06x): wrote %d bytes\n", (writtenblocks[i].snesoffset >> 16) & 0xFF, writtenblocks[i].snesoffset & 0xFFFF, writtenblocks[i].pcoffset, writtenblocks[i].numbytes);
	}

	mappertype currentmapper = asar_getmapper();

	asar_close();
	return 0;
}