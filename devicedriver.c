#include <linux/module.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include "gpio_api.h"

#define NOD_NAME "deviceFile"


MODULE_LICENSE("GPL");

static int NOD_MAJOR;
static struct class *cls;
static dev_t dev;


static int deviceFile_open(struct inode *inode, struct file *filp){
    pr_info("Open Device\n");
    return 0;
}

static int deviceFile_release(struct inode *inode, struct file *filp){
    pr_info("Close Device\n");
    return 0;
}

static ssize_t deviceFile_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    switch(cmd){
        case _IO(0,3):
                gpio_output(20, "HIGH");
            break;
        case _IO(0,4):
            gpio_output(20, "LOW");
            break;
            
        //초기 세팅
        case _IO(0,5):
            gpio_pinMode(20, "OUTPUT");
            gpio_pinMode(21, "OUTPUT");
            gpio_pinMode(2, "INPUT");
            gpio_pinMode(3, "INPUT");
            break;
        //app의 요청을 통해 메모리를 파싱하여 변홙되었을때 led를 on/off함 
        case _IO(0,6):
            if(gpio_input(2) == 0){
                gpio_output(21, "HIGH");
                gpio_output(20, "LOW");
            }
            else if(gpio_input(3) == 0){
                gpio_output(20, "HIGH");
                gpio_output(21, "LOW");
            }
            else{
              break;
            }
        default :
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = deviceFile_open,
    .release = deviceFile_release,
    .unlocked_ioctl = deviceFile_ioctl,
};

//chmod 666 setting
char *node_devnode(const struct device *devc, umode_t *mode) {
    if (!mode)
        return NULL;
    if (devc->devt == dev)
        *mode = 0666;
    return NULL;
}

static int __init deviceFile_init(void){
    
    NOD_MAJOR = register_chrdev(NOD_MAJOR, NOD_NAME, &fops);
    if( NOD_MAJOR < 0 ){
        pr_alert("Register File\n");
        return NOD_MAJOR;
    }
    PERIBase = (uint32_t*) ioremap(0x1f00000000, 64*1024*1024);
    IOBank0 = PERIBase + 0xd0000/4;
    PADBank0 = PERIBase + 0xf0000/4;
    volatile uint32_t *GPIO_CTRL;
    GPIO_CTRL = IOBank0 + gpio_offset[20]/4;
    
    pr_info("Insmod Module\n");

    dev = MKDEV(NOD_MAJOR, 0);
    cls = class_create(NOD_NAME);
        //chmod outo
        cls->devnode = node_devnode;
    device_create(cls, NULL, dev, NULL, NOD_NAME);

    pr_info("Major number %d\n", NOD_MAJOR);
    pr_info("Device file : /dev/%s\n", NOD_NAME);
    
    pr_info("FUNCSEL default: %x\n",  (*GPIO_CTRL) & 0x1f);
    
    pr_info("F_M default: %x\n",  (*GPIO_CTRL >> 5) & 0x3f);
    
    return 0;
}

static void __exit deviceFile_exit(void)
{
    
    device_destroy(cls, dev);
    class_destroy(cls);
    unregister_chrdev(NOD_MAJOR, NOD_NAME);
    pr_info("Unload Module\n");
}

module_init(deviceFile_init);
module_exit(deviceFile_exit);
