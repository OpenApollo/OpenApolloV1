/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_UART4_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

//This is supposed to transmit a custom error message on call but I lowkey don't think it works
volatile const char *error_msg;
void errorInterrupt(const char *msg){
	error_msg = msg;
	printf("ERROR: %s\r\n", error_msg);
	HAL_UART_Transmit(&huart2, (uint8_t*)error_msg, strlen((char*)error_msg), HAL_MAX_DELAY);
}

//////////////////////////////////////////MS561101BA03-50////////////////////////////////////////////

void baroReset(){
	uint8_t resetData = 0x1E;
	for(int i = 0; i < 2; i++){
	HAL_I2C_Master_Transmit(&hi2c1,0x77 << 1,&resetData,1,300);
	HAL_Delay(400);
	}
}

uint8_t dataPROM[2];
uint16_t C1, C2, C3, C4, C5, C6; //PROM data *constants* for math
void baroPROM(){
	uint8_t requestPROM = 0xA0;
	for(int i = 1; i <= 7; i++){ //Maybe i needs to start at 1?
	HAL_I2C_Master_Transmit(&hi2c1,0x77 << 1,&requestPROM,1,300);
	HAL_I2C_Master_Receive(&hi2c1 ,0x77 << 1,dataPROM, 2,1000);
	//TODO: in production code, change this to DMA USART TX instead of blocking (increase baud!!!)
	HAL_UART_Transmit(&huart2, dataPROM, sizeof(dataPROM), 100);
	switch(requestPROM){
				case 0xA0: HAL_Delay(1); break;
				case 0xA2: C1=((uint16_t)dataPROM[0] << 8) | dataPROM[1]; break;
				case 0xA4: C2=((uint16_t)dataPROM[0] << 8) | dataPROM[1]; break;
				case 0xA6: C3=((uint16_t)dataPROM[0] << 8) | dataPROM[1]; break;
				case 0xA8: C4=((uint16_t)dataPROM[0] << 8) | dataPROM[1]; break;
				case 0xAA: C5=((uint16_t)dataPROM[0] << 8) | dataPROM[1]; break;
				case 0xAC: C6=((uint16_t)dataPROM[0] << 8) | dataPROM[1]; break;
				default: errorInterrupt("ERR_MS5611_PROM_C_ASSIGN");
	}
	requestPROM = 0xA0 + (i*2); //0xA0 -> 0xAC
	}
	HAL_UART_Transmit(&huart2, (uint8_t*)&C1, sizeof(C1), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)&C2, sizeof(C2), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)&C3, sizeof(C3), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)&C4, sizeof(C4), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)&C5, sizeof(C5), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)&C6, sizeof(C6), HAL_MAX_DELAY);
}

uint8_t readADC = 0x00;
uint8_t D1[3];
uint8_t D2[3];
uint32_t D1_val;
uint32_t D2_val;
void d1Read(){
	uint8_t requestD1 = 0x48; //OSR=4096
	HAL_I2C_Master_Transmit(&hi2c1,0x77 << 1,&requestD1,1,300); //Start pressure conversion
	HAL_Delay(20); //Wait for conversion (9.04ms MAX as per the docs)
	HAL_I2C_Master_Transmit(&hi2c1,0x77 << 1,&readADC,1,300); //request sending of ADC data
	HAL_I2C_Master_Receive(&hi2c1 ,0x77 << 1,D1, 3,1000); //read ADC data
	D1_val = ((uint32_t)D1[0] << 16) | ((uint32_t)D1[1] << 8) | D1[2];
}
void d2Read(){
	uint8_t requestD2 = 0x52 ;//OSR=512
	HAL_I2C_Master_Transmit(&hi2c1,0x77 << 1,&requestD2,1,300); //Start pressure conversion
	HAL_Delay(5); //Wait for conversion (1.17ms MAX as per the docs)
	HAL_I2C_Master_Transmit(&hi2c1,0x77 << 1,&readADC,1,300); //request sending of ADC data
	HAL_I2C_Master_Receive(&hi2c1 ,0x77 << 1,D2, 3,1000); //read ADC data
	D2_val = ((uint32_t)D2[0] << 16) | ((uint32_t)D2[1] << 8) | D2[2];
}

int64_t OFF, SENS, OFF2, SENS2, dT, TEMP, P, T2;
void firstComp(){
	dT = D2_val - (C5*pow(2, 8));
	OFF = C2*pow(2, 16) + (C4*dT)/pow(2, 7);
	SENS = C1*pow(2, 15)+(C3*dT)/pow(2, 8);
}
void firstAdcCalc(){
	TEMP = 2000 + dT * C6/pow(2, 23); //f
	P = (D1_val * SENS/pow(2, 21)-OFF)/pow(2, 15);
}
void adcCalc(){
	TEMP = 2000 + dT * C6/pow(2, 23);
	P = (D1_val * SENS/pow(2, 21)-OFF)/pow(2, 15);
	TEMP = TEMP - T2;
}
void tempComp(){
	TEMP = 2000 + dT * C6/pow(2, 23);
	P = (D1_val * SENS/pow(2, 21)-OFF)/pow(2, 15);
	if(TEMP < 2000){
		T2 = (dT*dT)/pow(2, 31);
		OFF2 = 5*((TEMP-2000)*(TEMP-2000))/2;
		SENS2 = 5*((TEMP-2000)*(TEMP-2000))/(2*2);
		if(TEMP < -1500){
			OFF2 = OFF2 + 7 * ((TEMP+1500)*(TEMP+1500));
			SENS2 = SENS2 + 11 * ((TEMP+1500)*(TEMP+1500))/2;
		}
	}
	else{
		T2 = 0;
		OFF2 = 0;
		SENS2 = 0;
	}
	OFF = OFF - OFF2;
	SENS = SENS - SENS2;
}

void baroUart(){
    char buf[64];
    snprintf(buf, sizeof(buf), "#TEMP:%ld #P:%ld\r\n", (long)TEMP, (long)P);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
}

////////////////////////////////////////////BNO085/////////////////////////////////////////////

//-The BNO085 does kalman filtering by itself on the game rotation vector, and also does
// filtering on the other sensors. I mean, it's literally a mini ARM computer


	uint8_t readStartup(){
	    while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET){
	        uint8_t peekHeader[4];
	        if(HAL_I2C_Master_Receive(&hi2c1, 0x4B << 1, peekHeader, 4, 5000) != HAL_OK){
	            errorInterrupt("ERR_BNO085_BOOT_HEADER_READ");
	            return 1;
	        }
	        uint16_t totalSize = peekHeader[1] << 8 | peekHeader[0];
	        if(totalSize == 0xFFFF || totalSize < 4 || totalSize > 256){
	            errorInterrupt("ERR_BNO085_BOOT_PCKG_SIZE");

	        }

	        uint8_t fullPacket[256];
	        if(HAL_I2C_Master_Receive(&hi2c1, 0x4B << 1, fullPacket, totalSize, 5000) != HAL_OK){
	            errorInterrupt("ERR_BNO085_BOOT_DATA_READ");
	            return 3;
	        }
	        HAL_UART_Transmit(&huart2, fullPacket + 4, totalSize - 4, HAL_MAX_DELAY);
	    }
	    return 0;
	}


//Byte 0: Length LSB
//Byte 1: Length MSB
//Byte 2: Channel
//Byte 3: SeqNum
uint8_t bnoHeader[4];
uint16_t packageSize;
uint8_t packageChannel;
uint8_t seqNumTransmit[6] = {0, 0, 0, 0, 0, 0};
uint8_t seqNumReceive[6] = {0, 0, 0, 0, 0, 0};
int16_t gyroX, gyroY, gyroZ;
int16_t linAccX, linAccY, linAccZ;
int16_t quatI, quatJ, quatK, quatW;
uint8_t bnoAccel(){
		//char dbg[48];
	    //snprintf(dbg, sizeof(dbg), "INTN_STATE: %d\r\n", HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4));
	    //HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 100);

    while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_4) == GPIO_PIN_RESET){
        uint8_t peekHeader[4];
        if(HAL_I2C_Master_Receive(&hi2c1, 0x4B << 1, peekHeader, 4, 2000) != HAL_OK){
            errorInterrupt("ERR_BNO085_HEADER_READ");
            return 1;
        }
        uint16_t totalSize = peekHeader[1] << 8 | peekHeader[0];
        if(totalSize == 0xFFFF || totalSize < 4 || totalSize > 256){
            errorInterrupt("ERR_BNO085_PCKG_SIZE");
        }

        uint8_t fullPacket[256];
        if(HAL_I2C_Master_Receive(&hi2c1, 0x4B << 1, fullPacket, totalSize, 2000) != HAL_OK){
            errorInterrupt("ERR_BNO085_DATA_READ");
            return 3;
        }

        packageChannel = fullPacket[2];
        seqNumReceive[fullPacket[2]] = fullPacket[3];

        uint8_t *payload = fullPacket + 4;
        uint16_t payloadSize = totalSize - 4;
        uint16_t offset = 0;
        char buf[64];
                char errBuf[48];
                char dbg2[48];
        while(offset < payloadSize){
            uint8_t reportID = payload[offset];
            //char dbg[48];
            //snprintf(dbg, sizeof(dbg), "REPORT_ID: 0x%02X offset:%u\r\n", reportID, offset);
            //HAL_UART_Transmit(&huart2, (uint8_t*)dbg, strlen(dbg), 100);

            switch(reportID){
                case 0xFB: // Timestamp Rebase - 5 bytes
                    offset += 5;
                    break;
                case 0x02: // Gyro - 10 bytes
                    gyroX = payload[offset+5] << 8 | payload[offset+4];
                    gyroY = payload[offset+7] << 8 | payload[offset+6];
                    gyroZ = payload[offset+9] << 8 | payload[offset+8];
                    snprintf(buf, sizeof(buf), "#GyroX:%d #GyroY:%d #GyroZ:%d\r\n", (long)gyroX, (long)gyroY, (long)gyroZ);
                    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
                    offset += 10;
                    break;
                case 0x04: // Linear Accel - 10 bytes
                    linAccX = payload[offset+5] << 8 | payload[offset+4];
                    linAccY = payload[offset+7] << 8 | payload[offset+6];
                    linAccZ = payload[offset+9] << 8 | payload[offset+8];
                    snprintf(buf, sizeof(buf), "#linX:%d #linY:%d #linZ:%d\r\n", (long)linAccX, (long)linAccY, (long)linAccZ);
                    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
                    offset += 10;
                    break;
                case 0x08: // Game Rotation Vector - 14 bytes
                    quatI = payload[offset+5] << 8 | payload[offset+4];
                    quatJ = payload[offset+7] << 8 | payload[offset+6];
                    quatK = payload[offset+9] << 8 | payload[offset+8];
                    quatW = payload[offset+11] << 8 | payload[offset+10];
                    snprintf(buf, sizeof(buf), "#quatI:%d #quatJ:%d #quatK:%d #quatW:%d\r\n", (long)quatI, (long)quatJ, (long)quatK, (long)quatW);
                    HAL_UART_Transmit(&huart2, (uint8_t*)buf, strlen(buf), HAL_MAX_DELAY);
                    offset += 14;
                    break;
                default:
                {
                    char errBuf[48];
                    snprintf(errBuf, sizeof(errBuf), "UNKNOWN_REPORT: 0x%02X offset:%u\r\n", reportID, offset);
                    HAL_UART_Transmit(&huart2, (uint8_t*)errBuf, strlen(errBuf), 100);
                    goto exit_loop; // unknown report size, can't continue
                }
            }
        }
}
    exit_loop:;
    return 0;
}


uint8_t setFeatureGyro(){
	uint8_t featureHeader[4] = {0x15, //LSB (Header + Payload = 21 bytes)
								0x00, //MSB
								0x02, //Channel
								seqNumTransmit[2]}; //seqNum for channel 2 TODO: increment this
	uint8_t enableGyro[17] = {0xFD, //Report ID (static?)
							   0x02, //for gyro (SH-2 Reference manual Page 38)
							   0x00,
							   0x00,
							   0x00,
							   0x50, //Sampling rate LSB (50ms = 50000us)
							   0xC3, //Sampling rate MSB
							   0x00,
							   0x00,
							   0x00,//
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00};
	uint8_t enableGyroData[21];
	memcpy(enableGyroData, featureHeader, 4);
	memcpy(enableGyroData + 4, enableGyro, 17);
	if(HAL_I2C_Master_Transmit(&hi2c1, 0x4B << 1, enableGyroData, 21, 1000) != HAL_OK){
		errorInterrupt("ERR_BNO_SET_GYRO");
		return 1;
	}
	seqNumTransmit[2]++;
	return 0;
}

//Game vector because it excludes magnetometer, which probably wouldn't be accurate in an environment
//like a rocket
uint8_t setFeatureGameVector(){
	uint8_t featureHeader[4] = {0x15, //LSB (Header + Payload = 21 bytes)
								0x00, //MSB
								0x02, //Channel
								seqNumTransmit[2]}; //seqNum for channel 2
	uint8_t enableGame[17] = {0xFD, //Report ID (static?)
							   0x08, //for game rotation vector (SH-2 Reference manual Page 38)
							   0x00,
							   0x00,
							   0x00,
							   0x50, //Sampling rate LSB (50ms = 50000us)
							   0xC3, //Sampling rate MSB
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00};
	uint8_t enableGameData[21];
	memcpy(enableGameData, featureHeader, 4);
	memcpy(enableGameData + 4, enableGame, 17);
	if(HAL_I2C_Master_Transmit(&hi2c1, 0x4B << 1, enableGameData, 21, 1000) != HAL_OK){
		errorInterrupt("ERR_BNO_SET_GAME");
		return 1;
	}
	seqNumTransmit[2]++;
	return 0;
}

uint8_t setFeatureLinearAccel(){
	uint8_t featureHeader[4] = {0x15, //LSB (Header + Payload = 21 bytes)
								0x00, //MSB
								0x02, //Channel
								seqNumTransmit[2]}; //seqNum for channel 2 TODO: increment this
	uint8_t enableLinear[17] = {0xFD, //Report ID (static?)
							   0x04, //for linear acceleration (SH-2 Reference manual Page 38)
							   0x00,
							   0x00,
							   0x00,
							   0x50, //Sampling rate LSB (50ms = 50000us)
							   0xC3, //Sampling rate MSB
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00,
							   0x00};
	uint8_t enableLinearData[21];
	memcpy(enableLinearData, featureHeader, 4);
	memcpy(enableLinearData + 4, enableLinear, 17);
	if(HAL_I2C_Master_Transmit(&hi2c1, 0x4B << 1, enableLinearData, 21, 1000) != HAL_OK){
		errorInterrupt("ERR_BNO_SET_LINEAR");
		return 1;
	}
	seqNumTransmit[2]++;
	return 0;
}

/////////////////////////////////////////PIC18F46K22//////////////////////////////////////////////////


/* Okay, so hear me out.
 * I had an elegant method to do this with delimiters and memory copying
 * And that worked in a previous version where I was just demoing the features
 * Yet here where everything is exactly the same besides the main() loop function calls
 * it doesn't work.
 * ...
 * So this is arguably not as pretty or sophisticated
 * but honestly, it's probably faster and needs less overhead than what I had before lol
*/
void readPIC(){
    uint8_t picBuffer;
    bool breakNext = false;
    for(int i = 0; i <= 200; i++){
    if(breakNext == true){break;}
    HAL_UART_Receive(&huart3, &picBuffer, 1, 1000); //Get too much data so DMA wizardry not needed
    HAL_UART_Transmit(&huart2, &picBuffer, 1, 1000);
    if((char)picBuffer == 'D'){breakNext = true;}
    }
}

///////////////////////////////////////////GPS/////////////////////////////////////////////////////////


char lineBuf[83] = {0};
uint8_t lineLen     = 0;
void readGPS(void)
{
	    uint8_t byte;
	    while (1)
	    {
	        if (HAL_UART_Receive(&huart4, &byte, 1, 1000) != HAL_OK) {
	            continue;
	        }

	        if (byte == '$') {
	            lineBuf[0] = '$';
	            lineLen    = 1;
	            continue;
	        }

	        if (lineLen == 0) {
	            continue;
	        }

	        if (lineLen >= 82) {
	            lineLen = 0;
	            continue;
	        }

	        lineBuf[lineLen++] = (char)byte;
	        lineBuf[lineLen]   = '\0';

	        if (byte == '\n' && lineLen >= 2 && lineBuf[lineLen - 2] == '\r')
	        {
	            if (lineLen > 6 &&
	                lineBuf[0] == '$' &&
	                lineBuf[1] == 'G' &&
	                lineBuf[2] == 'N' &&
	                lineBuf[3] == 'R' &&
	                lineBuf[4] == 'M' &&
	                lineBuf[5] == 'C')
	            {

	                HAL_UART_Transmit(&huart2, (uint8_t *)lineBuf, lineLen, 1000);
	                return;  	            }

	            lineLen = 0;  // wrong sentence type, reset and keep looking
	        }
	    }
}


void gpsRaw() { //Just for debugging
    uint8_t byte;
    while (1) {
        if (HAL_UART_Receive(&huart4, &byte, 1, 1000) == HAL_OK) {
            HAL_UART_Transmit(&huart2, &byte, 1, 1000);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////



//quaternion: page 86


int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_UART4_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */
  HAL_Delay(3000);
  bool armed_c1 = false;
    baroReset();
    baroPROM();
    d1Read();
    d2Read();
    firstComp();
    firstAdcCalc();
    tempComp();
    adcCalc();

    HAL_Delay(1000);
    for(int i = 0; i == 5; i++){ //In case there are multiple startup packets
  		  readStartup();
    }


    HAL_Delay(300);
    setFeatureGyro();


    HAL_Delay(5);
    setFeatureLinearAccel();


    HAL_Delay(5);
    setFeatureGameVector();




    /*
     * Commands (Should not start or end with letter already used):
     * ARM
     * DRM (Disarm - No sensor data stream)
     * ORI (Orientation - only BNO data)
     * NAV (Navigation - BNO and GPS data)
     * BAR (Barometer - BNO and Barometer data)
     * SRC (Servo Current - only Servo current draw data)
     *
     * POS/NEG for servo PIC
     * STP to stop servo PIC
     */
    uint8_t cmCommand[3];

	char endMark[] = "\n\r\0";
	char ackTrans[4] = "#ACK";
	char dArm[] = "#Disarmed";
	char zeroServ[] = "#Zero_Servos_POS/NEG_?"; //Positive/Negative because 'NO' is only two letters
	char stopMsg[] = "#Send_STP_to_Stop";

	bool armed_c2 = false;
    //Two different armed markers at vastly different memory addresses to protect against bitflips

		while(1){
		armed_c1 = false;
		armed_c2 = false;
			do{
				HAL_UART_Receive(&huart2, cmCommand, 3, 2000);
				if((char)cmCommand[0] == 'A' && (char)cmCommand[1] == 'R' && (char)cmCommand[2] == 'M'){
					armed_c1 = true;
					armed_c2 = true;
					HAL_UART_Transmit(&huart2, (uint8_t*)ackTrans, strlen(ackTrans), HAL_MAX_DELAY);
					HAL_UART_Transmit(&huart2, (uint8_t*)endMark, strlen(endMark), HAL_MAX_DELAY);
				}
				else{HAL_UART_Transmit(&huart2, (uint8_t*)dArm, strlen(dArm), HAL_MAX_DELAY);}
			}while(armed_c1 == false || armed_c2 == false);

			invalidCommand:;
			HAL_UART_Transmit(&huart2, (uint8_t*)zeroServ, strlen(zeroServ), HAL_MAX_DELAY);
			HAL_UART_Receive(&huart2, cmCommand, 3, HAL_MAX_DELAY);
			if((char)cmCommand[0] == 'P' && (char)cmCommand[2] == 'S'){
				HAL_UART_Transmit(&huart2, (uint8_t*)stopMsg, strlen(stopMsg), HAL_MAX_DELAY);
				bool doneZero = false;
				do{
				readPIC();
				HAL_UART_Receive(&huart2, cmCommand, 3, 1000);
				if((char)cmCommand[0] == 'S' && (char)cmCommand[2] == 'P'){doneZero = true;}
				}while(doneZero == false);
			}
			else if((char)cmCommand[0] == 'N' && (char)cmCommand[2] == 'G'){}
			else{goto invalidCommand;}

				while (armed_c1 == true && armed_c2 == true){
							bnoAccel();
							d1Read();
							d2Read();
							bnoAccel();
							firstComp();
							firstAdcCalc();
							tempComp();
							adcCalc();
							baroUart();
							bnoAccel();
							readGPS();
							for(int k = 0; k >= 20; k++){
								HAL_UART_Transmit(&huart2, (uint8_t*)endMark, strlen(endMark), HAL_MAX_DELAY);
								bnoAccel();
							}
							HAL_UART_Receive(&huart2, cmCommand, 3, 30);
							if((char)cmCommand[0] == 'D' && (char)cmCommand[2] == 'M'){
								armed_c1 = false;
								armed_c2 = false;
								HAL_UART_Transmit(&huart2, (uint8_t*)endMark, strlen(endMark), HAL_MAX_DELAY);
							}
					 /* USER CODE BEGIN 3 */
			}
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00303D5B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 9600;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 230400;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 57600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : BNO_FLAG_Pin */
  GPIO_InitStruct.Pin = BNO_FLAG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BNO_FLAG_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
