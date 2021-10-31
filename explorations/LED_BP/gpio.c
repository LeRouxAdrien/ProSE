// #include <module.h>
// #include <timer.h>
// #include <gpio.h>
// #include <fs.h>

// // Sortie sur broche 18 (GPIO 24)
// #define RPI_GPIO_OUT 24

// // Entree sur broche 16 (GPIO 23)
// #define RPI_GPIO_IN 23

// #define O_LED 24          //link GPIO_24 and GND together
// #define R_LED 25          //link GPIO_25 and GND together
// #define G_LED 23          //link GPIO_23 and GND together
// #define POWER_BUTTON 22   //link GPIO_22 and GND together
// #define LIFTING_BUTTON 21 //link GPIO_21 and GND together
// static struct timer_list rpi_gpio_1_timer;

// static void rpi_gpio_1_function(unsigned long unused)
// {
//     /*static int value = 1;
//     value = 1 - value;
//     if (gpio_get_value(RPI_GPIO_IN) == 0)
//         value = 0;
//     gpio_set_value(RPI_GPIO_OUT, value);
//     mod_timer(&rpi_gpio_1_timer, jiffies + (HZ >> 3));*/
//     static int value = 1;
//     gpio_set_value(G_LED, value);
//     mod_timer(&rpi_gpio_1_timer, jiffies + (HZ >> 1));
//     gpio_set_value(G_LED, 0);
// }

// static int __init rpi_gpio_1_init(void)
// {
//     int err;

//     if ((err = gpio_request(G_LED, THIS_MODULE->name)) != 0) // optional hook for chip-specific activation, such as enabling module power and clock; may sleep
//         return err;
//     if ((err = gpio_request(O_LED, THIS_MODULE->name)) != 0)
//     {
//         gpio_free(G_LED);
//         return err;
//     }
//     if ((err = gpio_request(R_LED, THIS_MODULE->name)) != 0)
//     {
//         gpio_free(G_LED);
//         gpio_free(O_LED);
//         return err;
//     }
//     if ((err = gpio_request(POWER_BUTTON, THIS_MODULE->name)) != 0)
//     {
//         gpio_free(G_LED);
//         gpio_free(O_LED);
//         gpio_free(R_LED);
//         return err;
//     }
//     if ((err = gpio_request(LIFTING_BUTTON, THIS_MODULE->name)) != 0)
//     {
//         gpio_free(G_LED);
//         gpio_free(O_LED);
//         gpio_free(R_LED);
//         gpio_free(POWER_BUTTON);
//         return err;
//     }
//     if ((err = gpio_direction_input(POWER_BUTTON)) != 0)
//     { // configures signal “offset” as input, or returns error
//         gpio_free(O_LED);
//         gpio_free(R_LED);
//         gpio_free(G_LED);
//         gpio_free(POWER_BUTTON);
//         gpio_free(LIFTING_BUTTON);
//         return err;
//     }
//     if ((err = gpio_direction_input(LIFTING_BUTTON)) != 0)
//     { // configures signal “offset” as input, or returns error
//         gpio_free(O_LED);
//         gpio_free(R_LED);
//         gpio_free(G_LED);
//         gpio_free(POWER_BUTTON);
//         gpio_free(LIFTING_BUTTON);
//         return err;
//     }
//     if ((err = gpio_direction_output(O_LED, 1)) != 0)
//     { //configures signal “offset” as output, or returns error
//         gpio_free(O_LED);
//         gpio_free(R_LED);
//         gpio_free(G_LED);
//         gpio_free(POWER_BUTTON);
//         gpio_free(LIFTING_BUTTON);
//         return err;
//     }
//     if ((err = gpio_direction_output(R_LED, 1)) != 0)
//     { //configures signal “offset” as output, or returns error
//         gpio_free(O_LED);
//         gpio_free(R_LED);
//         gpio_free(G_LED);
//         gpio_free(POWER_BUTTON);
//         gpio_free(LIFTING_BUTTON);
//         return err;
//     }
//     if ((err = gpio_direction_output(G_LED, 1)) != 0)
//     { //configures signal “offset” as output, or returns error
//         gpio_free(O_LED);
//         gpio_free(R_LED);
//         gpio_free(G_LED);
//         gpio_free(POWER_BUTTON);
//         gpio_free(LIFTING_BUTTON);
//         return err;
//     }

//     init_timer(&rpi_gpio_1_timer);
//     rpi_gpio_1_timer.function = rpi_gpio_1_function;
//     rpi_gpio_1_timer.data = 0; // non utilise
//     rpi_gpio_1_timer.expires = jiffies + (HZ >> 1);
//     add_timer(&rpi_gpio_1_timer);

//     return 0;
// }

// static void __exit rpi_gpio_1_exit(void)
// {
//     del_timer(&rpi_gpio_1_timer);
//     gpio_free(O_LED);
//     gpio_free(R_LED);
//     gpio_free(G_LED);
//     gpio_free(POWER_BUTTON);
//     gpio_free(LIFTING_BUTTON);

//     module_init(rpi_gpio_1_init);
//     module_exit(rpi_gpio_1_exit);
//     //MODULE_LICENSE("GPL");
    
 
// /*
// Makefile
// ifneq (${KERNELRELEASE},)

//   obj-m += rpi-gpio-1.o

// else
//   ARCH          ?= arm
//   KERNEL_DIR ?= ~/linux-3.2.27
//   CROSS_COMPILE ?= /usr/local/cross/rpi/bin/arm-linux-
//   MODULE_DIR    := $(shell pwd)
//   CFLAGS        := -Wall

// all: modules

// modules:
//   ${MAKE} -C ${KERNEL_DIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} SUBDIRS=${MODULE_DIR} modules

// clean:
//   rm -f  *.o  .*.o  .*.o.* *.ko  .*.ko  *.mod.* .*.mod.* .*.cmd
//   rm -f Module.symvers Module.markers modules.order
//   rm -rf .tmp_versions
// endif
// */