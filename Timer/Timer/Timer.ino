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
void update_shift_register_output(int *digits, int arr_size, int dp);
void reset_shift_register();
void timer2_setup();
void timer0_setup();
void increment_digits();
// void refresh_display();

int dig0, dig1, dig2, dig3 = 2;
int counter = 0;
volatile char mode = 'a';
volatile char set_mode = 'd';

// Initialising the Timer0 Overflow counter setup
void timer0_setup()
{
  // Select Clock prescaler to /64
  TCCR0B = 0b00000100;
  // Enable Timer overflow counter
  TIMSK0 = 0b00000001;
}

// Initialising the Timer Overflow Counter 2
void timer2_setup()
{
  // Clock divider set to 64
  TCCR2B = 0b00000100;
  // Enable Timer overflow
  TIMSK2 = 0b00000001;
}


// Function to increment the digits being displayed
void increment_digits()
{
  //If LSB is @9, set to 0 and increment next digit
  if (dig0 > 8) {
    dig0 = 0;
    if (dig1 > 4) { // If digit is @5, set to 0 and increment next digit
      dig1 = 0;
      if (dig2 > 8) { //If digit is @9 set to and increment next digit
        dig2 = 0;
        if (dig3 > 8) { //If digit is @9, reset all digits
          dig0, dig1, dig2, dig3 = 0;
        }
        else {
          dig3++;
        }
      }
      else {
        dig2++;
      }
    }
    else {
      dig1++;
    }
  }
  else {
    dig0++;
  }
}

// Function to decrement the digits being displayed
void decrement_digits()
{
  if(dig0 < 1)
  {
    if(dig1 < 1)
    {
      if(dig2 < 1)
      {
        if(dig3 < 1){
          TIMSK2=0b00000000;
        }
        else{
          dig3--;
          dig2=9; dig0 = 9;
          dig1 = 5;
        }
      }
      else{
        dig2--;
        dig0 = 9;
        dig1 = 5;
      }
    }
    else{
      dig1--;
      dig0 = 9;
    }
  }
  else
  {
    dig0--;
  }
}

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
void update_shift_register_output(int *digits, int arr_size, int dp)
{
  // Change DS to array, pulse clk
  for(int i=0; i<arr_size; i++)
  {
    // Pulsing the data for the desired digit through the shift register
    // PIND3=digits[i];
    if( digits[i]>0){
      PORTD |= 0b00001000;
    }
    else{
      PORTD &= 0b11110111;
    }
    toggle_clk();
  }
 
  // Adding decimal point if desired
  if(dp > 0){
    PORTD |= 0b00001000;
  }
  else{
    PORTD &= 0b11110111;
  }
  
  // OE = on
  PORTD &= 0b11111011;
  toggle_clk();
  toggle_clk();
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
  // PORTD = (0<<PIND0)|(1<<PIND1)|(0<<PIND2)|(1<<PIND3);
  // The 4 digits are controlled through PORTD pins 4,5,6,7 (D1,2,3,4)
  // Establishing start up values of PORTD
  PORTD = 0b11101010;

  // Setting up both timers
  timer2_setup();
  timer0_setup();

  // Initialisation of software counter to 0
  counter = 0;
  
  int display_digit, dig_count = 0;
  
  // Connection of 7 segment display to the Shift Register will be as follows:
  // Q7 =a; Q6=b ..... Q1=g; Q0=dp;
  // Individual arrays for each digit, all encapsulated into a single array
  int dig_0[] = {1,1,1,1,1,1,0};
  int dig_1[] = {0,1,1,0,0,0,0};
  int dig_2[] = {1,1,0,1,1,0,1};
  int dig_3[] = {1,1,1,1,0,0,1};
  int dig_4[] = {0,1,1,0,0,1,1};
  int dig_5[] = {1,0,1,1,0,1,1};
  int dig_6[] = {1,0,1,1,1,1,1};
  int dig_7[] = {1,1,1,0,0,0,0};
  int dig_8[] = {1,1,1,1,1,1,1};
  int dig_9[] = {1,1,1,1,0,1,1};

  // Array of arrays for the digits of each number
  int* arr_dig[10] = {dig_0,dig_1,dig_2,dig_3,dig_4,dig_5,dig_6,dig_7,dig_8,dig_9};

  reset_shift_register();
  // PORTD &= 0b11111101;
  // update_shift_register_output(arr_dig[1], 7, 1);
  // Enabling global interrupts
  SREG |= 0b10000000;
  sei();
  while(1)
  {
    switch(mode)
    {
      case 'a':
      // Default do nothing state
        break;
      case 'd':
      // Decrement the display
        SREG &= 0b01111111;
        decrement_digits();
        mode = 'a';
        SREG |= 0b10000000;
        TIMSK0 = 0b00000001;
        break;
      case 'i':
        // Increment the display
        SREG &= 0b01111111;
        increment_digits();
        // reset_shift_register();
        // update_shift_register_output(arr_dig[dig0], 7, 0);
        mode = 'a';
        SREG |= 0b10000000;
        // Enable refresh timer
        TIMSK0 = 0b00000001;
        break;
      case 'r':
        // Refresh the display
        SREG &= 0b01111111;
        if(dig_count < 1){
          PORTD |= 0b11110000;
          PORTD &= 0b11101111;
          display_digit = dig3;
          dig_count++;
        }else if(dig_count < 2){
          PORTD |= 0b11110000;
          PORTD &= 0b11011111;
          display_digit = dig2;
          dig_count++;
        }else if(dig_count < 3){
          PORTD |= 0b11110000;
          PORTD &= 0b10111111;
          display_digit = dig1;
          dig_count++;
        }else{
          PORTD |= 0b11110000;
          PORTD &= 0b01111111;
          display_digit = dig0;
          dig_count = 0;
        }
        reset_shift_register();
        update_shift_register_output(arr_dig[display_digit], 7, 0);
        SREG |= 0b10000000;
        mode = 'a';
        break;
      default:
        break;
    }
  }
  return 0;
}

ISR(TIMER0_OVF_vect)
{
  // Refresh display, ie. move to next digit
  mode = 'r';
}

// Timer2 Overflow interrupt to increment digit being displayed
ISR(TIMER2_OVF_vect)
{
  //Incrementing software counter
  counter++;
  if(counter > 976)
  {//Clk divider brings frequency down from 1MHz to 15,625Hz (8 bit counter)
    // Disable refresh timer
    TIMSK0 = 0b00000000;
    counter = 0;
    mode = char(set_mode);
  }
}
