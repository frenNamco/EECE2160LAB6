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

class DE1SoCfpga {
    char* pBase;
    int fd;

    public:
        DE1SoCfpga () {
            // Open /dev/mem to give access to physical addresses
            fd = open( "/dev/mem", (O_RDWR | O_SYNC));
            if (fd == -1) // check for errors in openning /dev/mem
            {
                cout << "ERROR: could not open /dev/mem..." << endl;
                exit(1);
            }

            // Get a mapping from physical addresses to virtual addresses
            pBase = (char *)mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE),
            MAP_SHARED, fd, LW_BRIDGE_BASE);
            if (pBase == MAP_FAILED) // check for errors
            {
                cout << "ERROR: mmap() failed..." << endl;
                close (fd); // close memory before exiting
                exit(1); // Returns 1 to the operating system;
            }
        }   

        ~DE1SoCfpga () {
            if (munmap (pBase, LW_BRIDGE_SPAN) != 0)
            {       
            cout << "ERROR: munmap() failed..." << endl;
            exit(1);
            }
            close (fd); // close memory
        }

        void RegisterWrite(unsigned int offset, int value) {
            * (volatile unsigned int *)(pBase + offset) = value;
        }

        int RegisterRead(unsigned int offset) {
            return * (volatile unsigned int *)(pBase + offset);
        }
};

class LEDControl {
    public:
        LEDControl() {

        }

        ~LEDControl() {

        }

        /** WriteAllLeds writes a value to all LEDs
         * @param   fpga    The fpga object passed by reference
         * @param   value   The value to be written to the LEDs
         * @return          Doesn't return anything
        */
        void WriteAllLeds(DE1SoCfpga& fpga, int value) {
            fpga.RegisterWrite(LEDR_BASE, value);
        }

        /**
         * Write1Led writes a value to a singular LED
         * @param   fpga    The fpga object passed by reference
         * @param   ledNum      The LED who's value should be changed
         * @param   state       The state to change the LED to
         */
        void Write1Led(DE1SoCfpga& fpga, int ledNum, int state){
            int value = fpga.RegisterRead(LEDR_BASE);   // Take in the current value at the LED address
            int bitToChange = state << ledNum;          // shift the state to the bit representing the appropriate LED

            // If the state is 1, bitwise OR the bit to change with the rest of the value to write the singular LED without changing the other values
            // If the state is 0, bitwise AND the bit to change with the rest of the value to write the singular LED without changing the other values
            if (state) {
                value = value | bitToChange;
            } else {
                value = value & bitToChange;
            }

            fpga.RegisterWrite(LEDR_BASE, value);   // Write the new value to the LED address
        }

        /**
         * Read1Switch reads the value of a given switch
         * 
         * @param   fpga        The fpga object passed by reference
         * @param   switchNum   The switch who's value should be read
         * @return              The value at the given switch
         */
        int Read1Switch(DE1SoCfpga& fpga, int switchNum) {
            int value = fpga.RegisterRead(SW_BASE); // Take in the value at the switch address
            value = value & 0x003F;                 // Clear all unnecesary values
            value = value >> switchNum;             // Shift the bit representing the given switch to the lsb
            value = value & 0x0001;                 // Clear all bits greater than the lsb
            return value;                           // Return the value of the given switch
        }

        /** Reads all the switches and returns their value in a single integer.
        *
        * @param   fpga     The fpga object passed by reference
        * @return           A value that represents the value of the switches
        */
        int readAllSwitches(DE1SoCfpga& fpga) {
            int value = fpga.RegisterRead(SW_BASE);     // Take in the current value at the switches address
            return value;                               // Return that value
        }

        /**
         * PushButtonGet reads the push buttons and returns a value depending on which push button is pressed
         * 
         * @param    pBase  Base address for general-purpose I/O
         * @return          A value indicating the button pressed, if no switches are pressed, or if two or more switches are pressed        
         */
        int pushButtonGet(DE1SoCfpga& fpga) {
            int value = fpga.RegisterRead(KEY_BASE);    // Get the current value at the push button address
    
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
};




/* Main Function */
int main() {
    return 0;
}