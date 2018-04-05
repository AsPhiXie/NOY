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
	int taillePage = g_cfg->PageSize;
	char bufSwap[128];
  	int bitSwap = g_machine->mmu->translationTable->getBitSwap(virtualPage);
  	int addrDisk = g_machine->mmu->translationTable->getAddrDisk(virtualPage);
  	int pageReel = g_physical_mem_manager->AddPhysicalToVirtualMapping(g_current_thread->GetProcessOwner()->addrspace, virtualPage);
  	if(bitSwap == 1){
  		while(addrDisk ==-1){;}
  		g_swap_manager->GetPageSwap(addrDisk, bufSwap);
  		g_physical_mem_manager->UnlockPage(pageReel);
  		memcpy(&g_machine->mainMemory[pageReel* taillePage], bufSwap, taillePage);
  	}
  	else if(bitSwap == 0 && addrDisk==-1){
  		g_physical_mem_manager->UnlockPage(pageReel);
  		/*bzero(bufSwap, taillePage);
  		g_machine->mainMemory[pageReel* taillePage] = bufSwap;*/
  		bzero(&g_machine->mainMemory[pageReel* taillePage], taillePage);
  	}
  	else if(bitSwap == 0 && addrDisk != -1){
  		OpenFile* file = g_current_thread->GetProcessOwner()->exec_file;
  		int BytesLu = file->ReadAt(bufSwap, taillePage, addrDisk);
  		memcpy(&g_machine->mainMemory[pageReel* taillePage], bufSwap, BytesLu);
  	}
  	return NO_EXCEPTION;
}




