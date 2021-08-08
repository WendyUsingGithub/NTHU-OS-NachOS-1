# NachOS MP2

## Trace code

### (a) threads/kernel.cc​ Kernel::ExecAll()​
Starting from "threads/kernel.cc​ Kernel::ExecAll()​" is called.
 
#### 1. int main(int argc, char **argv)
**threads/main.cc**

kernel 宣告為全域變數，整個 NachOS file 都可以共享。

```c
Kernel *kernel; /* global variable */
```

```c
int main(int argc, char **argv)
{
    kernel = new Kernel(argc, argv);
    kernel->Initialize();
    ... 
    kernel->ExecAll();
}
```

當我們在命令列下指令 -e halt，halt 即是我們想執行的可執行檔。

```c
/* threads/kernel.cc */

Kernel::Kernel(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        ...
        else if (strcmp(argv[i], "-e") == 0) {
            execfile[++execfileNum]= argv[++i];
            cout << execfile[execfileNum] << "\n";
        }     
}
```

```c
/* threads/kernel.h */

class Kernel {
public:
    Thread *currentThread; /* the thread holding CPU */
private:
	  Thread* t[10]; /* 10 threads at most */
	  char* execfile[10]; /* executable file name */
};
```

```c
/* threads/kernel.cc */

void Kernel::Initialize()
{
    currentThread = new Thread("main", threadNum++);		
    currentThread->setStatus(RUNNING); 
}
```

#### 2. void Kernel::ExecAll()
**threads/kernel.cc**​ 

把所有可執行檔丟進 Exec() 執行。

```c
void Kernel::ExecAll()
{
    for (int i = 1; i <= execfileNum; i++) {
        int a = Exec(execfile[i]);
    }
    currentThread->Finish();
}
```

#### 3. int Kernel::Exec(char* name)
**threads/kernel.cc**​ 

每一個可執行檔都需要 new 一個新的 thread 來執行。

```c
int Kernel::Exec(char* name)
{
    t[threadNum] = new Thread(name, threadNum);
    t[threadNum]->space = new AddrSpace();    
    t[threadNum]->Fork((VoidFunctionPtr)&ForkExecute,
                       (void*)t[threadNum]);
    threadNum++;
    return threadNum-1;
}
```

#### 4. void Thread::Fork(VoidFunctionPtr func, void *arg)
**threads/thread.cc**

建立 thread 執行所需要的 stack，並把 thread 丟進 readyList 等待執行。

```c
void Thread::Fork(VoidFunctionPtr func, void *arg)
{
    Interrupt *interrupt = kernel->interrupt;
    Scheduler *scheduler = kernel->scheduler;
    IntStatus oldLevel;
    
    StackAllocate(func, arg);

    oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);
    (void) interrupt->SetLevel(oldLevel);
}   
```

#### 5. void Scheduler::ReadyToRun (Thread *thread)
**threads/scheduler.cc**

將 thread 加入 readyList，當 scheduler 需要安排 thread 來執行的時候，就從 readyList dequeue 一個 thread 來執行。

```c
void Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    thread->setStatus(READY);
    readyList->Append(thread);
}
```

```c
class Scheduler {
    Thread* FindNextToRun();
    List<Thread *> *readyList;	
};
```

### (b) "threads/thread.cc thread::Sleep
Starting from "threads/thread.cc thread::Sleep​" is called.

#### 1. void Thread::Sleep (bool finishing)
**threads/thread.cc**

只有 currentThread 可以呼叫 Sleep()，currentThread 是目前正在執行的 thread，當currentThread 完成他的工作或是在等待 synchronization variable (Semaphore, Lock, or Condition)，會進入 Sleep()。 當 currentThread 進入 Sleep()，就能從 readyList 中 dequeue 一個 thread 成為 currentThread，使用 CPU。我們將進入 Sleep() 的 thread 的 state 存起來，並且進行 context switch 。當再次輪到在 Sleep() 的 thread 使用 CPU 時，Run() 才會 return。

```c
void Thread::Sleep (bool finishing)
{
    Thread *nextThread;
    ASSERT(this == kernel->currentThread);
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    status = BLOCKED;

    while ((nextThread = kernel->scheduler->FindNextToRun()) == NULL) {
        /* no one to run, wait for an interrupt */
        kernel->interrupt->Idle();
    }    

    /* returns when it's time for us to run */
    kernel->scheduler->Run(nextThread, finishing); 
}
```

#### 2. void Scheduler::Run (Thread *nextThread, bool finishing)
**threads/scheduler.cc**

```c
void Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
   
    if (finishing) {	
        /* mark that we need to delete current thread */
        ASSERT(toBeDestroyed == NULL);
	  toBeDestroyed = oldThread;
    }
    
    if (oldThread->space ！= NULL) {	
        /* save the user's state */
        oldThread->SaveUserState();
        oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();
    /* switch to the next thread */
    kernel->currentThread = nextThread;      
    /* nextThread is now running */
    nextThread->setStatus(RUNNING);

    /* This is a machine-dependent assembly language routine defined 
     * in switch.s */
    SWITCH(oldThread, nextThread);

    /* we're back, running oldThread */
    /* check if thread we were running before this one has finished
     * and needs to be cleaned up */
    CheckToBeDestroyed();
    
    if (oldThread->space != NULL) {	
        /* if there is an address space to restore, do it */
        oldThread->RestoreUserState();
	  oldThread->space->RestoreState();
    }
}
```

呼叫 SWITCH() 進行 context switch，在 SWITCH() 完成之後，並不會繼續執行 Run() 的下一行，因為控制權已經傳給 nextThread，下一行要被執行的指令是 nextThread 的指令，當 CheckToBeDestroyed() 被執行，代表已經又輪到這個 thread 使用 CPU，這中間可能已經在好幾個 thread 之間 context  switch 好幾次了。

```c
SWITCH(oldThread, nextThread);
```

### (c) machine/mipssim.cc Machine::Run()​
Starting from "machine/mipssim.cc Machine::Run()​" is called.

#### 1. void Machine::Run()
**machine/mippsim.cc**

```c
void Machine::Run()
{
    /* storage for decoded instruction */
    Instruction *instr = new Instruction; /

    kernel->interrupt->setStatus(UserMode);
    for (;;) {
        OneInstruction(instr);
        kernel->interrupt->OneTick();
}
```

#### 2. void Machine::OneInstruction(Instruction *instr)
**machine/mippsim.cc**

OneInstruction() 讀取當前指令，經過 decode()，我們可以得到 rs / rt / rd / opCode，依照 opCode 我們對 register[rs] 和 register[rt] 進行對應的操作，並且將結果寫入 register[rd]。

```c
void Machine::OneInstruction(Instruction *instr)
{
    int raw;
    /* Fetch instruction */ 
    if (!ReadMem(registers[PCReg], 4, &raw))
        return; /* exception occurred */

    instr->value = raw;
    instr->Decode();
    ...
    int pcAfter = registers[NextPCReg] + 4;
    unsigned int rs, rt, imm;

    /* Execute the instruction */
    switch (instr->opCode) {
        case OP_ADD:
        sum = registers[instr->rs] + registers[instr->rt];
	    registers[instr->rd] = sum;
	    break;
        ...
    }
    ...
    /* Advance program counters */
    registers[PrevPCReg] = registers[PCReg];
    registers[PCReg] = registers[NextPCReg];
    registers[NextPCReg] = pcAfter;
}
```

一個 Instruction 是 4 個 byte，恰好是一個 unsigned int 的大小，因此我們用一個 unsigned int 來儲存他。

```c
class Instruction {
public:
    void Decode();	
    unsigned int value;
    char opCode; /* Type of instruction */
    char rs, rt, rd; /* Three registers from instruction */
    int extra; /* Immediate or target or shamt field or offset */
};
```

#### 3. void Instruction::Decode()
**machine/mippsim.cc**

將我們存在 unsigned int value 裡的 instruction decode，我們可以得到這一到指令的 rs / rt / rd / opCode。

```c
void Instruction::Decode()
{
    OpInfo *opPtr;
    
    rs = (value >> 21) & 0x1f;
    rt = (value >> 16) & 0x1f;
    rd = (value >> 11) & 0x1f;
    opPtr = &opTable[(value >> 26) & 0x3f];
    opCode = opPtr->opCode;
    ...
}
```

## Implementation 

### (a) modify pageTable building

在修改之前，在 addrSpace 物件建立時就會建立 pageTable，預設 NachOS 只會運行一個可執行檔，因此這個 pageTable 涵蓋整個 physical memory，因此不需要做任何 virtualPage 和 physicalPage 之間的映射。 

```c
/* userprog/addrspace.cc */

AddrSpace::AddrSpace()
{
    pageTable = new TranslationEntry[NumPhysPages];
    for (int i = 0; i < NumPhysPages; i++) {
        pageTable[i].virtualPage = i;	
        pageTable[i].physicalPage = i;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  
    }

    /* zero out the entire address space */
    bzero(kernel->machine->mainMemory, MemorySize);
}
```

但是當我們想要同時執行一個以上的可執行檔時，會發生問題，因為單一個 thread 不能擁有整個 physical memory 的所有權，每個 thread 都只能擁有部份的 physical memory，因此我修改 pageTable 的建立，每個 thread 的 pageTable 的尺寸只包含他所需要的部份。但是在我們將可執行檔 load 進 memory 之前，我們並不知道這個可執行檔需要多少空間， 因此我將 pageTable 的建立延後，直到可執行檔的 header 讀入，我們知道這個可執行檔的大小之後，才建立 pageTable。

```c
/* userprog/addrspace.cc */

bool AddrSpace::Load(char *fileName) 
{
    ...
    numPages = divRoundUp(size, PageSize);

    /* setup pageTable after we know how much space the program needs */
    pageTable = new TranslationEntry[numPages];
    for (int i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;	
        pageTable[i].physicalPage = kernel->usedPhyPage->checkAndSet();
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false; 

        /* zero out this physical page */
        bzero(kernel->machine->mainMemory + pageTable[i].physicalPage * 
              PageSize, PageSize);
    }
}
```

新增 class UsedPhyPage 來管理 physical memory，使用一個 array 紀錄 physical page 是否已經被使用。

```c
/* threads/kernel.cc */

class UsedPhyPage {
public:
    int *pages; /* 0 for unused, 1 for used */
    int checkAndSet();
};
```

checkAndSet() 回傳一個未使用的 physical page 的 pageNum，如果整個 physical memory 都已經被使用，回傳 -1。使用這個方法建立 pageTable，每個 thread 管理自己的 pageTable，使用屬於自己的 memory space 。

```c
pageTable[i].physicalPage = kernel->usedPhyPage->checkAndSet();
```

### (b) modify executable file loading 

當我們要執行一個可執行檔，首先要把該檔案 load 進 memory，一個 NachOS 可執行檔分成四個部份，header / code / initData / readonlyData，我們首先讀出 header 來獲取檔案的 metadata，根據此 metadata 我們可以知道 code / initData / readonlyData 在檔案中的起始位置和大小。

```c
/* coff2noff/noff.h */

typedef struct noffHeader {
   int noffMagic;	/* should be NOFFMAGIC */
   Segment code; /* executable code segment */ 
   Segment initData; /* initialized data segment */
#ifdef RDATA
   Segment readonlyData;	/* read only data */
#endif
   Segment uninitData; /* uninitialized data segment, should be zero'ed 
                        * before use */
} NoffHeader;
```

```c
/* coff2noff/noff.h */

typedef struct segment {
    int virtualAddr; /* location of segment in virt addr space */
    int inFileAddr; /* location of segment in this file */
    int size; /* size of segment */
} Segment;
```

我們的目標是把每一段 segment 從他在檔案裡的位置，load 到他的 virtualAddr 所對應的 physicalAddr。未修改前因為整個 memory 只有一個 thread  在使用，所以 physicalAddr 和 virtualAddr 相同，並且因為這個可執行檔是第一個也是唯一一個 load 進 memory 的檔案，因此整個空間是連續且無人使用的，把可執行檔搬移到 memory，每一個 segment 只要搬移一次即可。

```c
/* userprog/addrspace.cc */

bool AddrSpace::Load(char *fileName) 
{
    /* read header file */
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    ...
#ifdef RDATA
    size = noffH.code.size + noffH.readonlyData.size +  
           noffH.initData.size + noffH.uninitData.size + UserStackSize;	
    ...
#endif
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    if (noffH.code.size > 0) {
        executable->ReadAt(
                  &(kernel->machine->mainMemory[noffH.code.virtualAddr]),
                  noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        ...
    }

#ifdef RDATA
    if (noffH.readonlyData.size > 0) {
        ...
    }
#endif
}
```

在修改之後，memory 由多個 thread 共用，因此在將資料從檔案 load 進 memory 之前，需要先使用 Translate() 將 virtualAddr 轉換成對應的 physicalAddr。因為有多個 thread 共用 memory，memory 中可能已經有些 page 被別的 thread 佔用了，因此資料需要一個 page 一個 page 的 load 進 memory ，而不能整段 segment (包含數個 page ) load 進 memory 。 以下圖為例，這個檔案有 4 個 page，每一個 page 有 5 個 bytes，virtal page 和 physical page 的對應如下，可以看到 virtual page 是連續的 (0 , 1 , 2 , 3)，但 physical page 不是連續的 (6 , 9 , 7 , 11)，因此我們在搬移資料時，一次只能搬一個 virtual page 的資料。

![](image1.svg)

假設我們所要搬移的資料從 virtual addr 8 - 17，可以看到他包含了 3 個 page，因此我們需要分成 3 次轉移資料，資料從 inFile addr 搬移到我們指定的 virtual addr 對應到的 physical addr，virtual addr 可以自訂，在 NachOS 裡 virtual addr 和 inFile addr 是相同的。 

![](image2.svg)

```c
/* userprog/addrspace.cc */

bool AddrSpace::Load(char *fileName) 
{
    ...
    unsigned int virtualAddr;
    unsigned int physicalAddr;
    int unReadSize;
    int chunkStart;
    int chunkSize;
    int inFilePosiotion;

    if (noffH.code.size > 0) {
        unReadSize = noffH.code.size;
        chunkStart = noffH.code.virtualAddr;
        chunkSize = 0;
        inFilePosiotion = 0;

        /* while still unread code */
        while(unReadSize > 0) {
            /* first chunk and last chunk might not be full */
            chunkSize =  calChunkSize(chunkStart, unReadSize); 
            /* mapping from virtual addr to physical addr */
            Translate(chunkStart, &physicalAddr, 1);
            
            executable->ReadAt(                     
                  &(kernel->machine->mainMemorymainMemory[physicalAddr]), 
                  chunkSize, noffH.code.inFileAddr + inFilePosiotion);

            unReadSize = unReadSize - chunkSize;
            chunkStart = chunkStart + chunkSize;
            inFilePosiotion = inFilePosiotion + chunkSize;
        }
    }
    if (noffH.initData.size > 0) {
        ...
    }

#ifdef RDATA
    if (noffH.readonlyData.size > 0) {
        ...        
    }
#endif
    ...
}
```

## Questions

### (a) allocate memory space & initiate memory content

- **How Nachos allocates the memory space for new thread(process)?**
- **How Nachos initializes the memory content of a thread(process), including loading the user binary code in the memory?**
- **How Nachos initializes the machine status (registers, etc) before running a thread(process)?**

每一個可執行檔都需要 new 一個新的 thread 來執行。

```c
/* threads/kernel.cc​ */

int Kernel::Exec(char* name)
{
    t[threadNum] = new Thread(name, threadNum);
    t[threadNum]->space = new AddrSpace();    
    t[threadNum]->Fork((VoidFunctionPtr)&ForkExecute,
                       (void*)t[threadNum]);
    threadNum++;
    return threadNum-1;
}
```

這個 thread 所要執行的 function 和他所需要的 argument 丟進 StackAllocate，此時我們要執行的 function 是 ForkExecute。

```c
/* threads/thread.cc */

void Thread::Fork(VoidFunctionPtr func, void *arg)
{
    Interrupt *interrupt = kernel->interrupt;
    Scheduler *scheduler = kernel->scheduler;
    IntStatus oldLevel;
    
    StackAllocate(func, arg);

    oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);
    (void) interrupt->SetLevel(oldLevel);
}   
```

建立和初始化這個 thread 的 execution stack。

```c
/* threads/thread.cc */

void Thread::StackAllocate (VoidFunctionPtr func, void *arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));
    ...
    machineState[PCState] = (void*)ThreadRoot;
    machineState[StartupPCState] = (void*)ThreadBegin;
    machineState[InitialPCState] = (void*)func;
    machineState[InitialArgState] = (void*)arg;
    machineState[WhenDonePCState] = (void*)ThreadFinish;
    ...
}
```

func 是之後要執行的函式。
machineState[InitialPCState] = (void*)func;


ForkExecute 是我們想執行的 func，將我們指定的可執行檔 load 進 memory，然後執行他。此時分配空間並且建立 pageTable。

```c
/* threads/kernel.cc */

void ForkExecute(Thread *t)
{
    /* allocate pageTable for this process */
    if ( !t->space->Load(t->getName()) ) {
    	return; /* executable not found */
    }
    t->space->Execute(t->getName());
}
```

將可執行檔讀入 memory。首先讀入可執行檔的 header，讀取 header 之後就能知道檔案大小，建立 pageTable。建立 pageTable 時會把該 thread 分配到的 physical page 清空。有了 pageTable 之後就能把可執行檔的其他部份 load 進 memory 了。

```c
t->space->Load(t->getName())
```

### (b) page table & address translation
- **How Nachos creates and manages the page table?**
- **How Nachos translates address?**

class UsedPhyPage 可以幫助建立和管理 physical memory，在完成 pageTable 之後使用 Translate 即可將 virtual addr 轉換為 physical addr。

```c
/* machine/translate.cc */

ExceptionType
Machine::Translate(int virtAddr, int* physAddr, int size, bool writing)
{
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    vpn = (unsigned) virtAddr / PageSize; /* virtual page number */
    offset = (unsigned) virtAddr % PageSize; /* offset within the page */
    ...
    entry = &pageTable[vpn]; /* translate using pageTable */
    ...
    pageFrame = entry->physicalPage;
    ...
    *physAddr = pageFrame * PageSize + offset;
    return NoException;
}
```

```c
class TranslationEntry {
public:
    int virtualPage;
    int physicalPage;
    ...
};
```

### (c) process control block

- **Which object​ in Nachos acts the role of ​ process control block?**

PCB (process control block) 包含和 process / thread 相關的一些資訊，例如 :
- process state
- program counter
- CPU register
- CPU scheduling info (e.g. priority)
- memory management info (e.g. base / limit register)
- and more

class Thread 有很多 PCB 應該包含的資訊，例如 :
- process state --> status
- memory management --> space
- CPU register --> machineState
- and more

```c
class Thread {
private:
    int *stackTop;	 /* the current stack pointer */
    void *machineState[MachineStateSize]; /* all registers except for 
                                           * stackTop */
  
    int *stack; /* bottom of the stack */ 
    ThreadStatus status;	 /* ready, running or blocked */
    char* name;
    int ID;
    int userRegisters[NumTotalRegs]; /* user-level CPU register state */
    ...
public:
    AddrSpace *space; /* user code this thread is running */
    ...
};
```

### (d) ReadyToRun queue
- **When and how does a thread get added into the ReadyToRun queue of Nachos CPU scheduler?**

```c
/* threads/thread.cc */

void Thread::Fork(VoidFunctionPtr func, void *arg)
{
    Interrupt *interrupt = kernel->interrupt;
    Scheduler *scheduler = kernel->scheduler;
    IntStatus oldLevel;
    
    StackAllocate(func, arg);

    oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);
    (void) interrupt->SetLevel(oldLevel);
}   
```

當一個 thread 已經建立好他的 stack，將他加入 readyList，當 scheduler 需要安排 thread 來執行的時候，就從 readyList dequeue 一個 thread 來執行。

```c
scheduler->ReadyToRun(this);
```

## reference

[1] shawn2000100/10810CS_342301_OperatingSystem
https://github.com/shawn2000100/10810CS_342301_OperatingSystem
[2] OS::NachOS::HW1
http://blog.terrynini.tw/tw/OS-NachOS-HW1/
[3] Nachos Project 3
https://www.csie.ntu.edu.tw/~b96201044/Project3_Slide_2010.pdf


