/*
IBEX UK LTD http://www.ibexuk.com
Electronic Product Design Specialists
RELEASED SOFTWARE

The MIT License (MIT)

Copyright (c) 2013, IBEX UK Ltd, http://ibexuk.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
//Project Name:		PIC32 Full duplex UART Driver



//This is a general use PIC24 comms UART handler.
//The driver is full duplex bus so TX and RX can occur independantly
//It automatically deals with variable packet length by including a length value in the packet.  It also automatically adds simple checksumming of the packet.


//Packet format:
//	CommandH | CommandL | LengthH | LengthL | 0-# Data Bytes | Checksum H | Checksum L




/*
##### ADD THIS TO THE MAIN FILE INITIALISE ROUTINE #####

	//----- SETUP USART 1 -----
	//Used for:
    UARTConfigure(COMMS_UART_NAME, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode(COMMS_UART_NAME, UART_INTERRUPT_ON_TX_NOT_FULL | UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl(COMMS_UART_NAME, COMMS_UART_LINE_CONTROL);
    UARTSetDataRate(COMMS_UART_NAME, PERIPHERAL_CLOCK_FREQUENCY, 57600);
    UARTEnable(COMMS_UART_NAME, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

	INTSetVectorPriority(INT_VECTOR_UART(COMMS_UART_NAME), INT_PRIORITY_LEVEL_5);				//1=lowest priority to 7=highest priority.  ISR function must specify same value
	//INTClearFlag(INT_SOURCE_UART_TX(COMMS_UART_NAME));
	//INTEnable(INT_SOURCE_UART_TX(COMMS_UART_NAME), INT_ENABLED);
	INTClearFlag(INT_SOURCE_UART_RX(COMMS_UART_NAME));
	INTEnable(INT_SOURCE_UART_RX(COMMS_UART_NAME), INT_ENABLED);
	INTClearFlag(INT_SOURCE_UART_ERROR(COMMS_UART_NAME));
	INTEnable(INT_SOURCE_UART_ERROR(COMMS_UART_NAME), INT_ENABLED);


#### ADD TO MAIN LOOP ####
	//----- PROCESS COMMS -----
	comms_process();


##### ADD THIS TO THE INTERRUPT SERVICE FUNCTION #####
	//----- HERE EVERY 1 mSec -----
	if (comms_rx_1ms_timeout_timer)
		comms_rx_1ms_timeout_timer--;

*/


/* TRANSMIT A PACKET EXAMPLE
	if (comms_tx_byte == 0)
	{
		p_buffer = &comms_tx_buffer[0];
		*p_buffer++ = CMD_GET_STATUS_REQUEST >> 8;			//Command
		*p_buffer++ = CMD_GET_STATUS_REQUEST & 0x00ff;
		*p_buffer++ = 0;									//Length
		*p_buffer++ = 0;
		*p_buffer++ = 'H';									//Data
		*p_buffer++ = 'e';
		*p_buffer++ = 'l';
		*p_buffer++ = 'l';
		*p_buffer++ = '0';

		comms_tx_packet(p_buffer - &comms_tx_buffer[0]);
	}
*/


//********************************
//********************************
//********** DO DEFINES **********
//********************************
//********************************
#ifndef AP_COMMS_INIT		//Do only once the first time this file is used
#define	AP_COMMS_INIT


#define	COMMS_TX_BUFFER_LENGTH			200				//<<< Set required length
#define	COMMS_RX_BUFFER_LENGTH			200				//<<< Set required length

#define	COMMS_UART_NAME					UART1		//Also set UART ID IN "void __ISR"
#define	COMMS_UART_STATUS				U1STA
#define COMMS_UART_LINE_CONTROL			(UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1)





//COMMANDS
#define	CMD_GET_STATUS_REQUEST				0x1001
#define	CMD_GET_STATUS_RESPONSE				0x1002


#endif



//*******************************
//*******************************
//********** FUNCTIONS **********
//*******************************
//*******************************
#ifdef AP_COMMS
//-----------------------------------
//----- INTERNAL ONLY FUNCTIONS -----
//-----------------------------------


//-----------------------------------------
//----- INTERNAL & EXTERNAL FUNCTIONS -----
//-----------------------------------------
//(Also defined below as extern)
void comms_process (void);
void comms_tx_packet (WORD packet_length);


#else
//------------------------------
//----- EXTERNAL FUNCTIONS -----
//------------------------------
extern void comms_process (void);
void comms_tx_packet (WORD packet_length);


#endif




//****************************
//****************************
//********** MEMORY **********
//****************************
//****************************
#ifdef AP_COMMS
//--------------------------------------------
//----- INTERNAL ONLY MEMORY DEFINITIONS -----
//--------------------------------------------
WORD comms_rx_byte;
WORD comms_rx_no_of_bytes_to_rx = 0xffff;
BYTE comms_rx_process_packet = 0;
WORD comms_tx_chksum;

//--------------------------------------------------
//----- INTERNAL & EXTERNAL MEMORY DEFINITIONS -----
//--------------------------------------------------
//(Also defined below as extern)
WORD comms_tx_byte;
WORD comms_tx_no_of_bytes_to_tx;
BYTE comms_tx_buffer[COMMS_TX_BUFFER_LENGTH];
BYTE comms_rx_buffer[COMMS_RX_BUFFER_LENGTH];
volatile WORD comms_rx_1ms_timeout_timer = 0;



#else
//---------------------------------------
//----- EXTERNAL MEMORY DEFINITIONS -----
//---------------------------------------
extern WORD comms_tx_byte;
extern WORD comms_tx_no_of_bytes_to_tx;
extern BYTE comms_tx_buffer[COMMS_TX_BUFFER_LENGTH];
extern BYTE comms_rx_buffer[COMMS_RX_BUFFER_LENGTH];
extern volatile WORD comms_rx_1ms_timeout_timer;


#endif






