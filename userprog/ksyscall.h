/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt()
{
	kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}

int SysCreate(char *filename, int size)
{
  // return value
  // 1: success
  // 0: failed
  return kernel->interrupt->CreateFile(filename, size);
}

// When you finish the function "OpenAFile", you can remove the comment below.

OpenFileId SysOpen(char *name)
{
  OpenFileId fileId = kernel->interrupt->OpenFile(name);
  return fileId;
}

int SysWrite(char *buffer, int size, OpenFileId id)
{
    int numWritten = kernel->interrupt->WriteFile(buffer, size, id);
    return numWritten;
}

int SysRead(char *buffer, int size, OpenFileId id)
{
    int numRead = kernel->interrupt->ReadFile(buffer, size, id);
    return numRead;
}

int SysClose(OpenFileId id)
{
  int status = kernel->interrupt->CloseFile(id);
  return status;
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
