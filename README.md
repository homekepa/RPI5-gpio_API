# RPI5 gpio_API
 RPI5에서 system call과 Device Driver을 이용해 직접 gpio를 제어하는 API입니다. 

미들웨어 개발자는 low level API를 app 개발자가 사용할 수 있도록 wrapping 함수로 만든다

- gpio_api.c 를 만든다.
- 비트 연산 없이 개발할 수 있도록 API를 구성한다.
- 필요한 API를 구성하고 매개변수를 정한다.
- error 를 검출할 수 있도록 return 값도 정한다. (안함...)
- ex) 아래는 API 예시이다.
    - gpio_pinMode(20, OUTPUT);
    - gpio_output(20, HIGH);
- 코드
    
    ### gpio_api.h
    
    ```c
    #ifndef GPIO_API_H
    #define GPIO_API_H
    
    volatile uint32_t *PERIBase;
    volatile uint32_t *IOBank0;
    volatile uint32_t *PADBank0;
    
    // GPIO_CTRL offset
    volatile uint32_t gpio_offset[] = {0x04, 0x0c, 0x14, 0x1c,0x24,0x2c,0x34,0x3c,0x44,0x4c,0x54,0x5c,0x64,0x6c,0x74,0x7c,0x84,0x8c,0x94,0x9c,0xa4,0xac,0xb4,0xbc,0xc4,0xcc,0xd4,0xdc};
    
    // PGIO_STATUS offset
    volatile uint32_t gpio_state[] = {0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xc0, 0xc8, 0xd0, 0xd8};
    
    // PADS_BANK0 offset
    volatile uint32_t gpio_pads[] = {0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x64,0x68,0x6c,0x70};
    
    int gpio_pinMode(int gpio_pin, char* mode);
    void gpio_output(int gpio_pin, char* mode);
    int gpio_input(int gpio_pin); //리턴 값은 메모리 값을 참조한 데이터 status값을 리턴함
    
    #endif
    
    ```
    
    RPI5의 GPIO pin의 CTRL, STATUS,PADS_BANK0 주소 offset과 gpio_api.c의 함수를 선언한 헤더 파일이다.
    
    ### gpio_api.c
    
    ```c
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
        pr_info("gpio module loaded\n");
        return 0;
    }
    
    // rmmod
    static void __exit gpio_exit(void) {
        gpio_output(20, "LOW");
        gpio_output(21, "LOW");
        iounmap(PERIBase);
        pr_info("gpio module unloaded\n");
    }
    
    module_init(gpio_init);
    module_exit(gpio_exit);
    
    ```
    
    직접 메모리에 접근하여 제어할수 있도록 비트연산을 하는 파일이다.
    
    **`int gpio_pinMode(int gpio_pin, char* mode)`** : 핀모드를 지정하는 함수
    
    mode의 인자로 INPUT, OUTPUT을 받으며 출력핀인지 입력핀인지 결정한다. 
    
    gpio_pin = 핀 번호
    
    **`void gpio_output(int gpio_pin, char* mode)`** :  핀의 출력상태를 지정하는 함수
    
    mode의 인자로 HIGH, LOW를 받아오며, 핀에 HIGH, LOW상태를 입력한다.
    
    **`int gpio_input(int gpio_pin);` :** 핀의 입력을 받은 함수
    
    핀 번호를 인자로 받으며, 핀의 입력 상태를 감지한다.
    
    ### devicedriver.c
    
    ```c
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
    
    ```
    
    app과 Kernel을 연결하기위한 divicedriver이다
    
    app으로부터 들어온 ioctl를 통해 분류하여 kernel에 적재된  deviceFile에 접근하여 함수를 실행한다.
    
    (_IO(0,1), _IO(0,2)는 시스템에서 사용하고 있으므로 사용할수없다)
    
    _IO(0,3) : 20번 핀 상태를 1(high)로 만든다.
    
    _IO(0,4) : 20번 핀 상태를 0(low)로 만든다.
    
    _IO(0,5) : 20번 핀과 21번 핀을 출력핀으로 설정한다, 2,3 번 핀은  입력핀으로 설정한다.
    
    _IO(0,6) : 20번 핀과 21번 핀을 반대로 토글처럼 켜고 끌수 있다.
    
    ### app.c
    
    ```c
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    
    #define NOD_NAME "/dev/deviceFile"
    
    int main(){
        int fd = open(NOD_NAME, O_RDWR);
        if( fd<0 ){
            printf("ERROR\n");
            exit(1);
        }
    
            int num;
        while(1){
                    printf(">>");
                    scanf("%d", &num);
    
                    int ret = ioctl(fd, _IO(0,num), 0);
                    if( ret<0 ){
                            printf("command invalid!\n");
                            break;
                    }
            }
    
        close(fd);
        return 0;
    }
    
    ```
    
    app을 통해 입력을 받아 ioctl로 deviceFile에 전달한다.
    
    num 입력 범위(3,4,5,6)

    ---
    ### Makefile
    
    ```c
    KERNEL_HEADERS=/lib/modules/$(shell uname -r)/build
    CC = gcc
    
    TARGET := app
    obj-m += gpio_api.o devicedriver.o
    
    PWD := $(CURDIR)
    
    #make 시 make all 실행
    all: driver app
    	
    
    #driver build
    driver:
    	make -C $(KERNEL_HEADERS) M=$(PWD) modules
    
    #app build
    app:
    	$(CC) -o $@ $@.c
    
    #driver, app 모두 제거
    clean:
    	make -C $(KERNEL_HEADERS) M=$(PWD) clean
    	rm -f *.o $(TARGET)
    
    ```
    
    위 4개의 파일을 컴파일하는 Makefile이다.

  위에 컴파일후 devicedriver를 적재해야합니다.
  ```
  sudo insmode devicedriver.ko
  ```
  파일 접근 허용을 해야합니다.
  devicederiver.c에서 접근권한을 666으로 정의 설정해두었습니다.
  만일 접근권한이 필요한경우
  ```
  sudo chmod 666 /dev/deviceFile
  ```
  접근 권한은 원하는 방식으로 부여하시면 됩니다

  수정 혹은 디바이스 파일을 제거하려면
  ```
  sudo rmmod devicedriver
  ```

  ./app을 실행하시면 됩니다.
  
