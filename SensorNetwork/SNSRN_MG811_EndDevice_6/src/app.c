/**************************************************************************//**
  \file app.c

  \brief Basis-Anwendung.

  \author Markus Krauﬂe

******************************************************************************/


#include <appTimer.h>
#include <math.h>
#include <zdo.h>
#include <app.h>
#include <sysTaskManager.h>
#include <usartManager.h>
#include <bspLeds.h>
#include <adc.h>

static uint8_t deviceType;
static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo);
static ZDO_StartNetworkReq_t networkParams;

static SimpleDescriptor_t simpleDescriptor;
static APS_RegisterEndpointReq_t endPoint;
static void initEndpoint(void);
void APS_DataInd(APS_DataInd_t *indData);

HAL_AppTimer_t receiveTimerLed;
HAL_AppTimer_t transmitTimerLed;
HAL_AppTimer_t transmitTimer;

static void receiveTimerLedFired(void);
static void transmitTimerLedFired(void);
static void transmitTimerFired(void);
static void initTimer(void);


//////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t adcData;
uint8_t output[] = "XXXXXXXXXXXX_CO2_MG81:6_EG6) \n\r";
uint8_t Bat_Output[]= "100_BAT_EG6) \n\r";

uint32_t CO2_LENGTH = sizeof(output)-1;
uint32_t BAT_LENGTH = sizeof(Bat_Output)-1;

uint32_t transmitCounter = 0;

//uint8_t output[] = "XXXXXXXXXXXX ppm \r\n";
uint8_t Volt[] = "XXXXXX,XXXXX Volt \r\n";

static HAL_AppTimer_t timer;
static void timerfunc(void);

static AppState_t appstate = APP_INIT;
static void readSensorDone(void);
static void calculation(void);

static HAL_AdcDescriptor_t adc={
	.resolution = RESOLUTION_10_BIT,
	.sampleRate = ADC_4800SPS,
	.voltageReference = AVCC,
	.bufferPointer = &adcData,
	.selectionsAmount = 1,
	.callback = readSensorDone
};

static void readSensorDone(){
	//appWriteDataToUsart((uint8_t*)"ich bin in der Callbackfunktion\n\r", sizeof("ich bin in der Callbackfunktion\n\r")-1);
	appstate = APP_AUSGABE;
	SYS_PostTask(APL_TASK_ID);
}


static void timerfunc(void){
	appstate = APP_TRANSMIT_Battery;
	SYS_PostTask(APL_TASK_ID);
	
}

static void calculation(void){
//	float zeropointvoltage = 4.8;
//	float reactionvoltage = -0.07537688442 ;
	float minvolt = 4.475; 
	float maxvolt = 1.6;
//	float dcgain = 8.5;
	float adcvoltage = 0;
	float adcvoltagezwei = 0;
	uint32_t vorkomma = 0;
	uint32_t nachkomma = 0;
	uint32_t ergebnis = 0;
	float buffer ;
	float deltaV ;
	
	adcvoltage = (float)2 * ((float)3.3* (float)adcData / (float)1024);
	
	adcvoltagezwei = adcvoltage;
	
	vorkomma = adcvoltage;	
	
	uint32_to_str(Volt, sizeof(Volt), vorkomma, 0,6);
	
	adcvoltage = adcvoltage - vorkomma;
	adcvoltage = adcvoltage * 100000;
	
	nachkomma = adcvoltage;
	uint32_to_str(Volt, sizeof(Volt), nachkomma, 7,5);
	appWriteDataToUsart(Volt, sizeof(Volt));



	deltaV = minvolt - maxvolt;
	
	buffer = deltaV / (-2);
	buffer = (adcvoltagezwei - minvolt) / buffer;
	buffer = buffer + 2.602059991;
	
	ergebnis = pow(10,buffer);
	
	uint32_to_str(output, sizeof(output), ergebnis, 0,12);
	appWriteDataToUsart(output, sizeof(output));
	
	appWriteDataToUsart((uint8_t*)"\n\r", sizeof("\n\r")-1);
	uint32_to_str(output, sizeof(output), adcData, 0,12);
}




// -----------------------------------------
//---------------------Netzwerk--------------------------------------

static void initTimer(void){

	transmitTimerLed.interval= 500;
	transmitTimerLed.mode	= TIMER_ONE_SHOT_MODE;
	transmitTimerLed.callback= transmitTimerLedFired;
	
	receiveTimerLed.interval= 500;
	receiveTimerLed.mode= TIMER_ONE_SHOT_MODE;
	receiveTimerLed.callback= receiveTimerLedFired;
	
	transmitTimer.interval= 3000;
	transmitTimer.mode	= TIMER_ONE_SHOT_MODE;
	transmitTimer.callback= transmitTimerFired;
	
	//MG811------------------------------------------------
	timer.interval = 1000;
	timer.mode	   = TIMER_ONE_SHOT_MODE;
	timer.callback = timerfunc;
}

static void transmitTimerLedFired(void){
	BSP_OffLed(LED_YELLOW);
}

static void receiveTimerLedFired(void){
	BSP_OffLed(LED_RED);
}

static void transmitTimerFired(void){
	appstate=APP_TRANSMIT_CO2;
	SYS_PostTask(APL_TASK_ID);
}

BEGIN_PACK
typedef struct _AppMessage_t{
	uint8_t header[APS_ASDU_OFFSET];
	uint8_t data[35];
	uint8_t footer[APS_AFFIX_LENGTH - APS_ASDU_OFFSET];
} PACK AppMessage_t;
END_PACK

static AppMessage_t transmitData;

APS_DataReq_t dataReq;
static void APS_DataConf(APS_DataConf_t *confInfo);
static void initTransmitData(void);

static void initTransmitData(void){
	dataReq.profileId=1;
	dataReq.dstAddrMode =APS_SHORT_ADDRESS;
	dataReq.dstAddress.shortAddress= CPU_TO_LE16(0);
	dataReq.dstEndpoint =1;
	dataReq.asdu  =transmitData.data;
	dataReq.asduLength=sizeof(transmitData.data);
	dataReq.srcEndpoint = 1;
	//dataReq.APS_DataConf=APS_DataConf;
}

static void APS_DataConf_Co(APS_DataConf_t *confInfo){
	if (confInfo->status==APS_SUCCESS_STATUS){
		BSP_OnLed(LED_YELLOW);
		//HAL_StartAppTimer(&transmitTimerLed);
		appstate=APP_READ;
		SYS_PostTask(APL_TASK_ID);
		} else{
		appWriteDataToUsart((uint8_t*)"CO_FEHLER\n\r", sizeof("CO_FEHLER\n\r")-1);
		appstate=APP_INIT;
	}
}


static void APS_DataConf_Bat(APS_DataConf_t *confInfo){
	if (confInfo->status==APS_SUCCESS_STATUS){
		BSP_OnLed(LED_YELLOW);
		//HAL_StartAppTimer(&transmitTimerLed);
		appstate=APP_TRANSMIT_CO2;
		SYS_PostTask(APL_TASK_ID);
		} else{
		appWriteDataToUsart((uint8_t*)"TEMP_FEHLER\n\r", sizeof("TEMP_FEHLER\n\r")-1);
		appstate=APP_INIT;
	}
}




static void initEndpoint(void){
	simpleDescriptor.AppDeviceId = 1;
	simpleDescriptor.AppProfileId = 1;
	simpleDescriptor.endpoint = 1;
	simpleDescriptor.AppDeviceVersion = 1;
	endPoint.simpleDescriptor= &simpleDescriptor;
	endPoint.APS_DataInd = APS_DataInd;
	APS_RegisterEndpointReq(&endPoint);
}

void APS_DataInd(APS_DataInd_t *indData){
	BSP_OnLed(LED_RED);
	HAL_StartAppTimer(&receiveTimerLed);
	appWriteDataToUsart(indData->asdu,indData->asduLength);
	appWriteDataToUsart((uint8_t*)"\r\n",2);
}

// Taskhandler ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void APL_TaskHandler(void){
	
	switch(appstate){
		case APP_INIT:
		appInitUsartManager();
		initTimer();
		HAL_OpenAdc(&adc);
		BSP_OpenLeds();
		appWriteDataToUsart((uint8_t*)"Ich wurde initialisiert\n\r", sizeof("ich wurde initialisiert\n\r")-1);
		appstate = APP_STARTJOIN_NETWORK;
		SYS_PostTask(APL_TASK_ID);
		break;
		
		
		case APP_READ:
		appWriteDataToUsart((uint8_t*)"Ich versuche den Bus zu lesen\n\r", sizeof("Ich versuche den Bus zu lesen\n\r")-1);
		HAL_ReadAdc(&adc, HAL_ADC_CHANNEL1);
		appWriteDataToUsart((uint8_t*)"Ich versuche den Bus zu lesen2\n\r", sizeof("Ich versuche den Bus zu lesen2\n\r")-1);
		break;
		
		case APP_AUSGABE:
		appWriteDataToUsart((uint8_t*)"Ich bin in der Ausagbe\n\r", sizeof("Ich bin in der Ausagbe\n\r")-1);
		calculation();
		HAL_StartAppTimer(&timer);
		break;
		
		case APP_NOTHING_STATE:
		appWriteDataToUsart((uint8_t*)"Irgendwas ist hier schief gelaufen \r\n", sizeof("Irgendwas ist hier schief gelaufen \n\r"));
		break;
		
		
		
		
		
		//------------------------NETZWERK------------------------------------
		case APP_STARTJOIN_NETWORK:
		appWriteDataToUsart((uint8_t*)"APP_STARTJOIN_NETWORK\n\r", sizeof("APP_STARTJOIN_NETWORK\n\r")-1);
		networkParams.ZDO_StartNetworkConf = ZDO_StartNetworkConf;
		ZDO_StartNetworkReq(&networkParams);
		appstate=APP_INIT_ENDPOINT;
		break;
		
		case APP_INIT_ENDPOINT:
		appWriteDataToUsart((uint8_t*)"APP_INIT_ENDPOINT\n\r", sizeof("APP_INIT_ENDPOINT\n\r")-1);
		initEndpoint();
		#if CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR
		appstate=APP_NOTHING;
		#else
		appstate=APP_INIT_TRANSMITDATA;
		#endif
		SYS_PostTask(APL_TASK_ID);
		break;
		
		case APP_INIT_TRANSMITDATA:
		appWriteDataToUsart((uint8_t*)"APP_INIT_TRANSMITDATA\n\r", sizeof("APP_INIT_TRANSMITDATA\n\r")-1);
		initTransmitData();
		appstate=APP_READ;
		HAL_StartAppTimer(&transmitTimer);
		SYS_PostTask(APL_TASK_ID);
		break;
		
		
		//uint8_t output[] = "XXXXXXXXXXXX_CO2_MG8_EG6)";
		case APP_TRANSMIT_CO2:
		#if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
		for(transmitCounter = 0; transmitCounter < CO2_LENGTH; transmitCounter++) {
			transmitData.data[transmitCounter] = output[transmitCounter];
		}
		appWriteDataToUsart((uint8_t*)"APP_TRANSMIT_CO2\n\r", sizeof("APP_TRANSMIT_CO2\n\r")-1);
		#endif
		dataReq.APS_DataConf=APS_DataConf_Co;
		APS_DataReq(&dataReq);
		break;
		
		
		//uint8_t Bat_Output[]= "100_BAT_EG6)";
		case APP_TRANSMIT_Battery:
		#if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
		for(transmitCounter = 0; transmitCounter < BAT_LENGTH; transmitCounter++) {
			transmitData.data[transmitCounter] = Bat_Output[transmitCounter];
		}
		#endif
		dataReq.APS_DataConf=APS_DataConf_Bat;
		APS_DataReq(&dataReq);
		break;
		
		
	}
}



void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo){
	if (ZDO_SUCCESS_STATUS == confirmInfo->status) {
		CS_ReadParameter(CS_DEVICE_TYPE_ID, &deviceType);
		if (deviceType==DEV_TYPE_COORDINATOR) {
			appWriteDataToUsart((uint8_t*)"Coordinator\r\n", sizeof("Coordinator\r\n")-1);
		}
		BSP_OnLed(LED_YELLOW);
		} else {
		appWriteDataToUsart((uint8_t*)"Error\r\n",sizeof("Error\r\n")-1);
	}
	SYS_PostTask(APL_TASK_ID);
}




/*******************************************************************************
  \brief The function is called by the stack to notify the application about 
  various network-related events. See detailed description in API Reference.
  
  Mandatory function: must be present in any application.

  \param[in] nwkParams - contains notification type and additional data varying
             an event
  \return none
*******************************************************************************/
void ZDO_MgmtNwkUpdateNotf(ZDO_MgmtNwkUpdateNotf_t *nwkParams)
{
  nwkParams = nwkParams;  // Unused parameter warning prevention
}

/*******************************************************************************
  \brief The function is called by the stack when the node wakes up by timer.
  
  When the device starts after hardware reset the stack posts an application
  task (via SYS_PostTask()) once, giving control to the application, while
  upon wake up the stack only calls this indication function. So, to provide 
  control to the application on wake up, change the application state and post
  an application task via SYS_PostTask(APL_TASK_ID) from this function.

  Mandatory function: must be present in any application.
  
  \return none
*******************************************************************************/
void ZDO_WakeUpInd(void)
{
}

#ifdef _BINDING_
/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.
  
  Mandatory function: must be present in any application.

  \param[in] bindInd - information about the bound device
  \return none
 ***********************************************************************************/
void ZDO_BindIndication(ZDO_BindInd_t *bindInd)
{
  (void)bindInd;
}

/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.

  Mandatory function: must be present in any application.
  
  \param[in] unbindInd - information about the unbound device
  \return none
 ***********************************************************************************/
void ZDO_UnbindIndication(ZDO_UnbindInd_t *unbindInd)
{
  (void)unbindInd;
}
#endif //_BINDING_

/**********************************************************************//**
  \brief The entry point of the program. This function should not be
  changed by the user without necessity and must always include an
  invocation of the SYS_SysInit() function and an infinite loop with
  SYS_RunTask() function called on each step.

  \return none
**************************************************************************/
int main(void)
{
  //Initialization of the System Environment
  SYS_SysInit();

  //The infinite loop maintaing task management
  for(;;)
  {
    //Each time this function is called, the task
    //scheduler processes the next task posted by one
    //of the BitCloud components or the application
    SYS_RunTask();
  }
}

//eof app.c
