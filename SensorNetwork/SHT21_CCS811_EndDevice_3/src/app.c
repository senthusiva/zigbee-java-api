/**************************************************************************//**
  \file app.c

  \brief Basis-Anwendung.

  \author Markus Krauße

******************************************************************************/


#include <appTimer.h>
#include <zdo.h>
#include <app.h>
#include <sysTaskManager.h>
#include <usartManager.h>
#include <i2cpacket.h>
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


//CCS811 property ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t css811Data[4];
uint8_t resultregister[] = {0x02};
//uint8_t co2_Output[]= "xxxxx ppm(eCO2)";
//uint8_t voc_Output[]= "xxxxx ppb(eCO2)";

uint8_t co2_Output[]= "xxxxx_CO2_CCS1:3_EG3) \n\r";
uint8_t voc_Output[]= "xxxxx_VOC_CCS1:3_EG3) \n\r";

uint8_t temp_output[]="xxx.xx_DEG_SHT1:3_EG3) \n\r";
uint8_t hmd_output[]= "xx.xx_HMD_SHT1:3_EG3) \n\r";

uint8_t Bat_Output[]= "xxx_BAT_EG3) \n\r";

uint32_t CO2_LENGTH = sizeof(co2_Output)-1;
uint32_t VOC_LENGTH = sizeof(voc_Output)-1;
uint32_t TEM_LENGTH = sizeof(temp_output)-1;
uint32_t HMD_LENGTH = sizeof(hmd_output)-1;
uint32_t BAT_LENGTH = sizeof(Bat_Output)-1;

uint32_t transmitCounter = 0;


uint8_t measuremode[]= {0x01, 0x10};
uint8_t appstart[]= {0xF4};
uint8_t resetdata[]= {0xFF, 0x11,0xE5,0x72,0x8A};


static AppState_t appstate = APP_INIT;
static void setMeasureMode(bool result);
static void readSensorDoneCo2(bool result);
static void writeAppStart(bool result);
static void writeReset(bool result);
static void writeToResultRegister(bool result);
static void output(void);

static HAL_AppTimer_t transitiontimer;
static void transitiontimerfnc(void);

static HAL_AppTimer_t measuredelay;
static void measuredelayfnc(void);

static HAL_AppTimer_t appstartdelay;
static void appstartdelayfnc(void);

static HAL_AppTimer_t resetdelay;
static void resetdelayfnc(void);


static HAL_I2cDescriptor_t i2cccs={
	.tty = TWI_CHANNEL_0,
	.clockRate = I2C_CLOCK_RATE_62,
	.id = 0x5A,
	.lengthAddr = HAL_NO_INTERNAL_ADDRESS
};


static void setMeasureMode(bool result){
	if(result == true){
		appWriteDataToUsart((uint8_t*)"Zustand erfolgreich auf MeasureMode gesetzt\n\r", sizeof("Zustand erfolgreich auf MeasureMode gesetzt\n\r")-1);
		
		if (-1 == HAL_CloseI2cPacket(&i2cccs)){
		appWriteDataToUsart((uint8_t*)"Close fail\n\r", sizeof("Close fail\n\r")-1);}
		
		HAL_StartAppTimer(&transitiontimer);
	}
	else{
		appstate = APP_NOTHING;
		appWriteDataToUsart((uint8_t*)"Zustand nicht erfolgreich auf MeasureMode gesetzt\n\r", sizeof("Zustand nicht erfolgreich auf MeasureMode gesetzt\n\r")-1);
	}
	
}

static void readSensorDoneCo2(bool result){
	if(result == true){
//		appWriteDataToUsart((uint8_t*)"Werte koennen erfolgreich abgelesen werden\n\r", sizeof("Werte koennen erfolgreich abgelesen werden\n\r")-1);
		
		if (-1 == HAL_CloseI2cPacket(&i2cccs)){
		appWriteDataToUsart((uint8_t*)"Close fail\n\r", sizeof("Close fail\n\r")-1);}
		
		HAL_StartAppTimer(&measuredelay);
		
	}
	else{
		appstate = APP_NOTHING;
		appWriteDataToUsart((uint8_t*)"Werte können NICHT erfolgreich abgelesen werden\n\r", sizeof("Werte können NICHT erfolgreich abgelesen werden\n\r")-1);
	}
	
}

static void writeAppStart (bool result){
	
	if(result == true){
		
	//	appWriteDataToUsart((uint8_t*)"Zustand ist erfolgreich auf APP_Start gesetzt\n\r", sizeof("Zustand ist erfolgreich auf APP_Start gesetzt\n\r")-1);
		
		if (-1 == HAL_CloseI2cPacket(&i2cccs)){
		appWriteDataToUsart((uint8_t*)"Close fail\n\r", sizeof("Close fail\n\r")-1);}
		
		HAL_StartAppTimer(&appstartdelay);
	}
	else{
		appstate = APP_NOTHING;
		appWriteDataToUsart((uint8_t*)"Zustand ist nich erfolgreich auf APP_Start gesetzt\n\r", sizeof("Zustand ist nich erfolgreich auf APP_Start gesetzt\n\r")-1);
	}
	
}

static void writeReset(bool result){
	if(result == true){
		
	//	appWriteDataToUsart((uint8_t*)"Zustand wurde erfolgreich resettet\n\r", sizeof("Zustand wurde erfolgreich resettet\n\r")-1);
		
		if (-1 == HAL_CloseI2cPacket(&i2cccs)){
		appWriteDataToUsart((uint8_t*)"Close fail\n\r", sizeof("Close fail\n\r")-1);}
		
		HAL_StartAppTimer(&resetdelay);
	}
	else{
		appstate = APP_NOTHING;
		appWriteDataToUsart((uint8_t*)"Zustand wurde nicht erfolgreich resettet\n\r", sizeof("Zustand wurde nicht erfolgreich resettet\n\r")-1);
	}
}

static void writeToResultRegister(bool result){
	if(result == true){
	//	appWriteDataToUsart((uint8_t*)"Writeregister erfolgreich angepeilt\n\r", sizeof("Writeregister erfolgreich angepeilt\n\r")-1);
		if (-1 == HAL_CloseI2cPacket(&i2cccs)){
		appWriteDataToUsart((uint8_t*)"Close fail\n\r", sizeof("Close fail\n\r")-1);}
		
		appstate = APP_READ_CO2;
		SYS_PostTask(APL_TASK_ID);
	}
	
	else{
		appWriteDataToUsart((uint8_t*)"Writeregister nicht erfolgreich angepeilt\n\r", sizeof("Writeregister nicht erfolgreich angepeilt\n\r")-1);
		
	}
	
}

static void output(void){
	uint32_t readCo2 = 00000000000000000000000000000000;
	uint32_t readVoc = 00000000000000000000000000000000;
	
	readCo2 |= css811Data[0];
	readCo2 <<= 8;
	readCo2 |= css811Data[1];
	
	readVoc = css811Data[2];
	readVoc <<= 8;
	readVoc = css811Data[3];
	
	uint32_to_str(co2_Output,sizeof(co2_Output), readCo2,0,5);
	uint32_to_str(voc_Output,sizeof(voc_Output), readVoc,0,5);
	
	appWriteDataToUsart(co2_Output, sizeof(co2_Output));
	appWriteDataToUsart((uint8_t*)"\n\r",sizeof("\n\r")-1);
	
	appWriteDataToUsart(voc_Output, sizeof(voc_Output));
	appWriteDataToUsart((uint8_t*)"\n\r",sizeof("\n\r")-1);
}



static void measuredelayfnc(void){
	appstate = APP_OUTPUT_DATA;
	SYS_PostTask(APL_TASK_ID);
}

static void appstartdelayfnc(void){
//	appWriteDataToUsart((uint8_t*)"ich bin im appstartTimer\n\r", sizeof("ich bin im appstartTimer\n\r")-1);
	appstate = APP_SET_DRIVE_MODE;
	SYS_PostTask(APL_TASK_ID);
}

static void resetdelayfnc(void){
//	appWriteDataToUsart((uint8_t*)"ich bin im resetTimer\n\r", sizeof("ich bin im resettimer\n\r")-1);
	appstate = APP_APP_START;
	SYS_PostTask(APL_TASK_ID);
	
}

static void transitiontimerfnc(void){
//	appWriteDataToUsart((uint8_t*)"ich bin im transitionTimer\n\r", sizeof("ich bin im transitionTimer\n\r")-1);
	appstate=APP_STARTJOIN_NETWORK;
	//appstate = APP_CALL_TEMP_SENSOR;
	SYS_PostTask(APL_TASK_ID);
}

//sht21 property //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t testoutput[]=   "XXXXXXXXXXX\n\r";
uint8_t testoutput2[]=   "XXXXXXXXXXX\n\r";
//uint8_t temp_output[]="xxx.xx degree celcius\n\r";
//uint8_t hmd_output[]= "xx.xx% Humidity";

//uint8_t temp_output[]="xxx.xx_DEG_SHT_EG1)";
//uint8_t hmd_output[]= "xx.xx_HMD_SHT_EG1)";
uint8_t sht21Data[3];
uint8_t tempData[1] = {0xF3};
uint8_t hmdData[1] = {0xF5};

//tmp timer
static HAL_AppTimer_t measuredelaytmp;
static void measuredelaytmpfnc(void);
//hmd timer
static HAL_AppTimer_t measuredelayhmd;
static void measuredelayhmdfnc(void);
static void initTimer(void);


static void readSensorDoneTMP(bool result);
static void writeSensorDoneTMP(bool result2);
static void readSensorDoneHMD(bool result3);
static void writeSensorDoneHMD(bool result4);
static void calculateTemp(void);
static void calculateHum(void);

static HAL_I2cDescriptor_t i2csht={
	.tty = TWI_CHANNEL_0,
	.clockRate = I2C_CLOCK_RATE_62,
	.id = 0x40,
	.lengthAddr = HAL_NO_INTERNAL_ADDRESS
};

static void writeSensorDoneTMP(bool result2){
	if(result2 == true){
			appWriteDataToUsart((uint8_t*)"Sensor hat signal zur Tempmessung bekommen\n\r", sizeof("Sensor hat signal zur Tempmessung bekommen\n\r")-1);
		HAL_StartAppTimer(&measuredelaytmp);
		
	}
	else{
			appWriteDataToUsart((uint8_t*)"Sensor hat signal zur Tempmessung nicht bekommen\n\r", sizeof("Sensor hat signal zur Tempmessung nicht bekommen\n\r")-1);
		appstate = APP_NOTHING;
	}
}

static void writeSensorDoneHMD(bool result4){
	if(result4 == true){
			appWriteDataToUsart((uint8_t*)"HMDSensor hat signal zur Tempmessung bekommen\n\r", sizeof("HMDSensor hat signal zur Tempmessung bekommen\n\r")-1);
		HAL_StartAppTimer(&measuredelayhmd);
	}
	else{
		appstate = APP_NOTHING;
			appWriteDataToUsart((uint8_t*)"HMDSensor hat signal zur Tempmessung nicht bekommen\n\r", sizeof("HMDSensor hat signal zur Tempmessung nicht bekommen\n\r")-1);
	}
}

static void readSensorDoneTMP(bool result){
	if(result == true){
			appWriteDataToUsart((uint8_t*)"Sensorwert kann erfolgreich abgelesen werden\n\r", sizeof("Sensorwert kann erfolgreich abgelesen werden\n\r")-1);
		appstate = APP_OUTPUT_TEMP_SENSOR;
		SYS_PostTask(APL_TASK_ID);
	}
	
	else{
			appWriteDataToUsart((uint8_t*)"Sensorwert kann nicht erfolgreich abgelesen werden\n\r", sizeof("Sensorwert kann nicht erfolgreich abgelesen werden\n\r")-1);
		appstate = APP_NOTHING;
	}
}

static void readSensorDoneHMD(bool result3){
	if(result3 == true){
			appWriteDataToUsart((uint8_t*)"HMDSensorwert kann erfolgreich abgelesen werden\n\r", sizeof("HMDSensorwert kann erfolgreich abgelesen werden\n\r")-1);
		appstate = APP_OUTPUT_HMD_SENSOR;
		SYS_PostTask(APL_TASK_ID);
	}
	
	else{
			appWriteDataToUsart((uint8_t*)"HMDSensorwert kann nicht erfolgreich abgelesen werden\n\r", sizeof("HMDSensorwert kann nicht erfolgreich abgelesen werden\n\r")-1);
		appstate = APP_NOTHING;
	}
}

static void calculateTemp(void){
	uint32_t rawData = 00000000000000000000000000000000;
	float endwert;
	uint8_t remover  = 00000000;
	int32_t vorkomma= 00000000000000000000000000000000;
	uint32_t nachkomma= 00000000000000000000000000000000;
	
	remover = sht21Data[1] & 0xfc;
	
	rawData |= sht21Data[0];
	rawData <<= 8;
	rawData |= remover;

	endwert = ((float)rawData / 65536) * 175.72 - 46.85;
	vorkomma = endwert;
	int32_to_str(temp_output, sizeof(temp_output), vorkomma, 0, 3);
	
	endwert = endwert - vorkomma;
	endwert = endwert *100;
	nachkomma = endwert;
	uint32_to_str(temp_output, sizeof(temp_output), nachkomma, 4, 2);
	

	appWriteDataToUsart(temp_output, sizeof(temp_output));
	appWriteDataToUsart((uint8_t*)"\n\r",sizeof("\n\r")-1);

}

static void calculateHum(void){
	uint32_t rawData = 00000000000000000000000000000000;
	float endwert;
	uint8_t remover;
	uint32_t vorkomma, nachkomma;
	
	remover = sht21Data[1] & 0xfc;
	
	rawData |= sht21Data[0];
	rawData <<= 8;
	rawData |= remover;
	
	endwert = ((float) rawData / 65536) * 125 -6;
	vorkomma = endwert;
	uint32_to_str(hmd_output, sizeof(temp_output), vorkomma, 0, 2);
	
	endwert = endwert - vorkomma;
	endwert = endwert *100;
	nachkomma = endwert;
	uint32_to_str(hmd_output, sizeof(temp_output), nachkomma, 3, 2);
	
	appWriteDataToUsart(hmd_output, sizeof(hmd_output));
	appWriteDataToUsart((uint8_t*)"\n\r",sizeof("\n\r")-1);
}


static void measuredelaytmpfnc(void){
	appWriteDataToUsart((uint8_t*)"ich bin im Timer\n\r", sizeof("ich bin im timer\n\r")-1);
	appstate = APP_READ_TEMP_SENSOR;
	SYS_PostTask(APL_TASK_ID);
}
static void measuredelayhmdfnc(void){
		appWriteDataToUsart((uint8_t*)"ich bin im HMDTimer\n\r", sizeof("ich bin im timer\n\r")-1);
	appstate = APP_READ_HMD_SENSOR;
	SYS_PostTask(APL_TASK_ID);
}



// -----------------------------------------
//---------------------NETZWERK--------------------------------------
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
	
	//CCS------------------------------------------------
		measuredelay.interval = 1000;
		measuredelay.mode	 = TIMER_ONE_SHOT_MODE;
		measuredelay.callback = measuredelayfnc;

		appstartdelay.interval = 500;
		appstartdelay.mode	   = TIMER_ONE_SHOT_MODE;
		appstartdelay.callback = appstartdelayfnc;

		resetdelay.interval = 100;
		resetdelay.mode		= TIMER_ONE_SHOT_MODE;
		resetdelay.callback = resetdelayfnc;

		transitiontimer.interval = 1000;
		transitiontimer.mode	 = TIMER_ONE_SHOT_MODE;
		transitiontimer.callback = transitiontimerfnc;
	//CCS------------------------------------------------
	
	//SHT------------------------------------------------
		measuredelaytmp.interval = 3000;
		measuredelaytmp.mode	 = TIMER_ONE_SHOT_MODE;
		measuredelaytmp.callback = measuredelaytmpfnc;
		
		measuredelayhmd.interval = 3000;
		measuredelayhmd.mode	 = TIMER_ONE_SHOT_MODE;
		measuredelayhmd.callback = measuredelayhmdfnc;
	//SHT------------------------------------------------

}

// -----------------------------------------
//---------------------Batterie--------------------------------------

//uint8_t Bat_Output[]= "xxx_BAT_EG1)";
uint32_t adcData;
uint8_t Test_Output[] = "xxxx";
uint8_t adc_wert[] = "xxx";
static void calculationBattery(void);
static void readSensorDone(void);

static HAL_AdcDescriptor_t adc={
	.resolution = RESOLUTION_10_BIT,
	.sampleRate = ADC_4800SPS,
	.voltageReference = INTERNAL_1d1V,
	.bufferPointer = &adcData,
	.selectionsAmount = 1,
	.callback = readSensorDone
};


static void readSensorDone(){
	//appWriteDataToUsart((uint8_t*)"ich bin in der Callbackfunktion\n\r", sizeof("ich bin in der Callbackfunktion\n\r")-1);
	appstate = APP_Calculate_Battery;
	SYS_PostTask(APL_TASK_ID);
}

static void calculationBattery(void){
	uint32_t ergebnis = 0;
	float  nennspannung = 0;
	
	nennspannung = 4*((float)adcData * (float)1.5 / (float)1024);

	if (nennspannung < 2.6){
		ergebnis = 0;
	}
	else if (nennspannung < 2.8 && nennspannung >= 2.6){
		ergebnis = 1;
	}
	else if (nennspannung < 3.1 && nennspannung >= 2.8){
		ergebnis = 10;
	}
	else if (nennspannung >=3.1){
		ergebnis = 100;
	}

	uint32_to_str(Bat_Output, sizeof(Bat_Output), ergebnis, 0,3);
	appWriteDataToUsart(Bat_Output, sizeof(Bat_Output));
	appWriteDataToUsart((uint8_t*)"\n\r", sizeof("\n\r")-1);
	appWriteDataToUsart((uint8_t*)"\n\r", sizeof("\n\r")-1);
	appstate = APP_TRANSMIT_Battery;
	SYS_PostTask(APL_TASK_ID);
}

static void transmitTimerLedFired(void){
	BSP_OffLed(LED_YELLOW);
}

static void receiveTimerLedFired(void){
	BSP_OffLed(LED_RED);
}

static void transmitTimerFired(void){
	appstate=APP_TRANSMIT_TEMP;
	SYS_PostTask(APL_TASK_ID);
}

BEGIN_PACK
typedef struct _AppMessage_t{
	uint8_t header[APS_ASDU_OFFSET];
	uint8_t data[30];
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

static void APS_DataConf_Bat(APS_DataConf_t *confInfo){
	if (confInfo->status==APS_SUCCESS_STATUS){
		BSP_OnLed(LED_YELLOW);
		//HAL_StartAppTimer(&transmitTimerLed);
		appstate=APP_TRANSMIT_TEMP;
		SYS_PostTask(APL_TASK_ID);
		} else{
		appWriteDataToUsart((uint8_t*)"TEMP_FEHLER\n\r", sizeof("TEMP_FEHLER\n\r")-1);
		appstate=APP_INIT;
	}
}

static void APS_DataConf_Temp(APS_DataConf_t *confInfo){
	if (confInfo->status==APS_SUCCESS_STATUS){
		BSP_OnLed(LED_YELLOW);
		//HAL_StartAppTimer(&transmitTimerLed);
		appstate=APP_TRANSMIT_HMD;
		SYS_PostTask(APL_TASK_ID);
	} else{
		appWriteDataToUsart((uint8_t*)"TEMP_FEHLER\n\r", sizeof("TEMP_FEHLER\n\r")-1);
		appstate=APP_INIT;
	}
}

static void APS_DataConf_Hmd(APS_DataConf_t *confInfo){
	if (confInfo->status==APS_SUCCESS_STATUS){
		BSP_OnLed(LED_YELLOW);
		//HAL_StartAppTimer(&transmitTimerLed);
		appstate=APP_TRANSMIT_CO2;
		SYS_PostTask(APL_TASK_ID);
	} else{
		appWriteDataToUsart((uint8_t*)"HMD_FEHLER\n\r", sizeof("HMD_FEHLER\n\r")-1);
		appstate=APP_INIT;
	}
}

static void APS_DataConf_Co(APS_DataConf_t *confInfo){
	if (confInfo->status==APS_SUCCESS_STATUS){
		BSP_OnLed(LED_YELLOW);
		//HAL_StartAppTimer(&transmitTimerLed);
		appstate=APP_TRANSMIT_VOC;
		SYS_PostTask(APL_TASK_ID);
	}
	 else{
		 appWriteDataToUsart((uint8_t*)"CO_FEHLER\n\r", sizeof("CO_FEHLER\n\r")-1);
		 appstate=APP_INIT;
	 }
}

static void APS_DataConf_Voc(APS_DataConf_t *confInfo){
	if (confInfo->status==APS_SUCCESS_STATUS){
		BSP_OnLed(LED_YELLOW);
		//HAL_StartAppTimer(&transmitTimerLed);
		appstate=APP_CALL_TEMP_SENSOR;
		SYS_PostTask(APL_TASK_ID);
	} else{
		appWriteDataToUsart((uint8_t*)"VOC_FEHLER\n\r", sizeof("VOC_FEHLER\n\r")-1);
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
uint8_t m;
void APL_TaskHandler(void){
	switch(appstate){
		case APP_INIT:
			appInitUsartManager();
			initTimer();
			HAL_OpenAdc(&adc);
			BSP_OpenLeds();
			appstate = APP_RESET;
			SYS_PostTask(APL_TASK_ID);
		break;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
		case APP_RESET  :
			if (-1 == HAL_OpenI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"open1 fail\n\r", sizeof("open1 fail\n\r")-1);}
		
			appWriteDataToUsart((uint8_t*)"APP_RESET\n\r", sizeof("APP_RESET\n\r")-1);
			i2cccs.f		= writeReset;
			i2cccs.data	= resetdata;
			i2cccs.length	= 5;
		
			if (-1 == HAL_WriteI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"write to reset fail\n\r", sizeof("write to reset fail\n\r")-1);}
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_APP_START :
			if (-1 == HAL_OpenI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"open1 fail\n\r", sizeof("open1 fail\n\r")-1);}
		
			appWriteDataToUsart((uint8_t*)"APP_APP_START\n\r", sizeof("APP_APP_START\n\r")-1);
			i2cccs.f	= writeAppStart;
			i2cccs.data = appstart;
			i2cccs.length = 1;
		
			if (-1 == HAL_WriteI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"Write to app_start fail\n\r", sizeof("write to app_start fail\n\r")-1);}
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_SET_DRIVE_MODE  :
			if (-1 == HAL_OpenI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"open2 fail\n\r", sizeof("open2 fail\n\r")-1);}
		
			appWriteDataToUsart((uint8_t*)"APP_SET_DRIVE_MODE\n\r", sizeof("APP_SET_DRIVE_MODE\n\r")-1);
			i2cccs.f		= setMeasureMode;
			i2cccs.data	= measuremode;
			i2cccs.length  = 2;
		
			if (-1 == HAL_WriteI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"Write fail\n\r", sizeof("write fail\n\r")-1);}
			appWriteDataToUsart((uint8_t*)"Ich gebe weiter zum Timer\n\r", sizeof("Ich gebe weiter zum Timer\n\r")-1);
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_CALL_TEMP_SENSOR  :
			if (-1 == HAL_OpenI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"opentmp fail\n\r", sizeof("opentmp fail\n\r")-1);}
			
			appWriteDataToUsart((uint8_t*)"APP_CALL_TEMP_SENSOR\n\r", sizeof("APP_CALL_TEMP_SENSOR\n\r")-1);
			i2csht.f = writeSensorDoneTMP;
			i2csht.data = tempData;
			i2csht.length = 1;
			
			if (-1 == HAL_WriteI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"Writetmp fail\n\r", sizeof("writetmp fail\n\r")-1);}
			appstate = APP_NOTHING;
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_READ_TEMP_SENSOR  :
		
			appWriteDataToUsart((uint8_t*)"APP_READ_TEMP_SENSOR\n\r", sizeof("APP_READ_TEMP_SENSOR\n\r")-1);
			i2csht.f = readSensorDoneTMP;
			i2csht.data = sht21Data;
			i2csht.length = 3;
			
			if (-1 == HAL_ReadI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"Readtmp fail\n\r", sizeof("readtmp fail\n\r")-1);}
			appstate=APP_NOTHING;
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_OUTPUT_TEMP_SENSOR  :
			if(-1 == HAL_CloseI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"Closetmp fail\n\r", sizeof("Closetmp fail\n\r")-1);}
			//appWriteDataToUsart((uint8_t*)"ich bin im Outputstate\n\r", sizeof("ich bin im Outputstate\n\r")-1);
			appWriteDataToUsart((uint8_t*)"APP_OUTPUT_TEMP_SENSOR\n\r", sizeof("APP_OUTPUT_TEMP_SENSOR\n\r")-1);
			calculateTemp();
			appstate = APP_CALL_HMD_SENSOR;
			SYS_PostTask(APL_TASK_ID);
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_CALL_HMD_SENSOR  :
			if (-1 == HAL_OpenI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"openhmd fail\n\r", sizeof("openhmd fail\n\r")-1);}
			
			//appWriteDataToUsart((uint8_t*)"Ich versuche in den HMDBus zu schreiben\n\r", sizeof("Ich versuche in den HMDBus zu schreiben\n\r")-1);
			appWriteDataToUsart((uint8_t*)"APP_CALL_HMD_SENSOR\n\r", sizeof("APP_CALL_HMD_SENSOR\n\r")-1);
			i2csht.f = writeSensorDoneHMD;
			i2csht.data = hmdData;
			i2csht.length = 1;
			if (-1 == HAL_WriteI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"Writehmd fail\n\r", sizeof("writehmd fail\n\r")-1);}
			//appWriteDataToUsart((uint8_t*)"Ich gebe weiter zum HMDTimer\n\r", sizeof("Ich gebe weiter zum HMDTimer\n\r")-1);
			appstate = APP_NOTHING;
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_READ_HMD_SENSOR  :
		
			appWriteDataToUsart((uint8_t*)"APP_READ_HMD_SENSOR\n\r", sizeof("APP_READ_HMD_SENSOR\n\r")-1);
			i2csht.f = readSensorDoneHMD;
			i2csht.data = sht21Data;
			i2csht.length = 3;
			if (-1 == HAL_ReadI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"Readhmd fail\n\r", sizeof("readhmd fail\n\r")-1);}
			appstate=APP_NOTHING;
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_OUTPUT_HMD_SENSOR  :
			if(-1 == HAL_CloseI2cPacket(&i2csht)){
			appWriteDataToUsart((uint8_t*)"Closehmd fail\n\r", sizeof("Closehmd fail\n\r")-1);}
			//appWriteDataToUsart((uint8_t*)"ich bin im HMDOutputstate\n\r", sizeof("ich bin im HMDOutputstate\n\r")-1);
			appWriteDataToUsart((uint8_t*)"APP_OUTPUT_HMD_SENSOR\n\r", sizeof("APP_OUTPUT_HMD_SENSOR\n\r")-1);
			calculateHum();
			appstate = APP_CALL_RESULT_REGISTER;
			SYS_PostTask(APL_TASK_ID);
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_CALL_RESULT_REGISTER :
			if (-1 == HAL_OpenI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"open3 fail\n\r", sizeof("open3 fail\n\r")-1);}
		
			appWriteDataToUsart((uint8_t*)"APP_CALL_RESULT_REGISTER\n\r", sizeof("APP_CALL_RESULT_REGISTER\n\r")-1);
			i2cccs.f= writeToResultRegister;
			i2cccs.data = resultregister;
			i2cccs.length = 1 ;
		
			if (-1 == HAL_WriteI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"Write to result register failed\n\r", sizeof("Write to result register failed\n\r")-1);}
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_READ_CO2  :
			if (-1 == HAL_OpenI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"open4 fail\n\r", sizeof("open4 fail\n\r")-1);}
		
			appWriteDataToUsart((uint8_t*)"APP_READ_CO2\n\r", sizeof("APP_READ_CO2\n\r")-1);
			i2cccs.f = readSensorDoneCo2;
			i2cccs.data = css811Data ;
			i2cccs.length = 4;
		
		
			if (-1 == HAL_ReadI2cPacket(&i2cccs)){
			appWriteDataToUsart((uint8_t*)"Read fail\n\r", sizeof("read fail\n\r")-1);}
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////			
		case APP_OUTPUT_DATA  :
			appWriteDataToUsart((uint8_t*)"APP_OUTPUT_DATA\n\r", sizeof("APP_OUTPUT_DATA\n\r")-1);
			output();
		
			//appstate = APP_CALL_TEMP_SENSOR;
			appstate = APP_Batterylevel;
			SYS_PostTask(APL_TASK_ID);
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case APP_Batterylevel:
		HAL_ReadAdc(&adc, HAL_ADC_CHANNEL0);
		appstate = APP_Calculate_Battery;
		SYS_PostTask(APL_TASK_ID);
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case APP_Calculate_Battery:
		calculationBattery();
		break;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		case APP_NOTHING:
			appWriteDataToUsart((uint8_t*)"APP_NOTHING\n\r", sizeof("APP_NOTHING\n\r")-1);
			//appWriteDataToUsart((uint8_t*)"Feierabend\n\r", sizeof("Feierabend\n\r")-1);
			//appstate=APP_RESET;
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
			appstate=APP_CALL_TEMP_SENSOR;
			HAL_StartAppTimer(&transmitTimer);
			SYS_PostTask(APL_TASK_ID);
			break;
		

		case APP_TRANSMIT_Battery:
		#if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
			for(transmitCounter = 0; transmitCounter < BAT_LENGTH; transmitCounter++) {
				transmitData.data[transmitCounter] = Bat_Output[transmitCounter];
			}
		#endif
		dataReq.APS_DataConf=APS_DataConf_Bat;
		APS_DataReq(&dataReq);
		break;
		
		
		case APP_TRANSMIT_TEMP:
		#if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
			for(transmitCounter = 0; transmitCounter < TEM_LENGTH; transmitCounter++) {
				transmitData.data[transmitCounter] = temp_output[transmitCounter];
			}
			appWriteDataToUsart((uint8_t*)"APP_TRANSMIT_TEMP\n\r", sizeof("APP_TRANSMIT_TEMP\n\r")-1);
		#endif
		dataReq.APS_DataConf=APS_DataConf_Temp;
		APS_DataReq(&dataReq);
		break;
		
		case APP_TRANSMIT_HMD:
		#if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
			for(transmitCounter = 0; transmitCounter < HMD_LENGTH; transmitCounter++) {
				transmitData.data[transmitCounter] = hmd_output[transmitCounter];
			}
			appWriteDataToUsart((uint8_t*)"APP_TRANSMIT_HMD\n\r", sizeof("APP_TRANSMIT_HMD\n\r")-1);
		#endif
		dataReq.APS_DataConf=APS_DataConf_Hmd;
		APS_DataReq(&dataReq);
		break;
		
		case APP_TRANSMIT_CO2:
		#if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
			for(transmitCounter = 0; transmitCounter < CO2_LENGTH; transmitCounter++) {
				transmitData.data[transmitCounter] = co2_Output[transmitCounter];
			}
			appWriteDataToUsart((uint8_t*)"APP_TRANSMIT_CO2\n\r", sizeof("APP_TRANSMIT_CO2\n\r")-1);
			
		#endif
		dataReq.APS_DataConf=APS_DataConf_Co;
		APS_DataReq(&dataReq);
		break;
		
		case APP_TRANSMIT_VOC:
		appWriteDataToUsart((uint8_t*)"APP_TRANSMIT_VOC\n\r", sizeof("APP_TRANSMIT_VOC\n\r")-1);
		#if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
			for(transmitCounter = 0; transmitCounter < VOC_LENGTH; transmitCounter++) {
				transmitData.data[transmitCounter] = voc_Output[transmitCounter];
			}
			appWriteDataToUsart((uint8_t*)"APP_TRANSMIT_VOC\n\r", sizeof("APP_TRANSMIT_VOC\n\r")-1);
		#endif
		dataReq.APS_DataConf=APS_DataConf_Voc;
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
