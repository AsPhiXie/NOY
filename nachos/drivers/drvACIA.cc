/* \file drvACIA.cc
   \brief Routines of the ACIA device driver
//
//      The ACIA is an asynchronous device (requests return 
//      immediately, and an interrupt happens later on).  
//      This is a layer on top of the ACIA.
//      Two working modes are to be implemented in assignment 2:
//      a Busy Waiting mode and an Interrupt mode. The Busy Waiting
//      mode implements a synchronous IO whereas IOs are asynchronous
//      IOs are implemented in the Interrupt mode (see the Nachos
//      roadmap for further details).
//
//  Copyright (c) 1999-2000 INSA de Rennes.
//  All rights reserved.  
//  See copyright_insa.h for copyright notice and limitation 
//  of liability and disclaimer of warranty provisions.
//
*/

/*
	MASTER 1 SSR
	KHOUBI SINA
	BOUGAUD YVES

*/

/* Includes */

#include "kernel/system.h"         // for the ACIA object
#include "kernel/synch.h"
#include "machine/ACIA.h"
#include "drivers/drvACIA.h"

//-------------------------------------------------------------------------
// DriverACIA::DriverACIA()
/*! Constructor.
  Initialize the ACIA driver. In the ACIA Interrupt mode, 
  initialize the reception index and semaphores and allow 
  reception and emission interrupts. 
  In the ACIA Busy Waiting mode, simply inittialize the ACIA 
  working mode and create the semaphore.
  */
//-------------------------------------------------------------------------

DriverACIA::DriverACIA() {
	this->ind_send = -1;
	this->ind_rec = -1;
	if(g_cfg->ACIA == ACIA_INTERRUPT) {
		this->send_sema = new Semaphore("semaSend", 1);
		this->receive_sema = new Semaphore("semaReceive", 0);
		g_machine->acia->SetWorkingMode(REC_INTERRUPT);
	}
	else {
		this->send_sema = new Semaphore("semaSend", 1);
		this->receive_sema = new Semaphore("semaReceive", 1);
		g_machine->acia->SetWorkingMode(BUSY_WAITING);
	}
}

//-------------------------------------------------------------------------
// DriverACIA::TtySend(char* buff)
/*! Routine to send a message through the ACIA (Busy Waiting or Interrupt mode)
  */
//-------------------------------------------------------------------------

int DriverACIA::TtySend(char* buff) {
	 switch (g_cfg->ACIA) {
		case ACIA_BUSY_WAITING: {
			DEBUG('d', (char *)"TtySend Mode : BUSY_WAITING\n");
			this->ind_send = -1;
			do {
				while(g_machine->acia->GetOutputStateReg() == FULL) {;}
				this->ind_send++;
				g_machine->acia->PutChar(buff[this->ind_send]);
				DEBUG('d', (char *)"Character send = %c\n", buff[this->ind_send]);
			} while(buff[this->ind_send] != '\0');
			return this->ind_send;
		}
		
		case ACIA_INTERRUPT: {
        	this->send_sema->P();
        	this->ind_send = -1;
        	memcpy(this->send_buffer, buff, BUFFER_SIZE);
        	int result = strlen(this->send_buffer);
        	if(result >= BUFFER_SIZE) {
        		result = BUFFER_SIZE;
        	}
        	this->ind_send++;
        	g_machine->acia->SetWorkingMode(SEND_INTERRUPT);
		g_machine->acia->PutChar(this->send_buffer[this->ind_send]);
		DEBUG('d', (char *)"TtySend PutChar : %c\n", this->send_buffer[this->ind_send]);
		return result;
      	}
      	
      	default: {
        	DEBUG('d', (char *)"TtySend : Error : ACIA_NONE\n");
        	exit(-1);
        }
    }
}

//-------------------------------------------------------------------------
// DriverACIA::TtyReceive(char* buff,int length)
/*! Routine to reveive a message through the ACIA 
//  (Busy Waiting and Interrupt mode).
  */
//-------------------------------------------------------------------------

int DriverACIA::TtyReceive(char* buff, int lg) {
	switch (g_cfg->ACIA) {
		case ACIA_BUSY_WAITING: {
			DEBUG('d', (char *)"TtyReceive Mode : BUSY_WAITING\n");
			this->receive_sema->P();
			printf("lg = %d\n", lg);
			this->ind_rec = -1;
			do {
				while(g_machine->acia->GetInputStateReg() == EMPTY){;}
				this->ind_rec++;
				this->receive_buffer[this->ind_rec] = g_machine->acia->GetChar();
				DEBUG('d', (char *)"Character = %c\n", this->receive_buffer[this->ind_rec]);
				DEBUG('d', (char *)"Indice rec = %d\n", this->ind_rec);
			} while(this->ind_rec != lg && this->receive_buffer[this->ind_rec] != '\0');
			DEBUG('d', (char *)"Sorie de TtyReceive.\n");
			memcpy(buff, this->receive_buffer, lg);
			this->receive_sema->V();
			return this->ind_rec;
		}
		
		case ACIA_INTERRUPT: {
        	this->receive_sema->P();
        	int result = this->ind_rec;
        	this->ind_rec = -1;
        	memcpy(buff, this->receive_buffer, BUFFER_SIZE);
        	g_machine->acia->SetWorkingMode(REC_INTERRUPT);
        	return result;
      	}
      	
      	default: {
        	DEBUG('d', (char *)"TtyReceive : Error : ACIA_NONE\n");
        	exit(-1);
        }
    }
}


//-------------------------------------------------------------------------
// DriverACIA::InterruptSend()
/*! Emission interrupt handler.
  Used in the ACIA Interrupt mode only. 
  Detects when it's the end of the message (if so, releases the send_sema semaphore), else sends the next character according to index ind_send.
  */
//-------------------------------------------------------------------------

void DriverACIA::InterruptSend() {
	if(this->send_buffer[this->ind_send] != '\0') {
		this->ind_send++;
		g_machine->acia->PutChar(this->send_buffer[this->ind_send]);
		DEBUG('d', (char *)"InterruptSend PutChar : %c\n", this->send_buffer[this->ind_send]);
	}
	else {
		g_machine->acia->SetWorkingMode(REC_INTERRUPT);
		this->send_sema->V();
	}
}

//-------------------------------------------------------------------------
// DriverACIA::Interrupt_receive()
/*! Reception interrupt handler.
  Used in the ACIA Interrupt mode only. Reveices a character through the ACIA. 
  Releases the receive_sema semaphore and disables reception 
  interrupts when the last character of the message is received 
  (character '\0').
  */
//-------------------------------------------------------------------------

void DriverACIA::InterruptReceive() {
	char c = g_machine->acia->GetChar();
	DEBUG('d', (char *)"InterruptReceive GetChar : %c\n", c	);
	this->ind_rec++;
	this->receive_buffer[this->ind_rec] = c;
	DEBUG('d', (char *)"InterruptReceive receiveBuffer : %s\n", this->receive_buffer);
	if(c == '\0') {
		g_machine->acia->SetWorkingMode(SEND_INTERRUPT);
		this->receive_sema->V();
	}
}
