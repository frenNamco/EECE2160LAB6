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

        void Write1Led(DE1SoCfpga& fpga, int ledNum, int state){
            int value = fpga.RegisterRead(LEDR_BASE);
            int bitToChange = state << ledNum;

            if (state) {
                value = value | bitToChange;
            } else {
                value = value & bitToChange;
            }

            fpga.RegisterWrite(LEDR_BASE, value);
        }

        int Read1Switch(DE1SoCfpga& fpga, int switchNum) {
            int value = fpga.RegisterRead(SW_BASE);
            value = value & 0x003F;
            value = value >> switchNum;
            value = value & 0x0001;
            return value;
        }

        void WriteAllLeds(DE1SoCfpga& fpga, int value) {
            fpga.RegisterWrite(LEDR_BASE, value);
        }

        int readAllSwitches(DE1SoCfpga& fpga) {
            int value = fpga.RegisterRead(SW_BASE);

            return value;

            WriteAllLeds(fpga, value);
        }

        int pushButtonGet(DE1SoCfpga& fpga) {
            int value = fpga.RegisterRead(KEY_BASE);
            
            switch (value)
            {
            case 0x0000:
                return -1;
            case 0x0001:
                return 0;
                break;
            case 0x0002:
                return 1;
            case 0x0004:
                return 2;
            case 0x0008:
                return 3;
            default:
                return 4;
            }
        }
};




/* Main Function */
int main() {
    DE1SoCfpga fpga;
    LEDControl ledControl;

    // Sample test program
    int value = 0;
    cout << "Enter an int value between 0 to 1023: " << endl;
    cin >> value;
    cout << "value to be written to LEDs = " << value << endl;
    ledControl.WriteAllLeds(fpga, value);

    int readLEDs = fpga.RegisterRead(LEDR_BASE);

    int switchNum;
    int ledChange;
    int state1;
    int switchState;

    cout << "value of LEDS read = " << readLEDs << endl;
    cout << "Pick which switch you want to read the state of:";
    cin >> switchNum;
    cout << ledControl.Read1Switch(fpga, switchNum) <<  endl;

    cout << "What LED do you want to change:";
    cin >> ledChange;
    cout << "what state do you want your led (0 or 1)";
    cin >> state1;
    ledControl.Write1Led(fpga, ledChange, state1);

    while (true) {
        switchState = ledControl.readAllSwitches(fpga);
    }

    // int counter = 0;
    // int pushButtonState;

    // while (true) {

    //     pushButtonState =ledControl.pushButtonGet(fpga);
        
    //     if (pushButtonState != -1) {
    //         switch (pushButtonState)
    //         {
    //         case 0:
    //             counter++;
    //             break;
    //         case 1:
    //             counter--;
    //             break;
    //         case 2:
    //             counter = counter >> 1;
    //             break;
    //         case 3:
    //             counter = counter << 1;
    //             break;
    //         case 4:
    //             counter = 0;
    //         }
    //     }

    //     sleep(0.5);

    //     cout << counter << endl;

    //     if (counter > 1023 || counter < 0) {
    //         counter = 0;
    //     }
    // }

}