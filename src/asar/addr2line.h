#pragma once

//////////////////////////////////////////////////////////////////////////
// Class to store address-to-line mappings for richer symbolic information
//
// During assembly, included files and information about generated asm
// should be added to this, and then read back during symbol file
// generation

#include "autoarray.h"
#include "libstr.h"

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

	// While the virtual filesystem is available, calculate the crc's of the entire filelist
	void calculateFileListCrcs();

	const autoarray<string>& getFileList() const { return m_fileList; }
	const autoarray<unsigned long>& getFileListCrcs() const { return m_fileListCrcs; }
	const autoarray<AddrToLineInfo>& getAddrToLineInfo() const { return m_addrToLineInfo; }

private:

	// Helper to add file to list, and get the index of that file
	int getFileIndex(const char* filename);

	autoarray<string> m_fileList;
	autoarray<unsigned long> m_fileListCrcs;

	autoarray<AddrToLineInfo> m_addrToLineInfo;
};

