#ifndef RPMSG_H
#define RPMSG_H

#include "motion_control_lib.h"

typedef struct{
    uint8_t type;  //操作类型 读 5  写 6   配置pdo 7
    uint16_t baseIndex; //基地址
    uint16_t offectIndex; //偏移地址
    uint8_t len;//数据长度
    double data[25]; //数据内容
    int32_t returnValue; //操作返回值

}rpmsg_single_frame_t;

typedef struct{
    uint8_t type;  //操作类型 读 5  写 6
    uint16_t baseIndex; //基地址
    uint16_t offectIndex; //偏移地址
    uint8_t len;//数据长度
    int32_t data[60]; //数据内容
}rpmsg_protocol_frame_t;



union rpmsg_single_frame{
    char inf[sizeof(rpmsg_single_frame_t)];
    rpmsg_single_frame_t frame;
};

union rpmsg_protocol_frame{
    char inf[sizeof(rpmsg_protocol_frame_t)];
    rpmsg_protocol_frame_t block;
};
int32_t rpmsg_init();
void rpmsg_close();
int32_t rpmsg_clear();
int32_t rpmsg_write(const char *app_buf,size_t len);
int rpmsg_read(char *data, size_t len);
#endif // RPMSG_H
