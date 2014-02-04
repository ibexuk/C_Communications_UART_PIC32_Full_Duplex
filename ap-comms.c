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
//Visit http://www.embedded-code.com/source-code/communications/uart/pic32-uart-full-duplex for more information
//
//Project Name:		PIC32 Full duplex UART Driver




//ALL COMMS ARE DONE BY LOADING A BUFFER AND JUST LETTING THESE FUNCTIONS GET ON WITH IT

#include "main.h"					//Global data type definitions (see https://github.com/ibexuk/C_Generic_Header_File )
#define AP_COMMS
#include "ap-comms.h"







//***********************************
//***********************************
//********** PROCESS COMMS **********
//***********************************
//***********************************
void comms_process (void)
{
	BYTE *p_buffer;
	WORD command;


	//----- HAS A PACKET BEEN RECEVEID ? -----
	if (comms_rx_no_of_bytes_to_rx != 0)
		return;


	//---------------------------
	//----- PACKET RECEIVED -----
	//---------------------------

	//Packet format:
	//	CommandH | CommandL | LengthH | LengthL | 0-# Data Bytes | Checksum H | Checksum L

	//comms_rx_byte		The size of the received packet including the checksum

	p_buffer = &comms_rx_buffer[4];						//Set pointer to start of data area

	command = (WORD)comms_rx_buffer[0] << 8;
	command |= (WORD)comms_rx_buffer[1];

	switch (command)
	{
	case CMD_GET_STATUS_REQUEST:
		//------------------------------
		//----- GET STATUS REQUEST -----
		//------------------------------

		//Deal with RX
		// = *p_buffer++;


		break;


	//<<<<< ADD HANDLING OF OTHER PACKETS HERE


	}


	//Enable RX again
	comms_rx_byte = 0;
	comms_rx_no_of_bytes_to_rx = 0xffff;
}






//*************************************
//*************************************
//********** COMMS TX PACKET **********
//*************************************
//*************************************
//Call with:
//	comms_tx_byte			Check this is zero before loading comms_tx_buffer (confirms last tx is complete)
//	comms_tx_buffer[]		The packet data to send with the command in byte 0:1.  The Length will automatically be added to bytes 2:3 by this function
//	packet_length			The number of data bytes excluding the checksum (which is automatically added by the tx function)
void comms_tx_packet (WORD packet_length)
{

	//Packet format:
	//	CommandH | CommandL | LengthH | LengthL | 0-# Data Bytes | Checksum H | Checksum L

	//Check last tx is complete
	if (comms_tx_byte)
		return;

	if (packet_length > COMMS_TX_BUFFER_LENGTH)
		return;

	//----- START TX -----
	comms_tx_no_of_bytes_to_tx = packet_length;		//no of bytes to tx (excluding checksum)

	//Set the length bytes
	comms_tx_buffer[2] = (BYTE)(packet_length >> 8);
	comms_tx_buffer[3] = (BYTE)(packet_length & 0x00ff);

	//comms_rx_no_of_bytes_to_rx = 0xfffe;		//If you want to reset rx
	//comms_rx_1ms_timeout_timer = ;

	comms_tx_byte = 0;

	comms_tx_chksum = (WORD)comms_tx_buffer[0];
	
	INTClearFlag(INT_SOURCE_UART_TX(COMMS_UART_NAME));
	UARTSendDataByte(COMMS_UART_NAME, comms_tx_buffer[comms_tx_byte++]);	//Manually trigger the first tx
	INTEnable(INT_SOURCE_UART_TX(COMMS_UART_NAME), INT_ENABLED);
}






//********************************
//********************************
//********** INTERRUPTS **********
//********************************
//********************************
void __ISR(_UART_1_VECTOR, ipl5) Uart1InterruptHandler (void) 		//(ipl# must match the priority level assigned to the irq where its enabled)
{
	static BYTE data;
	static BYTE status;
	static WORD tx_checksum;
	static WORD rx_checksum;
	static WORD checksum_recevied;
	static WORD w_temp;
	static WORD rx_length_received;

	Nop();

	if (
	(INTGetEnable(INT_SOURCE_UART_TX(COMMS_UART_NAME))) &&
	(INTGetFlag(INT_SOURCE_UART_TX(COMMS_UART_NAME)))
	)
	{
		//----------------------------------
		//----------------------------------
		//---------- TX INTERRUPT ----------
		//----------------------------------
		//----------------------------------

		while (UARTTransmitterIsReady(COMMS_UART_NAME))
		{
			//if (comms_tx_byte == 0)		//Done in start tx function
			//{
			//	//----- SEND BYTE 0 -----
			//	comms_tx_chksum = (WORD)comms_tx_buffer[0];
			//	COMMS_TX_REG = (WORD)comms_tx_buffer[comms_tx_byte++];
			//}
			if (comms_tx_byte < comms_tx_no_of_bytes_to_tx)
			{
				//----- SEND NEXT DATA BYTE -----
				comms_tx_chksum += (WORD)comms_tx_buffer[comms_tx_byte];			//Add to checksum

				UARTSendDataByte(COMMS_UART_NAME, comms_tx_buffer[comms_tx_byte++]);
			}
			else if (comms_tx_byte == comms_tx_no_of_bytes_to_tx)
			{
				//----- SEND CHECKSUM HIGH -----
				UARTSendDataByte(COMMS_UART_NAME, (comms_tx_chksum >> 8));
				comms_tx_byte++;
			}
			else if (comms_tx_byte == (comms_tx_no_of_bytes_to_tx + 1))
			{
				//----- SEND CHECKSUM LOW -----
				UARTSendDataByte(COMMS_UART_NAME, (comms_tx_chksum & 0x00ff));
				comms_tx_byte++;

				//----- ALL BYTES SENT -----
				comms_tx_byte = 0;					//Reset tx byte counter (indicates we're done)
				comms_tx_no_of_bytes_to_tx = 0;

				//DISABLE TX IRQ
				INTEnable(INT_SOURCE_UART_TX(COMMS_UART_NAME), INT_DISABLED);
			}

		} //while (UARTTransmitterIsReady(COMMS_UART_NAME))
		INTClearFlag(INT_SOURCE_UART_TX(COMMS_UART_NAME));		//Do after sending bytes
	}



	if (
	(INTGetEnable(INT_SOURCE_UART_ERROR(COMMS_UART_NAME))) &&
	(INTGetFlag(INT_SOURCE_UART_ERROR(COMMS_UART_NAME)))
	)
	{
		//----------------------------------------
		//----------------------------------------
		//---------- RX ERROR INTERRUPT ----------
		//----------------------------------------
		//----------------------------------------
		status = UARTGetLineStatus(COMMS_UART_NAME);
		if (status & UART_PARITY_ERROR)
		{
			Nop();
		}
		if (status & UART_FRAMING_ERROR)
		{
			Nop();
		}
		if (status & UART_OVERRUN_ERROR)			//OERR must be cleared, clearing will reset RX FIFO
		{
			COMMS_UART_STATUS = (COMMS_UART_STATUS & ~UART_OVERRUN_ERROR);		//Clear OERR bit
		}

    	//Clear RX buffer of bad data
		while (UARTReceivedDataIsAvailable(COMMS_UART_NAME))
			data = UARTGetDataByte(COMMS_UART_NAME);
		INTClearFlag(INT_SOURCE_UART_RX(COMMS_UART_NAME));
		INTClearFlag(INT_SOURCE_UART_ERROR(COMMS_UART_NAME));

		if (comms_rx_no_of_bytes_to_rx != 0)		//Don't reset if there is a received packet waiting to be processed
		{
			comms_rx_byte = 0;
			comms_rx_no_of_bytes_to_rx = 0xffff;		//Reset to waiting for rx
		}
	}



	if (
	(INTGetEnable(INT_SOURCE_UART_RX(COMMS_UART_NAME))) &&
	(INTGetFlag(INT_SOURCE_UART_RX(COMMS_UART_NAME)))
	)
	{
		//----------------------------------
		//----------------------------------
		//---------- RX INTERRUPT ----------
		//----------------------------------
		//----------------------------------

		//Packet format:
		//	CommandH | CommandL | LengthH | LengthL | 0-# Data Bytes | Checksum H | Checksum L

		if (comms_rx_1ms_timeout_timer == 0)
		{
			comms_rx_byte = 0;
			comms_rx_no_of_bytes_to_rx = 0xffff;		//Reset to waiting for rx
		}

		comms_rx_1ms_timeout_timer = 50;			//<<<Force new packet reset if no data seen for # mS <<<<<<<<<<<<<<<<<<<<SET THIS SO RESET WILL OCCURS BETWEEN PACKETS IF WE START RECEIVEING AT THE WRONG MOMENT

		while (UARTReceivedDataIsAvailable(COMMS_UART_NAME))
		{
			//----- GET DATA BYTE -----
			data = UARTGetDataByte(COMMS_UART_NAME);

			if (comms_rx_byte == 0)
				rx_checksum = 0;

			if (comms_rx_byte < comms_rx_no_of_bytes_to_rx)
			{
				//----- DATA BYTE -----
				if (comms_rx_byte < COMMS_RX_BUFFER_LENGTH)
					comms_rx_buffer[comms_rx_byte] = data;

				rx_checksum += (WORD)data;

				if (comms_rx_byte == 2)			//BYTES 2:3 ARE LENGTH
				{
					rx_length_received = (WORD)data << 8;
				}
				else if (comms_rx_byte == 3)
				{
					rx_length_received |= (WORD)data;
					comms_rx_no_of_bytes_to_rx = rx_length_received;
				}

				comms_rx_byte++;
			}
			else if (comms_rx_byte == comms_rx_no_of_bytes_to_rx)
			{
				//----- CHECKSUM HIGH -----
				checksum_recevied = (WORD)data << 8;

				comms_rx_byte++;
			}
			else if (comms_rx_byte == (comms_rx_no_of_bytes_to_rx + 1))
			{
				//----- CHECKSUM LOW -----
				checksum_recevied |= (WORD)data;

				comms_rx_byte++;

				if (rx_checksum == checksum_recevied)
				{
					//----- CHECKSUM IS GOOD -----
					comms_rx_no_of_bytes_to_rx = 0;				//Flag that we have rx to process
				}
				else
				{
					//CHECKSUM IS BAD
					comms_rx_byte = 0;
					comms_rx_no_of_bytes_to_rx = 0xffff;		//Reset to waiting for rx
				}
			}

		}
		INTClearFlag(INT_SOURCE_UART_RX(COMMS_UART_NAME));				//Do after reading data as any data in RX FIFO will stop irq bit being cleared
	}
}










