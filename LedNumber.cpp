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
/*
* Write a value to all LEDs
*/
int Read1Switch(char *pBase, int switchNum) {
    int value = RegisterRead(pBase, SW_BASE);
    value = value & 0x003F;
    value = value >> switchNum;
    value = value & 0x0001;
    return value;
}

void Write1Led(char *pBase, int ledNum, int state){
    int value = RegisterRead(pBase, LEDR_BASE);
    int bitToChange = state;
    bitToChange = value << ledNum;

    if (state) {
        value = value | bitToChange;
    } else {
        value = value & bitToChange;
    }

    RegisterWrite(pBase, LEDR_BASE, value);
}

void WriteAllLeds(char *pBase, int value)
{
    RegisterWrite(pBase, LEDR_BASE, value);
}

/* Main Function */
int main()
{
    // Initialize
    int fd;
    char *pBase = Initialize(&fd);

    // Sample test program
    int value = 0;
    cout << "Enter an int value between 0 to 1023: " << endl;
    cin >> value;
    cout << "value to be written to LEDs = " << value << endl;
    WriteAllLeds(pBase, value);

    int readLEDs = RegisterRead(pBase, LEDR_BASE);

    int switchNum;
    int ledChange;
    int state1;

    cout << "value of LEDS read = " << readLEDs << endl;
    cout << "Pick which switch you want to read the state of:";
    cin >> switchNum;
    cout << Read1Switch(pBase, switchNum) <<  endl;

    cout << "What LED do you want to change:";
    cin >> ledChange;
    cout << "what state do you want your led (0 or 1)";
    cin >> state1;
    Write1Led(pBase, ledChange, state1);

    // Done
    Finalize(pBase, fd);
}