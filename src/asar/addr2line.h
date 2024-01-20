#pragma once

//////////////////////////////////////////////////////////////////////////
// Class to store address-to-line mappings for richer symbolic information
//
// During assembly, included files and information about generated asm
// should be added to this, and then read back during symbol file
// generation

#include "autoarray.h"
#include "libstr.h"
#include <cstdint>

class AddressToLineMapping
{
public:

	struct AddrToLineInfo
	{
		int fileIdx;
		int line;
		int addr;
	};

	// resets the mapping to initial state
	void reset();

	// Adds information of what source file and line number an output rom address is at
	void includeMapping(const char* filename, int line, int addr);

	struct FileInfo
	{
		string filename;
		uint32_t fileCrc;
	};
	const autoarray<FileInfo>& getFileList() const { return m_fileList; }
	const autoarray<AddrToLineInfo>& getAddrToLineInfo() const { return m_addrToLineInfo; }

private:

	// Helper to add file to list, and get the index of that file
	int getFileIndex(const char* filename);

	autoarray<FileInfo> m_fileList;
	// parallel list of crcs of the filenames in fileList, to speed up lookups
	autoarray<uint32_t> m_filenameCrcs;


	autoarray<AddrToLineInfo> m_addrToLineInfo;
};

extern AddressToLineMapping addressToLineMapping;
