#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>

using namespace std;

// Physical base address of FPGA Devices
const unsigned int LW_BRIDGE_BASE = 0xFF200000; // Base offset
// Length of memory-mapped IO window
const unsigned int LW_BRIDGE_SPAN = 0x00005000; // Address map size
// Cyclone V FPGA device addresses
const unsigned int LEDR_BASE = 0x00000000; // Leds offset
const unsigned int SW_BASE = 0x00000040; // Switches offset
const unsigned int KEY_BASE = 0x00000050; // Push buttons offset

/**
* Initialize general-purpose I/O
* - Opens access to physical memory /dev/mem
* - Maps memory into virtual address space
*
* @param fd File descriptor passed by reference, where the result
* of function 'open' will be stored.
* @return Address to virtual memory which is mapped to physical,
* or MAP_FAILED on error.
*/
char *Initialize(int *fd)
{
    // Open /dev/mem to give access to physical addresses
    *fd = open( "/dev/mem", (O_RDWR | O_SYNC));
    if (*fd == -1) // check for errors in openning /dev/mem
    {
        cout << "ERROR: could not open /dev/mem..." << endl;
        exit(1);
    }

    // Get a mapping from physical addresses to virtual addresses
    char *virtual_base = (char *)mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE),
    MAP_SHARED, *fd, LW_BRIDGE_BASE);
    if (virtual_base == MAP_FAILED) // check for errors
    {
        cout << "ERROR: mmap() failed..." << endl;
        close (*fd); // close memory before exiting
        exit(1); // Returns 1 to the operating system;
    }
    return virtual_base;
}

/**
* Close general-purpose I/O.
*
* @param pBase Virtual address where I/O was mapped.
* @param fd File descriptor previously returned by 'open'.
*/
void Finalize(char *pBase, int fd)
{
    if (munmap (pBase, LW_BRIDGE_SPAN) != 0)
{
    cout << "ERROR: munmap() failed..." << endl;
    exit(1);
}
    close (fd); // close memory
}

/**
* Write a 4-byte value at the specified general-purpose I/O location.
*
* @param pBase Base address returned by 'mmap'.
* @parem offset Offset where device is mapped.
* @param value Value to be written.
*/
void RegisterWrite(char *pBase, unsigned int reg_offset, int value)
{
    * (volatile unsigned int *)(pBase + reg_offset) = value;
}
/**
* Read a 4-byte value from the specified general-purpose I/O location.
*
* @param pBase Base address returned by 'mmap'.
* @param offset Offset where device is mapped.
* @return Value read.
*/
int RegisterRead(char *pBase, unsigned int reg_offset)
{
    return * (volatile unsigned int *)(pBase + reg_offset);
}

/**
 * Read1Switch reads the value of a given switch
 * 
 * @param   pBase       Base address returned by 'mmap'
 * @param   switchNum   The switch who's value should be read
 * @return              The value at the given switch
 */
int Read1Switch(char *pBase, int switchNum) {
    int value = RegisterRead(pBase, SW_BASE);       // Take in the value at the switch address
    value = value & 0x003F;                         // Clear all unnecesary values
    value = value >> switchNum;                     // Shift the bit representing the given switch to the lsb
    value = value & 0x0001;                         // Clear all bits greater than the lsb
    return value;                                   // Return the value of the given switch
}

/**
 * Write1Led writes a value to a singular LED
 * @param   pBase       Base address returned by 'mmap'
 * @param   ledNum      The LED who's value should be changed
 * @param   state       The state to change the LED to
 */
void Write1Led(char *pBase, int ledNum, int state){
    int value = RegisterRead(pBase, LEDR_BASE);     // Take in the current value at the LED address
    int bitToChange = state << ledNum;              // shift the state to the bit representing the appropriate LED

    // If the state is 1, bitwise OR the bit to change with the rest of the value to write the singular LED without changing the other values
    // If the state is 0, bitwise AND the bit to change with the rest of the value to write the singular LED without changing the other values
    if (state) {
        value = value | bitToChange;
    } else {
        value = value & bitToChange;
    }

    RegisterWrite(pBase, LEDR_BASE, value);         // Write the new value to the LED address
}

/*
* Write a value to all LEDs
*/
void WriteAllLeds(char *pBase, int value)
{
    RegisterWrite(pBase, LEDR_BASE, value);
}

/** Reads all the switches and returns their value in a single integer.
*
* @param    pBase   Base address for general-purpose I/O
* @return           A value that represents the value of the switches
*/
int ReadAllSwitches(char *pBase) {
    int value = RegisterRead(pBase, SW_BASE);   // Take in the current value at the switches address
    return value;                               // Return that value
}

/**
 * PushButtonGet reads the push buttons and returns a value depending on which push button is pressed
 * 
 * @param    pBase  Base address for general-purpose I/O
 * @return          A value indicating the button pressed, if no switches are pressed, or if two or more switches are pressed        
 */
int PushButtonGet(char *pBase) {
    int value = RegisterRead(pBase, KEY_BASE); // Get the current value at the push button address
    
    // Check the current value at the address and return a different number depending on the buttons being pressed
    switch (value)
    {
    case 0x0000:
        return -1;  // Return a -1 if no button is being pressed
    case 0x0001:
        return 0;   // Return a 0 if button 0 is being pressed
    case 0x0002:
        return 1;   // Return a 1 if button 1 is being pressed
    case 0x0004:
        return 2;   // Return a 2 if button 2 is being pressed
    case 0x0008:
        return 3;   // Return a 3 if button 3 is being pressed
    default:
        return 4;   // return a 4 if 2 or more buttons are being pressed
    }
}

/* Main Function */
int main()
{
    // Initialize
    int fd;
    char *pBase = Initialize(&fd);

    int counter = 0;                // Counter to be used in program
    int pushButtonState;            // Integer of the value returned by 
    int pushButtonPreviousState;    // Integer to save the previous state of the push button

    // Loop to test the program
    while (true) {
      pushButtonState = PushButtonGet(pBase);   // Get the current state of the push buttons
          
        // Check that a button is being pushed and a button also isn't being held
        if (pushButtonState != -1 && pushButtonState != pushButtonPreviousState) {
            pushButtonPreviousState = pushButtonState;  // Update the previous state with the new state if the button isn't being held
            
            // Check the state of the push buttons
            switch (pushButtonState)
            {
            case 0:
                counter++;                              // Increment the counter if button 0 is being pressed
                break;
            case 1:
                counter--;                              // Decrement the counter if button 1 is being pressed
                break;
            case 2:
                counter = counter >> 1;                 // Shift the counter left 1 bit if button 2 is being pressed
                break;
            case 3:
                counter = counter << 1;                 // Shift the counter right 1 bit if button 3 is being pressed
                break;
            case 4:
                counter = ReadAllSwitches(pBase);       // Set the counter to the value of the switches if more than 2 buttons are being pressed
                break;
            }
            
        } else {
            // Continously set the push previous state as the current state if a button is being held
            pushButtonPreviousState = pushButtonState;
        }

        // Reset the counter if it rolls over or if it falls below 0
        if (counter > 1023 || counter < 0) {
            counter = 0;
        }
        
        // Display the counter on the LEDs
        WriteAllLeds(pBase, counter);
    }

    // Done
    Finalize(pBase, fd);
}