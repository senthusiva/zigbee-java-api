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
	APP_RESET,
	APP_APP_START,
	APP_SET_DRIVE_MODE,
	APP_CALL_TEMP_SENSOR,
	APP_READ_TEMP_SENSOR,
	APP_OUTPUT_TEMP_SENSOR,
	APP_CALL_HMD_SENSOR,
	APP_READ_HMD_SENSOR,
	APP_OUTPUT_HMD_SENSOR,
	APP_CALL_RESULT_REGISTER,
	APP_READ_CO2,
	APP_OUTPUT_DATA,
	APP_NOTHING,
	
	//------NETZWERK----------
	APP_STARTJOIN_NETWORK,
	APP_INIT_ENDPOINT,
	APP_INIT_TRANSMITDATA,
	
	//------Batteriestand-------
	APP_Batterylevel,
	APP_Calculate_Battery,
	//------transmits------------
	APP_TRANSMIT_Battery,
	APP_TRANSMIT_TEMP,
	APP_TRANSMIT_HMD,
	APP_TRANSMIT_CO2,
	APP_TRANSMIT_VOC
} AppState_t;
#endif
// eof app.h