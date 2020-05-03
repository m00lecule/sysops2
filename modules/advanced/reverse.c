#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <linux/pid.h>

#define MAX(x, y) (((x) > (y))? (x) : (y) )
#define MIN(x, y) (((x) < (y))? (x) : (y) )

#include <linux/string.h>
MODULE_LICENSE("GPL");
#define BUFF_SIZE 128

char buffer[BUFF_SIZE] = {};
static size_t DEVICE_SIZE = 40;
static char* mydata = NULL;
static unsigned int index = 0;


void myprint(void){
  int i = 0 ;

  while(i < DEVICE_SIZE){
    printk("%d %d %c",i,(int)mydata[i],mydata[i]);
    i++;
  }
  printk("\n");
}

int memalloc(size_t n_size){
    if(mydata == NULL){
                DEVICE_SIZE = MAX(DEVICE_SIZE, n_size);
        mydata = (char*) kmalloc(DEVICE_SIZE, GFP_KERNEL);

        if(!mydata){
          return -ENOMEM;
        }
            memset(mydata, 0, DEVICE_SIZE);

    }else{
        if(n_size + 1 == DEVICE_SIZE)
          return 0;
        
                  mydata = (char*) krealloc(mydata, n_size + 1, GFP_KERNEL);
        if(!mydata){
          return -ENOMEM;
        }

        if( n_size + 1> DEVICE_SIZE ){
          memset(mydata + DEVICE_SIZE , 0 , (n_size + 1)- DEVICE_SIZE );
          DEVICE_SIZE = n_size + 1;
        }else{
          mydata[n_size - 1] = '\n';
          mydata[n_size ] = '\0';
          index = n_size;
          DEVICE_SIZE = n_size + 1;
        }
        myprint();
    }

    return 0;
}

ssize_t prname_read(struct file *filep,char *buff,size_t count,loff_t *offp )
{
 
    if(*offp > 0){
        return 0;
    }

    size_t len = strlen(buffer);

    if (copy_to_user(buff, buffer, len)) {
        printk(KERN_INFO "[CIRCULAR] Copying to userspace has failed at %lld!", *offp);
        return -EFAULT;
    }

    *offp += len;

    return len;
}



ssize_t prname_write(struct file *filep,const char *buff,size_t count,loff_t *offp )
{

unsigned long parsed = 0;
 int result;
  char kbuff[BUFF_SIZE] = {};



    result = copy_from_user(kbuff, buff, count);
  if(result != 0) {
    printk(KERN_INFO "circular_proc_write: error copying from user-space\n");
    return -EFAULT;
  }

  result = kstrtoul(kbuff, 10, &parsed);
  if(result != 0) {
    printk(KERN_INFO "circular_proc_write: parsing error\n");
    return result;
  }

  if(parsed == 0) {
    printk(KERN_INFO "circular_proc_write: 0 is not allowed\n");
    return -EINVAL;
  }

      struct pid *pid_struct;
struct task_struct *task;


  pid_struct = find_get_pid(parsed);

  if(pid_struct == NULL){
    strcpy(buffer,"none");
    return count;
  }

  task = pid_task(pid_struct,PIDTYPE_PID);

  if(task == NULL){
    strcpy(buffer,"none");
    return count;
  }

  strcpy(buffer, task->comm);

  size_t end = strlen(buffer);
  buffer[end] = '\n';
  buffer[end + 1] = '\0';

  return count;
}

ssize_t jiffies_read(struct file *filep,char *buff,size_t count,loff_t *offp )
{
 
    if(*offp > 0){
        return 0;
    }

    char kernelbuff[BUFF_SIZE] = {};

    sprintf(kernelbuff, "%lu\n", jiffies);

    size_t copy_size = strlen(kernelbuff); 

    if (copy_to_user(buff, kernelbuff, copy_size)) {
        printk(KERN_INFO "[CIRCULAR] Copying to userspace has failed at %lld!", *offp);
        return -EFAULT;
    }

    *offp += copy_size;

    return copy_size;
}


ssize_t reverse_read(struct file *filep,char *buff,size_t count,loff_t *offp )
{
 
    if(*offp >= index){
        return 0;
    }

    size_t copy_size = ((count > index) ? index : count); 

    if (copy_to_user(buff, &mydata[*offp], copy_size)) {
        printk(KERN_INFO "[CIRCULAR] Copying to userspace has failed at %lld!", *offp);
        return -EFAULT;
    }

    *offp += copy_size;

    return copy_size;
}

ssize_t reverse_write(struct file *filep,const char *buff,size_t count,loff_t *offp )
{
  int result;
  char kbuff[BUFF_SIZE] = {};
  ssize_t bytes_read = 0;
ssize_t read = 0;
    ssize_t iter = 0;

    printk("i want to write %ld", count );


    while(bytes_read < count){
      read = ((count - bytes_read > BUFF_SIZE) ? BUFF_SIZE : count - bytes_read );

    result = copy_from_user(kbuff, buff + bytes_read, read);
    if(result != 0) {
        printk(KERN_INFO "[CIRCULAR] Copying from userspace has failed!");
        return -EFAULT;
    }
    iter = 0;
    while(iter < read){
        mydata[index] = kbuff[iter];
        index = (index + 1 ) % DEVICE_SIZE;
        iter++;
    }  
    
        bytes_read += read;
    }

    mydata[index] = '\0';

            myprint();


  return count;
}

struct file_operations reverse_fops = {
    read: reverse_read,
    write: reverse_write
};

struct file_operations prname_fops = {
    read: prname_read,
    write: prname_write
};

struct file_operations jiffies_fops = {
    read: jiffies_read
};

static struct miscdevice reverse_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "reverse",
    .fops = &reverse_fops
};

static struct miscdevice jiffies_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "jiffies",
    .fops = &jiffies_fops
};

static struct miscdevice prname_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "prname",
    .fops = &prname_fops
};

static int __init reverse_init(void)
{

    memalloc(DEVICE_SIZE);
    misc_register(&reverse_misc_device);
    misc_register(&jiffies_device);
    misc_register(&prname_device);

    return 0;
}

static void __exit reverse_exit(void)
{
    misc_deregister(&reverse_misc_device);
    misc_deregister(&jiffies_device);
    misc_deregister(&prname_device);

    if(mydata != NULL){
        kfree(mydata);
    }
}

module_init(reverse_init);
module_exit(reverse_exit);