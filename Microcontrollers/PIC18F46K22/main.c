/*
 * File:   main.c
 * Author: Tamás Z. Tóth
 * 
 * Project GitHub: OpenApollo
 * Personal GitHub: ReTTr0c 
 * Project Website: openapollo.space
 * 
 * For OTIO (Hungarian National Science and Innovation Olympiad)
 * Created on March 13, 2026, 12:29 AM
 * ///////////////////////////////////////////////////////////////////
 * For PIC18F46K22
 * ADC and UART setup, measurement & transmission
 * Note: basically every delay and acquisition time here is extremely overkill
 * 
 * EUSART baud: 57600 bits/s
 * ADC channels: 0-12
 */


#include <xc.h>
#include <stdint.h>
#include <stdio.h>
#include <pic18f46k22.h>
#define _XTAL_FREQ 16000000UL


const float R = 0.1;

void uartsuccess(void){ //Send UART signal if UART setup successful
    char message[] = "UART setup successful\r\n";
for (int i = 0; message[i] != '\0'; i++) { //'\0' is marker for end of Char array in C.
        while(PIR1bits.TXIF == 0){; // Wait until TXREG empty

    }   
    TXREG1 = message[i];
}
    __delay_ms(5);
}

void adcsuccess(void){ //Send UART signal if ADC setup successful
    char message[] = "ADC setup successful\r\n";
for (int i = 0; message[i] != '\0'; i++) { //'\0' is marker for end of Char array in C.
        while(PIR1bits.TXIF == 0){; // Wait until TXREG empty

    }   
    TXREG1 = message[i];
}
    __delay_ms(5);
}

void uartsetup(void){
    
    ANSELCbits.ANSC6 = 0; // 0 = Digital, 1 = Analog Input
    TRISCbits.TRISC6 = 0;// 0 = Output, 1 = Input
    
    BAUDCON1bits.CKTXP = 0; //Set TX Idle to HIGH (default)
    BAUDCON1bits.BRG16 = 0; //8-bit Baud Rate Generator is used

    SPBRG1 = 16;            // 57.6k baud   
    //B-rates; 9600: 103 - 10417: 95 - 19.2k: 51 - 57.6k: 16 - 115.2k: 8
 
    RCSTA1bits.SPEN = 1; //Configure TX/RX pins (And configure receiving)
    
    TXSTA1bits.TXEN = 1; //Enable Transmission
    TXSTA1bits.SYNC = 0; //Set to Async.
    
    TXSTA1bits.TX9 = 0; //Select 8-bit transmission
    TXSTA1bits.BRGH = 1; //Select High-speed TX
          
}

void adcsetup(void){
    
    //AN0
    ANSELAbits.ANSA0 = 1; // 0 = Digital, 1 = Analog
    TRISAbits.TRISA0 = 1;// 0 = Output, 1 = Input
    //AN1
    ANSELAbits.ANSA1 = 1; 
    TRISAbits.TRISA1 = 1;
    //AN2
    ANSELAbits.ANSA2 = 1; 
    TRISAbits.TRISA2 = 1;
    //AN3
    ANSELAbits.ANSA3 = 1;
    TRISAbits.TRISA3 = 1; 
    //AN4
    ANSELAbits.ANSA5 = 1;
    TRISAbits.TRISA5 = 1;
    //AN5
    ANSELEbits.ANSE0 = 1;
    TRISEbits.TRISE0 = 1;
    //AN6
    ANSELEbits.ANSE1 = 1;
    TRISEbits.TRISE1 = 1;
    //AN7
    ANSELEbits.ANSE2 = 1;
    TRISEbits.TRISE2 = 1;
    //AN8
    ANSELBbits.ANSB2 = 1;
    TRISBbits.TRISB2 = 1;
    //AN9
    ANSELBbits.ANSB3 = 1;
    TRISBbits.TRISB3 = 1;
    //AN10
    ANSELBbits.ANSB1 = 1;
    TRISBbits.TRISB1 = 1;
    //AN11
    ANSELBbits.ANSB4 = 1;
    TRISBbits.TRISB4 = 1;
    //AN12
    ANSELBbits.ANSB0 = 1;
    TRISBbits.TRISB0 = 1;
    //Not used here:
    //AN13
    //ANSELBbits.ANSB5 = 1;
    //TRISBbits.TRISB5 = 1;
    //AN14
    //ANSELCbits.ANSC2 = 1;
    //TRISCbits.TRISC2 = 1;
    
    ///////////////////////////////////////////////////////////////////////
    
    ADCON0bits.ADON = 1; //Enable ADC.
    
    ADCON1bits.PVCFG = 0b10; //Select FVR BUF2 as Vref
    VREFCON0bits.FVREN = 1; //Enable fixed Vref
    VREFCON0bits.FVRS = 0b01; //Set Vref to 1x (1.024V)
    
    ADCON2bits.ADCS = 0b010; //For 16MHz clock, ~2 uS TAD time (FOSC/32)
    ADCON2bits.ADFM = 1; //Set ADC results RIGHT justified.
    ADCON2bits.ACQT = 0b111; //20 TAD (20x2uS = 40uS) acquisition time (really conservative).
    
    while(VREFCON0bits.FVRST == 0){ //Wait until fixed Vref is up
        __delay_ms(1);
    }
}
void uartStart(){
    const char startMarker[] = "#START\r\n";
    for (int i = 0; startMarker[i] != '\0'; i++) { //'\0' is marker for end of Char array in C.
        while(PIR1bits.TXIF == 0); // Wait until TXREG empty
        TXREG1 = startMarker[i];
    }

}
void uartEnd(){
    const char endMarker[] = "#END\r\n";
    for (int i = 0; endMarker[i] != '\0'; i++) { //'\0' is marker for end of Char array in C.
        while(PIR1bits.TXIF == 0); // Wait until TXREG empty
        TXREG1 = endMarker[i];
    }
    
}

void uarttx(char message[], char Anum[]){
    const char newline[] = "\r\n";

for (int i = 0; Anum[i] != '\0'; i++) { //'\0' is marker for end of Char array in C.
        while(PIR1bits.TXIF == 0); // Wait until TXREG empty
        TXREG1 = Anum[i];
    }
for (int i = 0; message[i] != '\0'; i++) { //'\0' is marker for end of Char array in C.
        while(PIR1bits.TXIF == 0); // Wait until TXREG empty
        TXREG1 = message[i];
    }
for (int i = 0; newline[i] != '\0'; i++) { //'\0' is marker for end of Char array in C.
        while(PIR1bits.TXIF == 0); // Wait until TXREG empty
        TXREG1 = newline[i];
    }   
}


void dectohex(uint16_t adcvalue, char Anum[]) 
{
    char hexvalue[6];
    uint16_t currentInt;

    float V = (float)adcvalue / 1000;
    
    // Compute current in amps
    float current = V / R;

    // Scale to milli-amps
    currentInt = (uint16_t)(current * 1000.0f); 
    
    uint16_t number = currentInt;
    char numberValue[20];
    sprintf(numberValue, "%d", number);

    // Send via UART
    uarttx(numberValue, Anum);
}

void main(void) {
    
    OSCCONbits.IRCF = 0b111; //select 16MHz Clock
    while(OSCCONbits.HFIOFS == 0){ //Wait until clock stabilizes
        __delay_ms(10);
    }
    
    uartsetup();
    uartsuccess();
     __delay_ms(10);
    adcsetup();
    adcsuccess();
    
    uint16_t results[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};    
    uint8_t ChSEL = 0b00000;
    char label[7];   // "#A14:\0"
    label[0] = '#';
    label[1] = 'A';
    __delay_ms(3000); //not needed, just want it to start slower
    
    while(1){
        for(int i = 0; i <= 12; i++){
            if(i == 0){
                uartStart();
            }
            ADCON0bits.CHS = ChSEL; //Select Channel
            __delay_ms(5); //Delay for channel switching
            
            for(int j = 0; j <= 3; j++){//3 times dummy sampling for more accuracy (to prevent sampling cap spillover)
            ADCON0bits.GO_nDONE = 1; //Start sampling 
            while(ADCON0bits.GO_nDONE == 1){} //Wait until sampling is done.
            }            
            
            ADCON0bits.GO_nDONE = 1; //Start sampling (true sampling cycle)
            while(ADCON0bits.GO_nDONE == 1){} //Wait until sampling is done.
            results[i] = ((uint16_t)ADRESH << 8) | ADRESL;
            
            if (ChSEL < 10) {
                label[2] = '0';
                label[3] = '0' + ChSEL;
                label[4] = ':';
                label[5] = '\0';
            } else {
                label[2] = '0' + (ChSEL / 10);
                label[3] = '0' + (ChSEL % 10);
                label[4] = ':';
                label[5] = '\0';
            }

            dectohex(results[i], label);
              ChSEL++;
            if(i == 12){
                uartEnd();
            }
    
        }
        ChSEL = 0b00000;
       __delay_ms(500);

    }
}

//If changing amount of ADC channels, don't forget line 207, 215