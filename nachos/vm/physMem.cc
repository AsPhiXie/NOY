//-----------------------------------------------------------------
/*! \file mem.cc
//  \brief Routines for the physical page management
*/
//
//  Copyright (c) 1999-2000 INSA de Rennes.
//  All rights reserved.  
//  See copyright_insa.h for copyright notice and limitation 
//  of liability and disclaimer of warranty provisions.
//-----------------------------------------------------------------

#include <unistd.h>
#include "vm/physMem.h"

//-----------------------------------------------------------------
// PhysicalMemManager::PhysicalMemManager
//
/*! Constructor. It simply clears all the page flags and inserts them in the
// free_page_list to indicate that the physical pages are free
*/
//-----------------------------------------------------------------
PhysicalMemManager::PhysicalMemManager() {

  long i;

  tpr = new struct tpr_c[g_cfg->NumPhysPages];

  for (i=0;i<g_cfg->NumPhysPages;i++) {
    tpr[i].free=true;
    tpr[i].locked=false;
    tpr[i].owner=NULL;
    free_page_list.Append((void*)i);
  }
  i_clock=-1;
}

PhysicalMemManager::~PhysicalMemManager() {
  // Empty free page list
  int64_t page;
  while (!free_page_list.IsEmpty()) page =  (int64_t)free_page_list.Remove();

  // Delete physical page table
  delete[] tpr;
}

//-----------------------------------------------------------------
// PhysicalMemManager::RemovePhysicalToVitualMapping
//
/*! This method releases an unused physical page by clearing the
//  corresponding bit in the page_flags bitmap structure, and adding
//  it in the free_page_list.
//
//  \param num_page is the number of the real page to free
*/
//-----------------------------------------------------------------
void PhysicalMemManager::RemovePhysicalToVirtualMapping(long num_page) {
  
  // Check that the page is not already free 
  ASSERT(!tpr[num_page].free);

  // Update the physical page table entry
  tpr[num_page].free=true;
  tpr[num_page].locked=false;
  if (tpr[num_page].owner->translationTable!=NULL) 
    tpr[num_page].owner->translationTable->clearBitValid(tpr[num_page].virtualPage);

  // Insert the page in the free list
  free_page_list.Prepend((void*)num_page);
}

//-----------------------------------------------------------------
// PhysicalMemManager::UnlockPage
//
/*! This method unlocks the page numPage, after
//  checking the page is in the locked state. Used
//  by the page fault manager to unlock at the
//  end of a page fault (the page cannot be evicted until
//  the page fault handler terminates).
//
//  \param num_page is the number of the real page to unlock
*/
//-----------------------------------------------------------------
void PhysicalMemManager::UnlockPage(long num_page) {
  ASSERT(num_page<g_cfg->NumPhysPages);
  ASSERT(tpr[num_page].locked==true);
  ASSERT(tpr[num_page].free==false);
  tpr[num_page].locked = false;
}

//-----------------------------------------------------------------
// PhysicalMemManager::ChangeOwner
//
/*! Change the owner of a page
//
//  \param owner is a pointer on new owner (Thread *)
//  \param numPage is the concerned page
*/
//-----------------------------------------------------------------
void PhysicalMemManager::ChangeOwner(long numPage, Thread* owner) {
  // Update statistics
  g_current_thread->GetProcessOwner()->stat->incrMemoryAccess();
  // Change the page owner
  tpr[numPage].owner = owner->GetProcessOwner()->addrspace;
}

//-----------------------------------------------------------------
// PhysicalMemManager::AddPhysicalToVirtualMapping 
//
/*! This method returns a new physical page number. If there is no
//  page available, it evicts one page (page replacement algorithm).
//
//  NB: this method locks the newly allocated physical page such that
//      it is not stolen during the page fault resolution. Don't forget
//      to unlock it
//
//  \param owner address space (for backlink)
//  \param virtualPage is the number of virtualPage to link with physical page
//  \return A new physical page number.
*/
//-----------------------------------------------------------------
int PhysicalMemManager::AddPhysicalToVirtualMapping(AddrSpace* owner,int virtualPage) {
	#ifdef ETUDIANTS_TP
		int taillePage = g_cfg->PageSize;
		int PageReel = this->FindFreePage();
		if(PageReel == -1){
			PageReel = this->EvictPage();
		}
		printf("pageReel = %x\n", PageReel);
		this->tpr[PageReel].locked = true;
		this->tpr[PageReel].virtualPage = virtualPage;
		this->tpr[PageReel].owner = owner;
		
		g_machine->mmu->translationTable->setPhysicalPage(virtualPage, PageReel);
		g_machine->mmu->translationTable->setBitValid(virtualPage);
		return PageReel;
		
	#endif
	#ifndef ETUDANTS_TP
  printf("**** Warning: function AddPhysicalToVirtualMapping is not implemented\n");
  exit(-1);
  return (0);
  #endif
}

//-----------------------------------------------------------------
// PhysicalMemManager::FindFreePage
//
/*! This method returns a new physical page number, if it finds one
//  free. If not, return -1. Does not run the clock algorithm.
//
//  \return A new free physical page number.
*/
//-----------------------------------------------------------------
int PhysicalMemManager::FindFreePage() {
  int64_t page;

  // Check that the free list is not empty
  if (free_page_list.IsEmpty())
    return -1;

  // Update statistics
  g_current_thread->GetProcessOwner()->stat->incrMemoryAccess();
  
  // Get a page from the free list
  page = (int64_t)free_page_list.Remove();
  
  // Check that the page is really free
  ASSERT(tpr[page].free);
  
  // Update the physical page table
  tpr[page].free = false;

  return page;
}

//-----------------------------------------------------------------
// PhysicalMemManager::EvictPage
//
/*! This method implements page replacement, using the well-known
//  clock algorithm.
//
//  \return A new free physical page number.
*/
//-----------------------------------------------------------------
int PhysicalMemManager::EvictPage() {
	#ifdef ETUDIANTS_TP
	printf("/**********************************************************************/\n");
	static int i = 0;
	int taillePage = g_cfg->PageSize;
	AddrSpace* addrSpaceLocal = g_current_thread->GetProcessOwner()->addrspace;
	int result;
	//1er tour de table
	int boolean = 1;
	while(boolean) {
		AddrSpace* addrSpace = tpr[i].owner;
		TranslationTable* transTabDei = tpr[i].owner->translationTable;
		if(&addrSpace != &addrSpaceLocal) {
			//On met les bits U à 0 si ils sont égales à 1
			if(transTabDei->getBitU(tpr[i].virtualPage) == 1) {
				transTabDei->clearBitU(tpr[i].virtualPage);
			}
			else if(transTabDei->getBitU(tpr[i].virtualPage) == 0 && tpr[i].locked == false) {
				tpr[i].locked = true;
				tpr[i].owner = g_current_thread->GetProcessOwner()->addrspace;
				if(transTabDei->getBitM(tpr[i].virtualPage) == 1) {
					if(transTabDei->getBitSwap(tpr[i].virtualPage) == 1) {
						g_swap_manager->PutPageSwap(transTabDei->getAddrDisk(tpr[i].virtualPage), (char*)&g_machine->mainMemory[i*taillePage]);
					}
					else {
						int addrDisk = g_swap_manager->PutPageSwap(-1, (char*)&g_machine->mainMemory[i*taillePage]);
						printf("addrDisk = %d\n", addrDisk);
						transTabDei->setAddrDisk(tpr[i].virtualPage, addrDisk);
					}
					transTabDei->clearBitValid(tpr[i].virtualPage);
				}
				boolean = 0;
			}
		}
		result = i;
		i++;
		i = i%g_cfg->NumPhysPages;
	}
	return result;
	#endif
	#ifndef ETUDIANTS_TP
  printf("**** Warning: page replacement algorithm is not implemented yet\n");
    exit(-1);
  #endif
}

//-----------------------------------------------------------------
// PhysicalMemManager::Print
//
/*! print the current status of the table of physical pages
//
//  \param rpage number of real page
*/
//-----------------------------------------------------------------

void PhysicalMemManager::Print(void) {
  int i;

  printf("Contents of TPR (%d pages)\n",g_cfg->NumPhysPages);
  for (i=0;i<g_cfg->NumPhysPages;i++) {
    printf("Page %d free=%d locked=%d virtpage=%d owner=%lx U=%d M=%d\n",
	   i,
	   tpr[i].free,
	   tpr[i].locked,
	   tpr[i].virtualPage,
	   (long int)tpr[i].owner,
	   (tpr[i].owner!=NULL) ? tpr[i].owner->translationTable->getBitU(tpr[i].virtualPage) : 0,
	   (tpr[i].owner!=NULL) ? tpr[i].owner->translationTable->getBitM(tpr[i].virtualPage) : 0);
  }
}
