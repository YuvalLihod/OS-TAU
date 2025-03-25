//this is message_slot.c

// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h> //for GFP_KERNEL falg used in kmalloc() 
#include "message_slot.h"

MODULE_LICENSE("GPL");

static channel_list devices[257]; //message slots device files

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode* inode, struct file*  file){
    profile *myprof;
    int minor = iminor(inode);
    printk("Invoking device_open(%p)\n", file);
    myprof = kmalloc(sizeof(profile),GFP_KERNEL);
    if(myprof == NULL){
      printk("kmalloc failed in deivce_open(%p),minor = %d",file,minor);
      return -ENOMEM;
    }
    myprof->minor = minor;
    myprof->active_channel = NULL;
    file->private_data = (void*)myprof;
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  printk("Invoking device_release(%p,%p)\n", inode, file);
  kfree(file->private_data);
  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
  ssize_t i;
  char *msg;
  int len_msg;
  profile *myprof = (profile*)(file->private_data);
  printk( "Invocing device_read(%p,%ld)\n",file, length);
  
  if(myprof->active_channel == NULL || buffer ==NULL){
    return -EINVAL;
  }
  len_msg = myprof->active_channel->len_msg;
  if(len_msg == 0){
    return -EWOULDBLOCK;
  }
  if(len_msg > length){
    return -ENOSPC;
  }
  msg = myprof->active_channel->msg;
  for(i = 0;i < BUF_LEN &&  i< len_msg ; ++i ){
    if(put_user(msg[i], &buffer[i]) !=0 ){//we don't trust proccess 
       return -EINVAL;
    }
  }
  return i;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
  profile *myprof = (profile*)(file->private_data);
  char tmp[BUF_LEN];
  ssize_t i;
  
  printk("Invoking device_write(%p,%ld)\n", file, length);
  
  if(myprof->active_channel == NULL || buffer ==NULL){
    return -EINVAL;
  }
  if(length == 0 || length > BUF_LEN){
    return -EMSGSIZE;
  }

  for( i = 0; i < length; ++i ) {
    if(get_user(tmp[i], &buffer[i]) != 0){ //we don't trust proccess 
      return -EINVAL;
    }
  }
    for(i = 0; i<length ; ++i){
      myprof->active_channel->msg[i] = tmp[i];
    }
    myprof->active_channel->len_msg = length;
  // return the number of input characters used
  return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param ){
  // Switch according to the ioctl called
    channel *prev, *tmp;
    profile *myprof = (profile*)(file->private_data);
    channel *curr = devices[myprof->minor].head;
    
    printk("Invoking ioctl,channe_id =%lu ",ioctl_param);
    if( MSG_SLOT_CHANNEL != ioctl_command_id || ioctl_param == 0) {
        return -EINVAL;
    }
    
    if (curr == NULL){ //messege slot device file has no channels
      curr = kmalloc(sizeof(channel), GFP_KERNEL); //create requested channel
      if(curr == NULL){
        printk("kmalloc failed in deivce_ioctl");
        return -ENOMEM;
      }
      curr->len_msg = 0;
      curr->id = ioctl_param;
      curr->next = NULL;
      myprof->active_channel = curr;
      devices[myprof->minor].head = curr;
      return SUCCESS;
    }
    else{ //messege slot device file has channels, check if given id exist
      while(curr != NULL && curr->id <= ioctl_param){
        if(curr->id == ioctl_param){ //requested id found
          myprof->active_channel = curr;
          return SUCCESS;
        }
        prev = curr;
        curr = curr->next;
      }
      //channel doesn't exist, create one:
      tmp = kmalloc(sizeof(channel), GFP_KERNEL); //create requested channel
      if(tmp == NULL){
        printk("kmalloc failed in deivce_ioctl");
        return -ENOMEM;
      }
      tmp->id = ioctl_param;
      tmp->len_msg = 0;
      myprof->active_channel = tmp;
      if(prev->id > ioctl_param){//only if there is exactly 1 channel in the MSDF
        tmp->next = prev;
        devices[myprof->minor].head = tmp;
      }
      else{
        tmp->next = curr;
        prev->next = tmp;
      }
      return SUCCESS;
    }
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void){
    int rc = -1;
    int i;
  // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
    if( rc < 0 ) {
        printk( KERN_ERR "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
        return rc;
    }

    for(i = 0; i<257; i++){
        devices[i].head = NULL;
    }
    printk( "Registeration of %s is successful.", DEVICE_FILE_NAME);

  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device
  // Should always succeed
  channel *curr, *tmp;
  int i;
  for(i = 0; i<257; i++){
    curr = devices[i].head;
    while(curr != NULL){
      tmp = curr;
      curr = curr->next;
      kfree(curr);
    }
  }
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);