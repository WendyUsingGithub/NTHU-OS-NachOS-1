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

void SysPrintInt(int number)
{
	kernel->synchConsoleOut->PutInt(number);
}

int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysCreate(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->interrupt->CreateFile(filename);
}

int SysOpen(char *filename)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->OpenFile(filename);
}

int SysWrite(char *buffer, int size, int id)
{
	// return value
	// positive integer: success
	// -1: failed
    return kernel->WriteFile(buffer, size, id);
}

int SysRead(char *buffer, int size, int id)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->ReadFile(buffer, size, id);
}


int SysClose(int id)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->CloseFile(id);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
