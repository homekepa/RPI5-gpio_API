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
