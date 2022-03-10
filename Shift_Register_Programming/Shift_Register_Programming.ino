/*
 * Programming the HC595 Shift Register
 *
 * Use of the shift regioster will allow us to save pins of the atmega for the connecting of the 7 segment display
 * For shift register operation:
 *  Connect pins 11 and 12 (SHCP, STCP) together, these are clocks for the 2 internal register blocks
 *  To reset the outputs pull pin 10 (MR) low and toggle the clocks, otherwise leave pin 10 (MR) high
 *  Can pull Pin 13 (OE) low after writing to register is completed to display the data
 *  Pin 14 (DS) is the data input
 *  
 * This is connected to the ATMEGA as follows:
 *  PIND0:  SHCP, STCP
 *  PIND1:  MR
 *  PIND2:  OE
 *  PIND3:  DS
 *
 */
 // Imports of header files for the ATMEGA register mapping as well as the standard C libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// #include <util/delay.h>
#include <stdbool.h>
#include <string.h>

void toggle_clk();
void update_shift_register_output(int *digits, int arr_size);
void reset_shift_register();

/*
 * Toggle the clk Pin
 * PORTD Pin 0
 */
void toggle_clk()
{
  PORTD |= 0b00000001;
  PORTD &= 0b11111110;
}

/*
 * Function to update the register output with array passed
 * Turn output on afterwards, pull OE low
 *  PORTD PIN 3
 */
void update_shift_register_output(int *digits, int arr_size)
{
  // Change DS to array, pulse clk
  for(int i=0; i<arr_size; i++)
  {
    // PIND3=digits[i];
    if( digits[i]>0){
      PORTD |= 0b00001000;
    }
    else{
      PORTD &= 0b11110111;
    }
    toggle_clk();
  }
  // OE high
  PORTD &= 0b11111011;
  toggle_clk();
  // toggle_clk();
}

/*
 * Function to reset the shift register output
 */
void reset_shift_register()
{
  // Pull MR low, pulse the clock, pull high
  // PIND1=0;
  PORTD &= 0b11111101;
  // PIND2=1;
  PORTD |= 0b00000100;
 
  toggle_clk();
  // PIND1=1;
  PORTD |= 0b00000010;
}

// Main function
int main(){
  // To start pull MR high, pull clks low and pull OE low
  // All of Port D configured as an output
  DDRD = 0xFF;
  PORTD = (0<<PIND0)|(1<<PIND1)|(0<<PIND2)|(1<<PIND3);
  // Connection of 7 segment display to the Shift Register will be as follows:
  // Q7 =a; Q6=b ..... Q1=g; Q0=dp;
  // Individual arrays for each digit, all encapsulated into a single array
  int dig_0[] = {0,1,1,1,1,1,1};
  int dig_1[] = {0,0,0,0,1,1,0};
  int dig_2[] = {1,0,1,1,0,1,1};
  int dig_3[] = {1,0,0,1,1,1,1};
  int dig_4[] = {1,1,0,0,1,1,0};
  int dig_5[] = {1,1,0,1,1,0,1};
  int dig_6[] = {1,1,1,1,1,0,1};
  // int dig_7[] = {0,0,0,0,1,1,1};
  int dig_7[] = {1,1,1,0,0,0,0};
  int dig_8[] = {1,1,1,1,1,1,1};
  int dig_9[] = {1,1,0,1,1,1,1};
//  int arr_dig[] = {&dig_9, &dig_8, &dig_7, &dig_6, &dig_5, &dig_4, &dig_3, &dig_2, &dig_1, &dig_0};
  // Array of arrays for the digits of each number
  int* arr_dig[10] = {dig_0,dig_1,dig_2,dig_3,dig_4,dig_5,dig_6,dig_7,dig_8,dig_9};
 
//  int arr_dig[10][7] = {
//    {0,1,1,1,1,1,1},
//    {0,0,0,0,1,1,0},
//    {1,0,1,1,0,1,1},
//    {1,0,0,1,1,1,1},
//    {1,1,0,0,1,1,0},
//    {1,1,0,1,1,0,1},
//    {1,1,1,1,1,0,1},
//    {0,0,0,0,1,1,1},
//    {1,1,1,1,1,1,1},
//    {1,1,0,1,1,1,1}
//  };
  reset_shift_register();
  int pattern1[] = {0,0,0,0,0,0,1};
  update_shift_register_output(arr_dig[7], 8);

//  for( int i = 0; i < 10; i++ ){
//    update_shift_register_output(arr_dig[i], 7);
//    // delay(500);
//    // reset_shift_register();
//    // delay(500);
//  }
 
}
