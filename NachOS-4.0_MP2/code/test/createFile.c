#include "syscall.h"

int main(void)
{
	int success= Create("file0.test"); // 191012[J]: «áÄò¥i¥H¦A®³¨Ó´ú¸Õopen, write, read, close
	if (success != 1) MSG("Failed on creating file");
	MSG("Success on creating file0.test");
	Halt();
}