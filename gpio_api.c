#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include "gpio_api.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("WANS");


// 입력 데이터 메모리 파싱을 통해 gpio_pin 인자를 통해 파싱하여 상태 확인
int gpio_input(int gpio_pin){
    volatile uint32_t *BTN;
    BTN = IOBank0 + gpio_state[gpio_pin]/4;

    return (*BTN >> 23 ) & 0x1;
}

EXPORT_SYMBOL(gpio_input);


// 출력설정 (HIGH or LOW 값을 통해 led를 on/off함)
void gpio_output(int gpio_pin, char* mode) {
    volatile uint32_t *GPIO_CTRL;
    GPIO_CTRL = IOBank0 + gpio_offset[gpio_pin]/4;
    //OUTOVER 13:12 2 bit CLR
    *GPIO_CTRL &= ~(0x3<<12);
    if(strcmp(mode, "HIGH") == 0){
        *GPIO_CTRL |= (0x3<<12);
        pr_info("LED ON\n");
    }
    else if(strcmp(mode, "LOW") == 0){
        *GPIO_CTRL |= (0x2<<12);
        pr_info("LED OFF\n");
    }
}

EXPORT_SYMBOL(gpio_output);


// 핀모드 설정 입, 출력을 설정한다.
int gpio_pinMode(int gpio_pin, char* mode){
    volatile uint32_t *GPIO_CTRL; 
    volatile uint32_t *PAD_GPIO;
    
    PAD_GPIO = PADBank0 + gpio_pads[gpio_pin]/4;
    GPIO_CTRL = IOBank0 + gpio_offset[gpio_pin]/4;
    
    //FUNSCEL 4:0 5bit CLR
    *GPIO_CTRL &= ~(0x1f);
    //FUNSCEL 4:0 5bit 0x7 SET
    *GPIO_CTRL |= (0x7);
    
    
    if(strcmp(mode, "INPUT") == 0){
        //GPIO2 INOVER 17:16 bit 0x0 SET ( default )
        *GPIO_CTRL |= (0x0 << 16);
        //PAD IE Input Enable 6 bit  0x1 SET
        *PAD_GPIO |= ( 0x1<<6 );
        pr_info("BTN Control!\n");
    }
    else if(strcmp(mode, "OUTPUT") == 0){
     //OEOVER output enable 15:14 bit 0x3 SET
        *GPIO_CTRL |= (0x3<<14);
        //PAD_GPIO20 7 bit CLR OUTPUT able
        *PAD_GPIO &= ~(0x1 << 7);
        //PAD_GPIO20 5:4 bit DRIVE 0x1 SET ( default )
        *PAD_GPIO |= (0x1<<4);
        
        pr_info("LED Access!\n");
    }
    return 0;
}

EXPORT_SYMBOL(gpio_pinMode);

// insmode
static int __init gpio_init(void) {
    PERIBase = (uint32_t*) ioremap(0x1f00000000, 64*1024*1024);
    IOBank0 = PERIBase + 0xd0000/4;
    PADBank0 = PERIBase + 0xf0000/4;
    pr_info("ssafy gpio module loaded\n");
    return 0;
}

// rmmod
static void __exit gpio_exit(void) {
    gpio_output(20, "LOW");
    gpio_output(21, "LOW");
    iounmap(PERIBase);
    pr_info("ssafy gpio module unloaded\n");
}

module_init(gpio_init);
module_exit(gpio_exit);
