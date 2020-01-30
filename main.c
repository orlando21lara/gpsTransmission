/*
 * File:   main.c
 * Author: Orlando Lara Guzman
 *
 * Created on January 29, 2020, 10:07 AM
 * This is the main file to read information from a GPS sensor via UART and 
 * communicate it to a Jetson via I2C
 */

#pragma config FOSC = HS1       // Oscillator (HS oscillator (Medium power, 4 MHz - 16 MHz))
#pragma config XINST = OFF      // Extended instruction set disabled
#pragma config MCLRE = ON       // Master Clear Enable (MCLR Disabled, RG5 Enabled)
#pragma config SOSCSEL = DIG    // SOSC Power Selection and mode Configuration bits (Digital (SCLKI) mode)

#include <xc.h>
#include <string.h>
#include <stdio.h>
#include "I2CCom.h"
#include "uart_layer.h"
#include "pic18f25k80.h"

#define PIC1_ADDR 0x1B
#define PIC2_ADDR 0x1C
#define PIC3_ADDR 0x1D

unsigned char uart_data = 0;
bool no_errors = false;
unsigned char gps_buffer[3] = {0};
unsigned char input_message[256] = {0};
unsigned char print_buffer[256] = {0};

typedef struct{
    unsigned char hour              : 5;
    unsigned char status            : 1;
    unsigned char north_not_south   : 1;
    unsigned char east_not_west     : 1;

    unsigned char minutes;
    unsigned char seconds;
    
    unsigned char day;
    unsigned char month;
    unsigned char year;
    
    unsigned char lat_deg;
    unsigned char lat_min;
    unsigned char lat_min_dec_L;
    unsigned char lat_min_dec_H;
    
    unsigned char lon_deg;
    unsigned char lon_min;
    unsigned char lon_min_dec_L;
    unsigned char lon_min_dec_H;
}gps_data_read;

gps_data_read gps_data;

void portSetup(void){
    //Make sure that all unused IO pins are set to output and cleared
    TRISA = 0x00;
    TRISB = 0x00;
    TRISC = 0x00;
    
    PORTA = 0x00;
    PORTB = 0x00;
    PORTC = 0x00;
}

void parseData(unsigned char *c){
    
}

void __interrupt(high_priority) isr_high(void){  
    
    if(PIE1bits.RC1IE && PIR1bits.RC1IF){
        static unsigned char n;     //initialized with 0 even if not explicitly
        static bool save_input = false;
        
        uartReceive(&uart_data, &no_errors);
        
        if(no_errors){   
            switch(uart_data){
                case '$':
                    if(!save_input){
                        save_input = true;
                        n = 0;
                        input_message[n++] = uart_data;
                    }
                    break;
                case '\n':
                    if(save_input){
                        input_message[n++] = uart_data;
                        parseData(input_message);
                        memset(input_message, 0, sizeof(input_message));
                        save_input = false;
                    }
                    break;
                default:
                    if(save_input){
                        input_message[n++] = uart_data;
                    }
                    break;
            }
        }
    }
    
    if(SSPIE && SSPIF){
        static unsigned char n;
        static unsigned char i;
        SSPIF = 0;  //reset flag
        I2CCheckError();
        
        if(!SSPSTATbits.D_NOT_A && SSPSTATbits.R_NOT_W){                        //If last byte was address and read
            n = 0;
            I2CSend(gps_buffer[n++]);
        }
        else if(SSPSTATbits.D_NOT_A && SSPSTATbits.R_NOT_W && SSPSTATbits.S){   //if last byte was data and read, and start bit was detected last    
            I2CSend(gps_buffer[n++]);
        }
    }
}

void main(void) {
    IPEN = 1;       //Enable interrupt priorities
    GIEH = 1;       //Enable High priority interruptions
    
    portSetup();
    I2CInit(0, PIC1_ADDR);
    uartInit(25,0,0,0);
    
    memset(&gps_data,0,sizeof(gps_data));
    
    gps_data.hour = 16;
    gps_data.minutes = 58;
    gps_data.seconds = 59;
    
    gps_data.status = 1;
    gps_data.north_not_south = 1;
    gps_data.east_not_west = 1;
    
    gps_data.day = 31;
    gps_data.month = 2;
    gps_data.year = 222;
    
    memcpy(gps_buffer,&gps_data,sizeof(gps_data));
    while(1);
}
