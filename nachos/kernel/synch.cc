/*! \file synch.cc
//  \brief Routines for synchronizing threads.
//
//      Three kinds of synchronization routines are defined here:
//      semaphores, locks and condition variables.
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation. We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts. While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
*/
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.


#include "kernel/system.h"
#include "kernel/scheduler.h"
#include "kernel/synch.h"

#define FALSE 0
#define TRUE 1

//----------------------------------------------------------------------
// Semaphore::Semaphore
/*! 	Initializes a semaphore, so that it can be used for synchronization.
//
// \param debugName is an arbitrary name, useful for debugging only.
// \param initialValue is the initial value of the semaphore.
*/
//----------------------------------------------------------------------
Semaphore::Semaphore(char* debugName, int initialValue)
{
  name = new char[strlen(debugName)+1];
  strcpy(name,debugName);
  value = initialValue;
  queue = new Listint;
  type = SEMAPHORE_TYPE;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
/*! 	De-allocates a semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
*/
//----------------------------------------------------------------------
Semaphore::~Semaphore()
{
  type = INVALID_TYPE;
  if (!queue->IsEmpty()) {
    DEBUG('s', (char *)"Destructor of semaphore \"%s\", queue is not empty!!\n",name);
    Thread *t =  (Thread *)queue->Remove();
    DEBUG('s', (char *)"Queue contents %s\n",t->GetName());
    queue->Append((void *)t);
  }
  ASSERT(queue->IsEmpty());
  delete [] name;
  delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
/*!
//      Decrement the value, and wait if it becomes < 0. Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
*/
//----------------------------------------------------------------------
void
Semaphore::P() {
  DEBUG('s', (char *)"Appel à P() \n");
  IntStatus oldLevel = g_machine->interrupt->SetStatus(INTERRUPTS_OFF);
  //printf("Thread %s P(%s) = %d (avant P)\n", g_current_thread->GetName(), this->getName(), this->value);
  DEBUG('s', (char *)"Thread %s P(%s) = %d (avant P)\n", g_current_thread->GetName(), this->getName(), this->value);
  this->value -= 1;
  DEBUG('s', (char *)"Thread %s P(%s) = %d (après P)\n", g_current_thread->GetName(), this->getName(), this->value);
  //printf("Thread %s P(%s) = %d (après P)\n", g_current_thread->GetName(), this->getName(), this->value);
  if(this->value < 0) {
    this->queue->Append(g_current_thread);
    DEBUG('s', (char *)"Nom thread dans P = %s\n", g_current_thread->GetName());
    g_current_thread->Sleep();
  }
  g_machine->interrupt->SetStatus(oldLevel);
}

//----------------------------------------------------------------------
// Semaphore::V
/*! 	Increment semaphore value, waking up a waiting thread if any.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that interrupts
//	are disabled when it is called.
*/
//----------------------------------------------------------------------
void
Semaphore::V() {
  DEBUG('s', (char *)"Appel à V() \n");
  IntStatus oldLevel = g_machine->interrupt->SetStatus(INTERRUPTS_OFF);
  DEBUG('s', (char *)"Thread %s V(%s) = %d (avant V)\n", g_current_thread->GetName(), this->getName(), this->value);
  //printf("Thread %s V(%s) = %d (avant V)\n", g_current_thread->GetName(), this->getName(), this->value);
  this->value += 1;
  DEBUG('s', (char *)"Thread %s V(%s) = %d (après V)\n", g_current_thread->GetName(), this->getName(), this->value);
  //printf("Thread %s V(%s) = %d (après V)\n", g_current_thread->GetName(), this->getName(), this->value);
  if(!this->queue->IsEmpty() && this->value >= 0) {
    Thread* tmp = (Thread*) this->queue->Remove();
    //printf("jsuis dans le if");
    g_scheduler->ReadyToRun(tmp);
    DEBUG('s', (char *)"la liste est Empty ? %d\n", this->queue->IsEmpty());
  }
  g_machine->interrupt->SetStatus(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Lock
/*! 	Initialize a Lock, so that it can be used for synchronization.
//      The lock is initialy free
//  \param "debugName" is an arbitrary name, useful for debugging.
*/
//----------------------------------------------------------------------
Lock::Lock(char* debugName) {
  name = new char[strlen(debugName)+1];
  strcpy(name,debugName);
  sleepqueue = new Listint;
  free = true;
  owner = NULL;
  type = LOCK_TYPE;
}


//----------------------------------------------------------------------
// Lock::~Lock
/*! 	De-allocate lock, when no longer needed. Assumes that no thread
//      is waiting on the lock.
*/
//----------------------------------------------------------------------
Lock::~Lock() {
  type = INVALID_TYPE;
  ASSERT(sleepqueue->IsEmpty());
  delete [] name;
  delete sleepqueue;
}

//----------------------------------------------------------------------
// Lock::Acquire
/*! 	Wait until the lock become free.  Checking the
//	state of the lock (free or busy) and modify it must be done
//	atomically, so we need to disable interrupts before checking
//	the value of free.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
*/
//----------------------------------------------------------------------
void Lock::Acquire() {
  DEBUG('s', (char *)"Lock Acquire() \n");
  IntStatus oldLevel = g_machine->interrupt->SetStatus(INTERRUPTS_OFF);
  if(!this->isHeldByCurrentThread()) {
  	if(!this->free) {
    	sleepqueue->Append(g_current_thread);
    	g_current_thread->Sleep();
  	}
  	DEBUG('s', (char *)"Thread %s Lock Acquire\n", g_current_thread->GetName());
  	this->free = FALSE;
  	this->owner = g_current_thread;
  }
  g_machine->interrupt->SetStatus(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Release
/*! 	Wake up a waiter if necessary, or release it if no thread is waiting.
//      We check that the lock is held by the g_current_thread.
//	As with Acquire, this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
*/
//----------------------------------------------------------------------
void Lock::Release() {
  DEBUG('s', (char *)"Lock Release() \n");
  IntStatus oldLevel = g_machine->interrupt->SetStatus(INTERRUPTS_OFF);
  if(this->isHeldByCurrentThread()) {
    if(!sleepqueue->IsEmpty()) {
      g_scheduler->ReadyToRun((Thread*) sleepqueue->Remove());
    }
    else {
      DEBUG('s', (char *)"Thread %s Lock Release\n", g_current_thread->GetName());
      this->owner = NULL;
      this->free = TRUE;
    }
  }
  g_machine->interrupt->SetStatus(oldLevel);
}

//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
/*! To check if current thread hold the lock
*/
//----------------------------------------------------------------------
bool Lock::isHeldByCurrentThread() {return (g_current_thread == owner);}

//----------------------------------------------------------------------
// Condition::Condition
/*! 	Initializes a Condition, so that it can be used for synchronization.
//
//    \param  "debugName" is an arbitrary name, useful for debugging.
*/
//----------------------------------------------------------------------
Condition::Condition(char* debugName) {
  name = new char[strlen(debugName)+1];
  strcpy(name,debugName);
  waitqueue = new Listint;
  type = CONDITION_TYPE;
}

//----------------------------------------------------------------------
// Condition::~Condition
/*! 	De-allocate condition, when no longer needed.
//      Assumes that nobody is waiting on the condition.
*/
//----------------------------------------------------------------------
Condition::~Condition() {
  type = INVALID_TYPE;
  ASSERT(waitqueue->IsEmpty());
  delete [] name;
  delete waitqueue;
}

//----------------------------------------------------------------------
// Condition::Wait
/*! Block the calling thread (put it in the wait queue).
//  This operation must be atomic, so we need to disable interrupts.
*/
//----------------------------------------------------------------------
void Condition::Wait() {
  IntStatus oldLevel = g_machine->interrupt->SetStatus(INTERRUPTS_OFF);
  waitqueue->Append(g_current_thread);
  g_current_thread->Sleep();
  g_machine->interrupt->SetStatus(oldLevel);
}

//----------------------------------------------------------------------
// Condition::Signal

/*! Wake up the first thread of the wait queue (if any).
// This operation must be atomic, so we need to disable interrupts.
*/
//----------------------------------------------------------------------
void Condition::Signal() {
  IntStatus oldLevel = g_machine->interrupt->SetStatus(INTERRUPTS_OFF);
  if(!waitqueue->IsEmpty()) {
    g_scheduler->ReadyToRun((Thread*) waitqueue->getFirst()->item);
    waitqueue->RemoveItem(waitqueue->getFirst()->item);
  }
  g_machine->interrupt->SetStatus(oldLevel);
}

//----------------------------------------------------------------------
/*! Condition::Broadcast
// wake up all threads waiting in the waitqueue of the condition
// This operation must be atomic, so we need to disable interrupts.
*/
//----------------------------------------------------------------------
void Condition::Broadcast() {
  IntStatus oldLevel = g_machine->interrupt->SetStatus(INTERRUPTS_OFF);
  while(!waitqueue->IsEmpty()) {
    g_scheduler->ReadyToRun((Thread*) waitqueue->getFirst()->item);
    waitqueue->RemoveItem(waitqueue->getFirst()->item);
  }
  g_machine->interrupt->SetStatus(oldLevel);
}
