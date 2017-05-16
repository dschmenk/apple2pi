//
// Modified from:
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// Access from ARM Running Linux
#define ARMv6_PERI_BASE          0x20000000
#define ARMv7_PERI_BASE          0x3F000000
#define GPIO_OFFSET              0x00200000
#define CMGP_OFFSET              0x00101000

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_REG(reg) (gpio[(reg)/4])
#define CMGP_REG(reg) (cmgp[(reg)/4])

#define GPFSEL0		0x000
#define CM_GP0CTL 	0x070
#define CM_GP0DIV 	0x074

#define IOMAP_LEN	4096
//
// Set up a memory regions to access GPIO
//
volatile unsigned int *setup_io(int reg_base)
{
    int  mem_fd;
    void *io_map;

    // open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
    {
        printf("can't open /dev/mem \n");
        exit(-1);
    }
    // mmap IO
    io_map = mmap(NULL,             //Any adddress in our space will do
		  IOMAP_LEN,       //Map length
		  PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		  MAP_SHARED,       //Shared with other processes
		  mem_fd,           //File to map
		  reg_base);        //Offset to peripheral
    close(mem_fd); //No need to keep mem_fd open after mmap
    if (io_map == MAP_FAILED)
    {
        printf("mmap error %d\n", (int)io_map);//errno also set!
        exit(-1);
    }
    return (volatile unsigned *)io_map;
}

void release_io(volatile unsigned int *io_map)
{
    munmap((void *)io_map, IOMAP_LEN);
}

void gpclk(int idiv)
{
    // I/O access
    volatile unsigned *gpio, *cmgp;
    int g,rep;
    unsigned int arm_base = ARMv6_PERI_BASE; // Default to ARMv6 peripheral base

    FILE *cpuinfo;
    char rasstr[1024], pistr[128];
    int version;

    if ((cpuinfo = fopen("/proc/device-tree/model", "r") ) == NULL)
    {
        printf("can't open /proc/device-tree/model\n");
        exit(-1);
    }
    if (fscanf(cpuinfo, "%s %s %d", rasstr, pistr, &version) == 3)
    {
        if (strcmp(rasstr, "Raspberry") == 0 && strcmp(pistr, "Pi") == 0)
        {
            //printf("Found %s %s version %d\n", rasstr, pistr, version);
            switch (version)
            {
                case 0:
                case 1:
                    arm_base = ARMv6_PERI_BASE; // Pi version 1 (ARMv6)
                    break;
                case 2:
                case 3:
                    arm_base = ARMv7_PERI_BASE; // Pi version 2 and 3 (ARMv7)
                    break;
            }
        }
    }
    fclose(cpuinfo);
    // Set up gpi pointer for direct register access
    cmgp = setup_io(arm_base + CMGP_OFFSET);
    gpio = setup_io(arm_base + GPIO_OFFSET);

    // Set Clock Manager to 500 MHz source (PLLD)
    CMGP_REG(CM_GP0CTL) = (0x5A << 24) // Password
	                               // Disable
	                | (1);         // Src = oscillator
    usleep(1000);
    CMGP_REG(CM_GP0CTL) = (0x5A << 24) // Password
	                               // Disable
	                | (6);         // Src = PLLD
    CMGP_REG(CM_GP0DIV) = (0x5A << 24) // Password
                        | ((idiv) << 12); // IDIV
    usleep(1000);
    CMGP_REG(CM_GP0CTL) = (0x5A << 24) // Password
	                | (1 << 4)     // Enable
	                | (6);         // Src = PLLD
    usleep(1000);
    // Set GCLK function (ALT0) for GPIO 4 (header pin #7)
    INP_GPIO(4);
    SET_GPIO_ALT(4, 0x00);
    // Release I/O space
    release_io(gpio);
    release_io(cmgp);
}
#ifndef SETSERCLK
int main(void)
{
        /*
     * Initialize ACIA clock for Apple II Pi card
     */
    gpclk(271); /* divisor for ~1.8 MHz => (500/271) MHz */
}
#endif
