/*
 * Implementation of a simple counter to operate as either a stopwatch, timer or interval timer (rest and work periods)
 *  Default state is stopwatch
 * 
 *
 * Use of the shift regioster will allow us to save pins of the atmega for the connecting of the 7 segment display
 * For shift register operation:
 *  Connect pins 11 and 12 (SHCP, STCP) together, these are clocks for the 2 internal register blocks
 *  To reset the outputs pull pin 10 (MR) low and toggle the clocks, otherwise leave pin 10 (MR) high
 *  Can pull Pin 13 (OE) low after writing to register is completed to display the data
 *  Pin 14 (DS) is the data input
 *  
 * This is connected to the ATMEGA328p as follows:
 *  PIND0:  SHCP, STCP
 *  PIND1:  MR
 *  PIND2:  OE
 *  PIND3:  DS
 *
 *  Use push buttons to change states and to set timers (timer or interval timer)
 */
 
 // Imports of header files for the ATMEGA register mapping as well as the standard C libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void toggle_clk();
void update_shift_register_output(int *digits, int arr_size, int dp);
void reset_shift_register();
void timer2_setup();
void timer0_setup();
void pcint_setup();
void increment_digits();
void decrement_display(int digit);
void increment_display(int digit);
// void refresh_display();

// Digits displayed, initialised to 0
int dig0, dig1, dig2, dig3 = 0;

// Work and rest digits
int work0, work1, work2, work3 = 0;
int rest0, rest1, rest2, rest3 = 0;

// Keeping track of intervals
int num_intervals, set_intervals = 0;

// Keep track of which digit currently altering
int set_digit = 0;

// Counter for timing control
int counter = 0;

// Chars used for control of states
volatile char mode = 'a';
volatile char set_mode = 'i';

// Flags used for toggling opf digits in different states
volatile bool set_flag = true;
bool interval_flag, t0_ovflow_flag = false;

/*
 * Initialising the Timer0 Overflow counter setup
 * Used to refresh the display
*/ 
void timer0_setup()
{
  // Select Clock prescaler to /64
  TCCR0B = 0b00000100;
  // Enable Timer overflow counter
  TIMSK0 = 0b00000001;
}

/* 
 *  Initialising the Timer Overflow Counter 2
 *  Used to increment the display of the digits
 */
void timer2_setup()
{
  // Clock divider set to 64
  TCCR2B = 0b00000100;
  // Enable Timer overflow
  TIMSK2 = 0b00000001;
}

// Initialising the PCINT1 interrupt
// PCINT13 - 9 setup
void pcint_setup()
{
  // EICRA = 0b0000;
  // Selection of pins allowed to cause interrupt
  PCICR = (1 << PCIE1);
  //Enable PCINT to cause an interrupt
  PCMSK1 = (1 << PCINT13) | (1 << PCINT12) | (1 << PCINT11) | (1 << PCINT10) | (1 << PCINT9);
  PCIFR = 0b00000000;
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

// Increment the selected digit
void increment_display(int digit) {
  // Function to increment selected digit
  if (digit < 1) {
    if (dig0 < 9) {
      dig0++;
    }
    else {
      dig0 = 0;
    }
  }
  else if (digit < 2) {
    if (dig1 < 5) {
      dig1++;
    }
    else {
      dig1 = 0;
    }
  }
  else if (digit < 3) {
    if (dig2 < 9) {
      dig2++;
    }
    else {
      dig2 = 0;
    }
  }
  else {
    if (dig3 < 9) {
      dig3++;
    }
    else {
      dig3 = 0;
    }
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

//Function to decrement selected digit
void decrement_display(int digit) {
  if (digit < 1) {
    if (dig0 < 1) {
      dig0 = 9;
    }
    else {
      dig0--;
    }
  }
  else if (digit < 2) {
    if (dig1 < 1) {
      dig1 = 5;
    }
    else {
      dig1--;
    }
  }
  else if (digit < 3) {
    if (dig2 < 1) {
      dig2 = 9;
    }
    else {
      dig2--;
    }
  }
  else {
    if (dig3 < 1) {
      dig3 = 9;
    }
    else {
      dig3--;
    }
  }
}


/*
 * Toggle the clk Pin of the Shift Register
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
    // PINdig3=digits[i];
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
  // PINdig1=0;
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
      case 'b':
        break;
      case 'c':
        break;
      case 'd':
      // Decrement the display
        SREG &= 0b01111111;
        decrement_digits();
        mode = 'a';
        SREG |= 0b10000000;
        TIMSK0 = 0b00000001;
        break;
      case 'e':
        increment_display(set_digit);
        mode = 'a';
        break;
      case 'f':
        decrement_display(set_digit);
        mode = 'a';
        break; 
      case 'g':
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
      case 'n':
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
      case 'w':
        // Work interval
        if ( dig0 < 1 && dig1 < 1 && dig2 < 1 && dig3 < 1 ) {
          //When reached 00:00, set digits to resting values
          dig0 = rest0;
          dig1 = rest1;
          dig2 = rest2;
          dig3 = rest3;
          mode = 'z';
          set_mode = 'x';
        }
        else {
          mode = 'd';
        }
        break;
      case 'x':
        if ( dig0 < 1 && dig1 < 1 && dig2 < 1 && dig3 < 1 && num_intervals < set_intervals) {
          //When reached 00:00, set digits to work values, if num intervals has not been reached
          dig0 = work0;
          dig1 = work1;
          dig2 = work2;
          dig3 = work3;
          mode = 'y';
          set_mode = 'w';
        }
        else {
          mode = 'd';
        }
        break;
      case 'y':
        if(num_intervals > 0){
          num_intervals++;
          mode = 'a';
        }
        else{
          mode = 'a';
          set_mode = 'a';
        }
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

/*
 * Push button interrupts
 * No error detection of multiple buttons pushed at once
 */
ISR(PCINT1_vect)
{ //Interrupt routine for PCINT1
  
  // Switch 0
  if (PINC & 0b00000010) {
    if (set_mode == 105){
      // If already in the mode of stopwatch (i)
      set_mode = 'c'; // Empty state to set the timer
    }
    else if (set_mode == 99 ){
      // If in the timer state (c)
      // Start timer
      set_mode = 'd';
    }
    else if( set_mode == 100){
      // If timer is on, set to Interval
      set_mode = 'g';
    }
    else if (set_mode == 103) {
      // State to set the work time
      set_mode = 'b';
      
      // Saving the work digits set
      work0 = dig0;
      work1 = dig1;
      work2 = dig2;
      work3 = dig3;
      
      // Setting the displayed digits to 0
      dig0 = 0;
      dig1 = 0;
      dig2 = 0; 
      dig3 = 0;
      
      set_digit = 0;
    }
    else if (set_mode == 98) {
      set_mode = 'n';

      // Saving the digits as rest time
      rest0 = dig0;
      rest1 = dig1;
      rest2 = dig2;
      rest3 = dig3;

      // Setting the digits to 0
      dig0 = 0;
      dig1 = 0;
      dig2 = 0; 
      dig3 = 0;
      set_digit = 0;
      // PORTB = 0b00000000;
    }
    else if (set_mode == 110){
      //Saving the digits displayed as the number of intervals
      set_intervals = dig0 + (dig1 * 10) + (dig2 * 100) + (dig3 * 1000);
      
      // Stops SW3/4 moving digits
      set_flag = false;

      // Place the set_mode state in work
      set_mode = 'w';
      // Set the digits displayed to 
      dig0 = work0;
      dig1 = work1;
      dig2 = work2;
      dig3 = work3;
      // Set the number of intervals to set_intervals
      num_intervals = set_intervals;
      set_digit = 0;
    }
  }
  
  // Switch 1
  else if (PINC & 0b00000100) {
    //if
    if (set_digit > 0 ) {
      set_digit--;
    }
  }
  
  // Switch 2
  else if (PINC & 0b00001000) {
    if (set_digit < 3) {
      set_digit++;
    }
  }
  
  // Switch 3
  else if (PINC & 0b00010000 && set_flag == true)
  { // Increment digit displayed if in timer mode and not running
    mode = 'e';
  }
  // Switch 4
  else if (PINC & 0b00100000 && set_flag == true)
  { //If set to timer and not running, decrement display for starting point of timer
    mode = 'f';
  }
  // Reset PCINT flag
  PCIFR = 0b00000000;
}
