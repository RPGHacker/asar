#include "std-includes.h"
#include "libsmw.h"
#include "autoarray.h"
#include "errors.h"
#include "asar.h"
#include "crc32.h"
#include <cstdint>

mapper_t mapper=lorom;
int sa1banks[8]={0<<20, 1<<20, -1, -1, 2<<20, 3<<20, -1, -1};
const unsigned char * romdata= nullptr; // NOTE: Changed into const to prevent direct write access - use writeromdata() functions below
int romlen;
static bool header;
static FILE * thisfile;

asar_error_id openromerror;

autoarray<writtenblockdata> writtenblocks;

// RPG Hacker: Uses binary search to find the insert position of our ROM write

static int findromwritepos(int snesoffset, int searchstartpos, int searchendpos)
{
	if (searchendpos == searchstartpos)
	{
		return searchstartpos;
	}

	int centerpos = searchstartpos + ((searchendpos - searchstartpos) / 2);

	if (writtenblocks[centerpos].snesoffset >= snesoffset)
	{
		return findromwritepos(snesoffset, searchstartpos, centerpos);
	}

	return findromwritepos(snesoffset, centerpos + 1, searchendpos);
}


static void addromwriteforbank(int snesoffset, int numbytes)
{
	int currentbank = (snesoffset & 0xFF0000);

	int insertpos = findromwritepos(snesoffset, 0, writtenblocks.count);

	if (insertpos > 0 && (writtenblocks[insertpos - 1].snesoffset & 0xFF0000) == currentbank
		&& writtenblocks[insertpos - 1].snesoffset + writtenblocks[insertpos - 1].numbytes >= snesoffset)
	{
		// Merge if we overlap with a preceding block
		int firstend = writtenblocks[insertpos - 1].snesoffset + writtenblocks[insertpos - 1].numbytes;
		int secondend = snesoffset + numbytes;

		int newend = (firstend > secondend ? firstend : secondend);

		numbytes = newend - writtenblocks[insertpos - 1].snesoffset;
		snesoffset = writtenblocks[insertpos - 1].snesoffset;

		writtenblocks.remove(insertpos - 1);
		insertpos -= 1;
	}

	while (insertpos <  writtenblocks.count && (writtenblocks[insertpos].snesoffset & 0xFF0000) == currentbank
		&& snesoffset + numbytes >= writtenblocks[insertpos].snesoffset)
	{
		// Merge if we overlap with a succeeding block
		int firstend = snesoffset + numbytes;
		int secondend = writtenblocks[insertpos].snesoffset + writtenblocks[insertpos].numbytes;

		int newend = (firstend > secondend ? firstend : secondend);

		numbytes = newend - snesoffset;

		writtenblocks.remove(insertpos);
	}

	// Insert ROM write
	writtenblockdata blockdata;
	blockdata.snesoffset = snesoffset;
	blockdata.pcoffset = snestopc(snesoffset);
	blockdata.numbytes = numbytes;

	writtenblocks.insert(insertpos, blockdata);
}


static void addromwrite(int pcoffset, int numbytes)
{
	int snesaddr = pctosnes(pcoffset);
	int bytesleft = numbytes;

	// RPG Hacker: Some kind of witchcraft which I actually hope works as intended
	// Basically, the purpose of this is to sort all ROM writes into banks for the sake of cleanness

	while (((snesaddr >> 16) & 0xFF) != (((snesaddr + bytesleft) >> 16) & 0xFF))
	{
		int bytesinbank = ((snesaddr + 0x10000) & 0xFF0000) - snesaddr;

		addromwriteforbank(snesaddr, bytesinbank);

		pcoffset += bytesinbank;
		snesaddr = pctosnes(pcoffset);
		bytesleft -= bytesinbank;
	}

	addromwriteforbank(snesaddr, bytesleft);
}

void writeromdata(int pcoffset, const void * indata, int numbytes)
{
	memcpy(const_cast<unsigned char*>(romdata) + pcoffset, indata, (size_t)numbytes);
	addromwrite(pcoffset, numbytes);
}

void writeromdata_byte(int pcoffset, unsigned char indata)
{
	memcpy(const_cast<unsigned char*>(romdata) + pcoffset, &indata, 1);
	addromwrite(pcoffset, 1);
}

void writeromdata_bytes(int pcoffset, unsigned char indata, int numbytes)
{
	memset(const_cast<unsigned char*>(romdata) + pcoffset, indata, (size_t)numbytes);
	addromwrite(pcoffset, numbytes);
}

int ratsstart(int snesaddr)
{
	int pcaddr=snestopc(snesaddr);
	if (pcaddr<0x7FFF8) return -1;
	const unsigned char * start=romdata+pcaddr-0x10000;
	for (int i=0x10000;i>=0;i--)
	{
		if (!strncmp((const char*)start+i, "STAR", 4) &&
				(start[i+4]^start[i+6])==0xFF && (start[i+5]^start[i+7])==0xFF)
		{
			if ((start[i+4]|(start[i+5]<<8))>0x10000-i-8-1) return pctosnes((int)(start-romdata+i));
			return -1;
		}
	}
	return -1;
}

void resizerats(int snesaddr, int newlen)
{
	int pos=snestopc(ratsstart(snesaddr));
	if (pos<0) return;
	if (newlen!=1) newlen--;
	writeromdata_byte(pos+4, (unsigned char)(newlen&0xFF));
	writeromdata_byte(pos+5, (unsigned char)((newlen>>8)&0xFF));
	writeromdata_byte(pos+6, (unsigned char)((newlen&0xFF)^0xFF));
	writeromdata_byte(pos+7, (unsigned char)(((newlen>>8)&0xFF)^0xFF));
}

static void handleprot(int loc, char * name, int len, const unsigned char * contents)
{
	(void)loc;		// RPG Hacker: Silence "unused argument" warning.

	if (!strncmp(name, "PROT", 4))
	{
		memcpy(name, "NULL", 4);//to block recursion, in case someone is an idiot
		if (len%3) return;
		len/=3;
		for (int i=0;i<len;i++) removerats((contents[(i*3)+0]    )|(contents[(i*3)+1]<<8 )|(contents[(i*3)+2]<<16), 0x00);
	}
}

void removerats(int snesaddr, unsigned char clean_byte)
{
	int addr=ratsstart(snesaddr);
	if (addr<0) return;
	// randomdude999: don't forget bank borders
	WalkMetadata(pctosnes(snestopc(addr)+8), handleprot);
	addr=snestopc(addr);
	for (int i=(romdata[addr+4]|(romdata[addr+5]<<8))+8;i>=0;i--) writeromdata_byte(addr+i, clean_byte);
}

static inline int trypcfreespace(int start, int end, int size, int banksize, int minalign, unsigned char freespacebyte)
{
	while (start+size<=end)
	{
		if (
				((start+8)&~banksize)!=((start+size-1)&~banksize&0xFFFFFF)//if the contents won't fit in this bank...
			&&
				(start&banksize&0xFFFFF8)!=(banksize&0xFFFFF8)//and the RATS tag can't fit in the bank either...
			)
		{
			start&=~banksize&0xFFFFFF;//round it down to the start of the bank,
			start|=banksize&0xFFFFF8;//then round it up to the end minus the RATS tag...
			continue;
		}
		if (minalign)
		{
			start&=~minalign&0xFFFFFF;
			start|=minalign&0xFFFFF8;
		}
		if (!strncmp((const char*)romdata+start, "STAR", 4) &&
				(romdata[start+4]^romdata[start+6])==0xFF && (romdata[start+5]^romdata[start+7])==0xFF)
		{
			start+=(romdata[start+4]|(romdata[start+5]<<8))+1+8;
			continue;
		}
		bool bad=false;
		for (int i=0;i<size;i++)
		{
			if (romdata[start+i]!=freespacebyte)
			{
				// TheBiob: fix freedata align freezing.
				if ((start & minalign) == 0x7FF8 && i < 8) i = 8;
				start+=i;
				if (!i) start++;//this could check for a rats tag instead, but somehow I think this will give better performance.
				bad=true;
				break;
			}
		}
		if (bad) continue;
		size-=8;
		if (size) size--;//rats tags eat one byte more than specified for some reason
		writeromdata_byte(start+0, 'S');
		writeromdata_byte(start+1, 'T');
		writeromdata_byte(start+2, 'A');
		writeromdata_byte(start+3, 'R');
		writeromdata_byte(start+4, (unsigned char)(size&0xFF));
		writeromdata_byte(start+5, (unsigned char)((size>>8)&0xFF));
		writeromdata_byte(start+6, (unsigned char)((size&0xFF)^0xFF));
		writeromdata_byte(start+7, (unsigned char)(((size>>8)&0xFF)^0xFF));
		return start+8;
	}
	return -1;
}

//This function finds a block of freespace. -1 means "no freespace found", anything else is a PC address.
//isforcode=false tells it to favor banks 40+, true tells it to avoid them entirely.
//It automatically adds a RATS tag.

int getpcfreespace(int size, bool isforcode, bool autoexpand, bool respectbankborders, bool align, unsigned char freespacebyte)
{
	if (!size) return 0x1234;//in case someone protects zero bytes for some dumb reason.
		//You can write zero bytes to anywhere, so I'll just return something that removerats will ignore.
	if (size>0x10000) return -1;
	size+=8;
	if (mapper==lorom)
	{
		if (size>0x8008 && respectbankborders) return -1;
	rebootlorom:
		if (romlen>0x200000 && !isforcode)
		{
			int pos=trypcfreespace(0x200000-8, (romlen<0x400000)?romlen:0x400000, size,
					respectbankborders?0x7FFF:0xFFFFFF, align?0x7FFF:(respectbankborders || size<32768)?0:0x7FFF, freespacebyte);
			if (pos>=0) return pos;
		}
		int pos=trypcfreespace(0x80000, (romlen<0x200000)?romlen:0x200000, size,
				respectbankborders?0x7FFF:0xFFFFFF, align?0x7FFF:(respectbankborders || size<32768)?0:0x7FFF, freespacebyte);
		if (pos>=0) return pos;
		if (autoexpand)
		{
			if(0);
			else if (romlen==0x080000)
			{
				romlen=0x100000;
				writeromdata_byte(snestopc(0x00FFD7), 0x0A);
			}
			else if (romlen==0x100000)
			{
				romlen=0x200000;
				writeromdata_byte(snestopc(0x00FFD7), 0x0B);
			}
			else if (isforcode) return -1;//no point creating freespace that can't be used
			else if (romlen==0x200000 || romlen==0x300000)
			{
				romlen=0x400000;
				writeromdata_byte(snestopc(0x00FFD7), 0x0C);
			}
			else return -1;
			autoexpand=false;
			goto rebootlorom;
		}
	}
	if (mapper==hirom)
	{
		if (isforcode) return -1;
		return trypcfreespace(0, romlen, size, 0xFFFF, align?0xFFFF:0, freespacebyte);
	}
	if (mapper==exlorom)
	{
		// RPG Hacker: Not really 100% sure what to do here, but I suppose this simplified code will do
		// and we won't need all the complicated stuff from LoROM above?
		if (isforcode) return -1;
		return trypcfreespace(0, romlen, size, 0x7FFF, align ? 0x7FFF : 0, freespacebyte);
	}
	if (mapper==exhirom)
	{
		if (isforcode) return -1;
		return trypcfreespace(0, romlen, size, 0xFFFF, align?0xFFFF:0, freespacebyte);
	}
	if (mapper==sfxrom)
	{
		if (!isforcode) return -1;
		// try not to overwrite smw stuff
		return trypcfreespace(0x80000, romlen, size, 0x7FFF, align?0x7FFF:0, freespacebyte);
	}
	if (mapper==sa1rom)
	{
	rebootsa1rom:
		int nextbank=-1;
		for (int i=0;i<8;i++)
		{
			if (i&2) continue;
			if (sa1banks[i]+0x100000>romlen)
			{
				if (sa1banks[i]<=romlen && sa1banks[i]+0x100000>romlen) nextbank=sa1banks[i];
				continue;
			}
			int pos=trypcfreespace(sa1banks[i]?sa1banks[i]:0x80000, sa1banks[i]+0x100000, size, 0x7FFF, align?0x7FFF:0, freespacebyte);
			if (pos>=0) return pos;
		}
		if (autoexpand && nextbank>=0)
		{
			unsigned char x7FD7[]={0, 0x0A, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D};
			romlen=nextbank+0x100000;
			writeromdata_byte(0x7FD7, x7FD7[romlen>>20]);
			autoexpand=false;
			goto rebootsa1rom;
		}
	}
	if (mapper==bigsa1rom)
	{
		if(!isforcode && romlen > 0x400000)
		{
			int pos=trypcfreespace(0x400000, romlen, size, 0xFFFF, align?0xFFFF:0, freespacebyte);
			if(pos>=0) return pos;
		}
		int pos=trypcfreespace(0x080000, romlen, size, 0x7FFF, align?0x7FFF:0, freespacebyte);
		if(pos>=0) return pos;
	}
	return -1;
}

void WalkRatsTags(void(*func)(int loc, int len))
{
	int pos=snestopc(0x108000);
	while (pos<romlen)
	{
		if (!strncmp((const char*)romdata+pos, "STAR", 4) &&
					(romdata[pos+4]^romdata[pos+6])==0xFF && (romdata[pos+5]^romdata[pos+7])==0xFF)
		{
			func(pctosnes(pos+8), (romdata[pos+4]|(romdata[pos+5]<<8))+1);
			pos+=(romdata[pos+4]|(romdata[pos+5]<<8))+1+8;
		}
		else pos++;
	}
}

void WalkMetadata(int loc, void(*func)(int loc, char * name, int len, const unsigned char * contents))
{
	int pcoff=snestopc(loc);
	if (strncmp((const char*)romdata+pcoff-8, "STAR", 4)) return;
	const unsigned char * metadata=romdata+pcoff;
	while (isupper(metadata[0]) && isupper(metadata[1]) && isupper(metadata[2]) && isupper(metadata[3]))
	{
		if (!strncmp((const char*)metadata, "STOP", 4))
		{
			metadata=romdata+pcoff;
			while (isupper(metadata[0]) && isupper(metadata[1]) && isupper(metadata[2]) && isupper(metadata[3]))
			{
				if (!strncmp((const char*)metadata, "STOP", 4))
				{
					break;
				}
				func(pctosnes((int)(metadata-romdata)), (char*)const_cast<unsigned char*>(metadata), metadata[4], metadata+5);
				metadata+=5+metadata[4];
			}
			break;
		}
		metadata+=5+metadata[4];
	}
}

int getsnesfreespace(int size, bool isforcode, bool autoexpand, bool respectbankborders, bool align, unsigned char freespacebyte)
{
	return pctosnes(getpcfreespace(size, isforcode, autoexpand, respectbankborders, align, freespacebyte));
}

bool openrom(const char * filename, bool confirm)
{
	closerom();
	thisfile=fopen(filename, "r+b");
	if (!thisfile)
	{
		openromerror = error_id_open_rom_failed;
		return false;
	}
	fseek(thisfile, 0, SEEK_END);
	header=false;
	if (strlen(filename)>4)
	{
		const char * fnameend=strchr(filename, '\0')-4;
		header=(!stricmp(fnameend, ".smc"));
	}
	romlen=ftell(thisfile)-(header*512);
	if (romlen<0) romlen=0;
	fseek(thisfile, header*512, SEEK_SET);
	romdata=(unsigned char*)malloc(sizeof(unsigned char)*16*1024*1024);
	int truelen=(int)fread(const_cast<unsigned char*>(romdata), 1u, (size_t)romlen, thisfile);
	if (truelen!=romlen)
	{
		openromerror = error_id_open_rom_failed;
		free(const_cast<unsigned char*>(romdata));
		return false;
	}
	memset(const_cast<unsigned char*>(romdata)+romlen, 0x00, (size_t)(16*1024*1024-romlen));
	if (confirm && snestopc(0x00FFC0)+21<(int)romlen && strncmp((const char*)romdata+snestopc(0x00FFC0), "SUPER MARIOWORLD     ", 21))
	{
		closerom(false);
		openromerror = header ? error_id_open_rom_not_smw_extension : error_id_open_rom_not_smw_header;
		return false;
	}
	return true;
}

uint32_t closerom(bool save)
{
	uint32_t romCrc = 0;
	if (thisfile && save && romlen)
	{
		fseek(thisfile, header*512, SEEK_SET);
		fwrite(const_cast<unsigned char*>(romdata), 1, (size_t)romlen, thisfile);

		// do a quick re-read of the header, and include that in the crc32 calculation if necessary
		{
			uint8_t* filedata = (uint8_t*)malloc(sizeof(uint8_t) * (romlen + header * 512));
			if (header)
			{
				fseek(thisfile, 0, SEEK_SET);
				fread(filedata, sizeof(uint8_t), 512, thisfile);
			}
			memcpy(filedata + (header * 512), romdata, sizeof(uint8_t) * (size_t)romlen);
			romCrc = crc32(filedata, (unsigned int)(romlen + header * 512));
			free(filedata);
		}
	}
	if (thisfile) fclose(thisfile);
	if (romdata) free(const_cast<unsigned char*>(romdata));
	thisfile= nullptr;
	romdata= nullptr;
	romlen=0;
	return romCrc;
}

static unsigned int getchecksum()
{
	unsigned int checksum=0;
	if((romlen & (romlen-1)) == 0)
	{
		// romlen is a power of 2, just add up all the bytes
		for (int i=0;i<romlen;i++) checksum+=romdata[i];
	}
	else
	{
		// assume romlen is the sum of 2 powers of 2 - i haven't seen any real rom that isn't,
		// and if you make such a rom, fixing its checksum is your problem.
		int firstpart = bitround(romlen) >> 1;
		int secondpart = romlen - firstpart;
		int repeatcount = firstpart / secondpart;
		unsigned int secondpart_sum = 0;
		for(int i = 0; i < firstpart; i++) checksum += romdata[i];
		for(int i = firstpart; i < romlen; i++) secondpart_sum += romdata[i];
		checksum += secondpart_sum * repeatcount;
	}
	return checksum&0xFFFF;
}

bool goodchecksum()
{
	int checksum=(int)getchecksum();
	return ((romdata[snestopc(0x00FFDE)]^romdata[snestopc(0x00FFDC)])==0xFF) && ((romdata[snestopc(0x00FFDF)]^romdata[snestopc(0x00FFDD)])==0xFF) &&
					((romdata[snestopc(0x00FFDE)]&0xFF)==(checksum&0xFF)) && ((romdata[snestopc(0x00FFDF)]&0xFF)==((checksum>>8)&0xFF));
}

void fixchecksum()
{
	// randomdude999: clear out checksum bytes before recalculating checksum, this should make it correct on roms that don't have a checksum yet
	writeromdata(snestopc(0x00FFDC), "\xFF\xFF\0\0", 4);
	int checksum=(int)getchecksum();
	writeromdata_byte(snestopc(0x00FFDE), (unsigned char)(checksum&255));
	writeromdata_byte(snestopc(0x00FFDF), (unsigned char)((checksum>>8)&255));
	writeromdata_byte(snestopc(0x00FFDC), (unsigned char)((checksum&255)^255));
	writeromdata_byte(snestopc(0x00FFDD), (unsigned char)(((checksum>>8)&255)^255));
}
