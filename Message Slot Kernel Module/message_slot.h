#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

// The major device number.
#define MAJOR_NUM 235

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot_dev"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0

typedef struct channel{
    char msg[BUF_LEN];
    int len_msg;
    unsigned int id;
    struct channel *next;
} channel;

typedef struct channel_list{
    channel *head;
} channel_list;

typedef struct profile{
    unsigned int minor;
    channel *active_channel;
} profile;

#endif
