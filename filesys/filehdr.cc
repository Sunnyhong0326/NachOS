// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"
#include "pbitmap.h"
//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader()
{
	numBytes = -1;
	numSectors = -1;
	memset(dataSectors, -1, sizeof(dataSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	// nothing to do now
}


//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
int FileHeader::MultiLevelAllocate(PersistentBitmap *freeMap, int totalSectors, int level, FileHeader *hdr){
    if(level != 0){
        int left = totalSectors;
        int use = 0;
        int time = 0;
        while(left > 0 && time < 30){
            hdr->dataSectors[time] = freeMap->FindAndSet();
            DEBUG('f', "index block sector # : " << hdr->dataSectors[time] << "\n");
            FileHeader *subHdr = new FileHeader;
            int have_use = MultiLevelAllocate(freeMap, left, level-1, subHdr);
            DEBUG('f', "Write back to : " << hdr->dataSectors[time] << ", namely " << "[" << time << "]"<< " in previous index block position\n");
            subHdr->WriteBack(hdr->dataSectors[time]);
            DEBUG('f', "successfully write back " << "\n");
            left -= have_use;
            use += have_use;
            time++;
        }
		hdr->numSectors = time;
		hdr->numBytes = use * 128;
        return use;
    }
    else{
        int numSectors = totalSectors > 30 ? 30 : totalSectors;
        DEBUG('f', "At level 0, with total sectors: " << totalSectors << "\n");
        for (int i = 0; i < numSectors; i++)
    		{
    			hdr->dataSectors[i] = freeMap->FindAndSet();
    			// since we checked that there was enough free space,
    			// we expect this to succeed
                //DEBUG("f",hdr->dataSectors[i]);
    			ASSERT(hdr->dataSectors[i] >= 0);
    		}
		hdr->numSectors = numSectors;
		hdr->numBytes = numSectors * 128;
		return numSectors;
    }
}

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
	numBytes = fileSize;
	int totalSectors = divRoundUp(fileSize, SectorSize);
	DEBUG('f', "Going to allocate " << numBytes << " bytes\n");
	DEBUG('f', "total # of sectors: " << totalSectors << "\n");

	if (freeMap->NumClear() < totalSectors)
		return FALSE; // not enough space

    int use;
	if (fileSize > MaxFileSize3){
        DEBUG('f', "Going to level : " << 4 << "\n");
        use = MultiLevelAllocate(freeMap, totalSectors, 3, this);
	} else if (fileSize > MaxFileSize2) {
        DEBUG('f', "Going to level : " << 3 << "\n");
        use = MultiLevelAllocate(freeMap, totalSectors, 2, this);
	} else if (fileSize > MaxFileSize1){
	    DEBUG('f', "Going to level : " << 2 << "\n");
		use = MultiLevelAllocate(freeMap, totalSectors, 1, this);
	} else {
	    DEBUG('f', "Going to level : " << 1 << "\n");
        use = MultiLevelAllocate(freeMap, totalSectors, 0, this);
	}

    DEBUG('f', "successfully allocate: " << use << " sectors \n");
    printf("**********FileHeader Allocated size: %d**********\n", fileSize);

	return TRUE;
}

//bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
//{
//    numBytes = fileSize;
//    numSectors  = divRoundUp(fileSize, SectorSize);
//    if (freeMap->NumClear() < numSectors)
//	return FALSE;		// not enough space
//
//    for (int i = 0; i < numSectors; i++) {
//	dataSectors[i] = freeMap->FindAndSet();
//	// since we checked that there was enough free space,
//	// we expect this to succeed
//	ASSERT(dataSectors[i] >= 0);
//    }
//    return TRUE;
//}
//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
	for (int i = 0; i < numSectors; i++)
	{
		ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
		freeMap->Clear((int)dataSectors[i]);
	}
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
	kernel->synchDisk->ReadSector(sector, (char *)this);

	/*
		MP4 Hint:
		After you add some in-core informations, you will need to rebuild the header's structure
	*/
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
	kernel->synchDisk->WriteSector(sector, (char *)this);

	/*
		MP4 Hint:
		After you add some in-core informations, you may not want to write all fields into disk.
		Use this instead:
		char buf[SectorSize];
		memcpy(buf + offset, &dataToBeWritten, sizeof(dataToBeWritten));
		...
	*/
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::FindSector(int offset, int level, FileHeader *hdr)  //offset -> # of Sectors
{
	if (level == 3){
        FileHeader *subhdr = new FileHeader;
        subhdr->FetchFrom(hdr->dataSectors[offset/27000]);
        return FindSector(offset%27000, level-1, subhdr);
	} else if (level == 2) {
        FileHeader *subhdr = new FileHeader;
        subhdr->FetchFrom(hdr->dataSectors[offset/900]);
        return FindSector(offset%900, level-1, subhdr);
	} else if (level == 1) {
        FileHeader *subhdr = new FileHeader;
        subhdr->FetchFrom(hdr->dataSectors[offset/30]);
        return FindSector(offset%30, level-1, subhdr);
	} else {
		return (hdr->dataSectors[offset]);
	}
}

int FileHeader::ByteToSector(int offset)  //offset -> # of Sectors
{
	DEBUG('d', "*********************offset: " << offset << "\n");
	DEBUG('d', "*********************numbytes: " << numBytes << "\n");
	int place;
	if (numBytes > MaxFileSize3){
        place = FindSector(offset, 3, this);
	} else if (numBytes > MaxFileSize2) {
        place = FindSector(offset, 2, this);
	} else if (numBytes > MaxFileSize1){
		place = FindSector(offset, 1, this);
	} else {
        place =  FindSector(offset, 0, this);
	}
	DEBUG('d', "******************************************place: " << place << "\n");
	return place;
}

//int FileHeader::ByteToSector(int offset)
//{
//    return(dataSectors[offset / SectorSize]);
//}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
	int i, j, k;
	char *data = new char[SectorSize];

	printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
	for (i = 0; i < numSectors; i++)
		printf("%d ", dataSectors[i]);
	printf("\nFile contents:\n");
	for (i = k = 0; i < numSectors; i++)
	{
		kernel->synchDisk->ReadSector(dataSectors[i], data);
		for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
		{
			if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
				printf("%c", data[j]);
			else
				printf("\\%x", (unsigned char)data[j]);
		}
		printf("\n");
	}
	delete[] data;
}
