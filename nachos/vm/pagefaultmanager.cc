/*! \file pagefaultmanager.cc
Routines for the page fault managerPage Fault Manager
*/
//
//  Copyright (c) 1999-2000 INSA de Rennes.
//  All rights reserved.  
//  See copyright_insa.h for copyright notice and limitation 
//  of liability and disclaimer of warranty provisions.
//

#include "kernel/thread.h"
#include "vm/swapManager.h"
#include "vm/physMem.h"
#include "vm/pagefaultmanager.h"

PageFaultManager::PageFaultManager() {
}

// PageFaultManager::~PageFaultManager()
/*! Nothing for now
*/
PageFaultManager::~PageFaultManager() {
}

// ExceptionType PageFault(uint32_t virtualPage)
/*! 	
//	This method is called by the Memory Management Unit when there is a 
//      page fault. This method loads the page from :
//      - read-only sections (text,rodata) $\Rightarrow$ executive
//        file
//      - read/write sections (data,...) $\Rightarrow$ executive
//        file (1st time only), or swap file
//      - anonymous mappings (stack/bss) $\Rightarrow$ new
//        page from the MemoryManager (1st time only), or swap file
//
//	\param virtualPage the virtual page subject to the page fault
//	  (supposed to be between 0 and the
//        size of the address space, and supposed to correspond to a
//        page mapped to something [code/data/bss/...])
//	\return the exception (generally the NO_EXCEPTION constant)
*/  
ExceptionType PageFaultManager::PageFault(uint32_t virtualPage) {
	char bufSwap[128];
	int taillePage = g_cfg->PageSize;
	int bitIO = g_machine->mmu->translationTable->getBitIo(virtualPage);
	int bitSwap = g_machine->mmu->translationTable->getBitSwap(virtualPage);
  	int addrDisk = g_machine->mmu->translationTable->getAddrDisk(virtualPage);
  	
	while(bitIO == 1) {
		g_current_thread->Yield();
		bitIO = g_machine->mmu->translationTable->getBitIo(virtualPage);
	}
	int bitV = g_machine->mmu->translationTable->getBitValid(virtualPage);
	//printf("bit read allowed = %d\n", g_machine->mmu->translationTable->getBitReadAllowed(virtualPage));
	if(bitV == 0) {
  		int pageReel = g_physical_mem_manager->AddPhysicalToVirtualMapping(g_current_thread->GetProcessOwner()->addrspace, virtualPage);
		g_machine->mmu->translationTable->setBitIo(virtualPage);
  		if(bitSwap == 1){
  			while(addrDisk ==-1){;}
  			g_swap_manager->GetPageSwap(addrDisk, bufSwap);
  			memcpy(&g_machine->mainMemory[pageReel* taillePage], bufSwap, taillePage);
  		}
  		else if(bitSwap == 0 && addrDisk==-1){
  			bzero(&g_machine->mainMemory[pageReel* taillePage], taillePage);
  		}
  		else if(bitSwap == 0 && addrDisk > -1){
  			OpenFile* file = g_current_thread->GetProcessOwner()->exec_file;
  			//printf("addrDisk = %d\n", addrDisk);
  			int BytesLu = file->ReadAt(bufSwap, taillePage, addrDisk);
  			//printf("byteslu = %d\n", BytesLu);
  			memcpy(&g_machine->mainMemory[pageReel* taillePage], bufSwap, BytesLu);
  		}
  		g_machine->mmu->translationTable->clearBitIo(virtualPage);
  		g_physical_mem_manager->UnlockPage(pageReel);
  	}
  	return NO_EXCEPTION;
}

