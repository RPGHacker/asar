#include "addr2line.h"
#include "crc32.h"

//////////////////////////////////////////////////////////////////////////
// Class to store address-to-line mappings for richer symbolic information
//
// During assembly, included files and information about generated asm
// should be added to this, and then read back during symbol file
// generation

void AddressToLineMapping::reset()
{
	m_fileList.reset();
	m_file_indices_map.clear();
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
	if(auto it = m_file_indices_map.find(filename); it != m_file_indices_map.end()) {
		return it->second;
	}

	// file doesn't exist, so start tracking it
	char* data = nullptr;
	int len = 0;
	uint32_t fileCrc = 0;
	if (readfile(filename, "", &data, &len))
	{
		fileCrc = crc32((const uint8_t*)data, (unsigned int)len);
	}
	free(data);

	int result = m_fileList.count;
	m_fileList.append({ string(filename), fileCrc });
	m_file_indices_map.emplace(filename, result);

	return result;
}
