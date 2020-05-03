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

#include <linux/string.h>
MODULE_LICENSE("GPL");
#define BUFF_SIZE 128

char buffer[BUFF_SIZE] = {};

ssize_t prname_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  if (*offp > 0) {
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

ssize_t prname_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  unsigned long parsed = 0;
  int result;
  char k_buff[BUFF_SIZE] = {};

  result = copy_from_user(k_buff, buff, count);
  if (!result) {
    return -EFAULT;
  }

  result = kstrtoul(k_buff, 10, &parsed);
  if (result) {
    return result;
  }

  if (parsed == 0) {
    return -EINVAL;
  }

  struct pid *pid_struct;
  struct task_struct *task;

  pid_struct = find_get_pid(parsed);

  if (!pid_struct) {
    strcpy(buffer, "none\n");
    return count;
  }

  task = pid_task(pid_struct, PIDTYPE_PID);

  if (!task) {
    strcpy(buffer, "none\n");
    return count;
  }

  strcpy(buffer, task->comm);

  size_t end = strlen(buffer);
  buffer[end] = '\n';
  buffer[end + 1] = '\0';

  return count;
}

ssize_t jiffies_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{

  if (*offp > 0) {
    return 0;
  }

  char k_buff[BUFF_SIZE] = {};

  sprintf(k_buff, "%lu\n", jiffies);

  size_t copy_size = strlen(k_buff);

  if (copy_to_user(buff, k_buff, copy_size)) {
    printk(KERN_INFO "[JIFFIES] Copying to userspace has failed at %lld!", *offp);
    return -EFAULT;
  }

  *offp += copy_size;

  return copy_size;
}

struct file_operations prname_fops = {
  read : prname_read,
  write : prname_write
};

struct file_operations jiffies_fops = {
  read : jiffies_read
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
  int res = 0;

  res = misc_register(&jiffies_device);

  if (res) {
    printk("[ADVANCED] Could not create jiffies character device!");
    return res;
  }

  res = misc_register(&prname_device);

  if (res) {
    misc_deregister(&jiffies_device);
    printk("[ADVANCED] Could not create prname character device!");
    return res;
  }

  printk("[ADVANCED] Module has been initialized!");

  return 0;
}

static void __exit reverse_exit(void)
{
  misc_deregister(&jiffies_device);
  misc_deregister(&prname_device);
  printk("[ADVANCED] Module has been removed!");
}

module_init(reverse_init);
module_exit(reverse_exit);