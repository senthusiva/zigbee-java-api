/**************************************************************************//**
  \file app.h

  \brief Usart-Anwendung Headerdatei.

  \author
    Markus Krauﬂe

******************************************************************************/

#ifndef _APP_H
#define _APP_H

#define APP_SENDE_INTERVAL 1000

typedef enum{
	APP_INIT,
	APP_READ,
	APP_AUSGABE,
	APP_NOTHING_STATE,
	
	//------NETZWERK----------
	APP_STARTJOIN_NETWORK,
	APP_INIT_ENDPOINT,
	APP_INIT_TRANSMITDATA,
	
	//------Batteriestand-------
	//APP_Batterylevel,
	//APP_Calculate_Battery,
	
	//------transmits------------
	APP_TRANSMIT_Battery,
	APP_TRANSMIT_CO2
	
} AppState_t;
#endif
// eof app.h