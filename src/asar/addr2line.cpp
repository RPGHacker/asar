#include "addr2line.h"

//////////////////////////////////////////////////////////////////////////
// Class to store address-to-line mappings for richer symbolic information
//
// During assembly, included files and information about generated asm
// should be added to this, and then read back during symbol file
// generation

void AddressToLineMapping::reset()
{
	m_fileList.reset();
	m_addrToLineInfo.reset();
}

// Adds information of what source file and line number an output rom address is at
void AddressToLineMapping::includeMapping(const char* filename, int line, int addr)
{
	AddrToLineInfo newInfo;
	newInfo.fileIdx = getFileIndex(filename);
	newInfo.line = line;
	newInfo.addr = addr;

	m_addrToLineInfo.append(newInfo);
}

// Helper to add file to list, and get the index of that file
int AddressToLineMapping::getFileIndex(const char* filename)
{
	// check if the file exists first
	for (int i = 0; i < m_fileList.count; ++i)
	{
		if (strcmp(filename, m_fileList[i].str) == 0)
		{
			return i;
		}
	}

	// file doesn't exist, so append it
	m_fileList.append(string(filename));

	return m_fileList.count - 1;
}

