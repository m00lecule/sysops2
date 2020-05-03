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
#include <linux/string.h>
#define BUFF_SIZE 128
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

MODULE_LICENSE("GPL");
static size_t DEVICE_SIZE = 40;
static char *buffer_data = NULL;
static unsigned int index = 0;

int memalloc(size_t n_size)
{
  if (buffer_data == NULL) {
    DEVICE_SIZE = MAX(DEVICE_SIZE, n_size);
    buffer_data = (char *)kmalloc(DEVICE_SIZE, GFP_KERNEL);

    if (!buffer_data) {
      return -ENOMEM;
    }
    memset(buffer_data, 0, DEVICE_SIZE);
    printk("[CIRCULAR] buffer of %lu has has been allocated", n_size);
  }
  else {
    if (n_size + 1 == DEVICE_SIZE){
      return 0;
    }
      

    buffer_data = (char *)krealloc(buffer_data, n_size + 1, GFP_KERNEL);
    if (!buffer_data) {
      return -ENOMEM;
    }

    printk("[CIRCULAR] buffers size has been changed to %lu", n_size + 1);

    if (n_size + 1 > DEVICE_SIZE) {
      memset(buffer_data + DEVICE_SIZE, 0, (n_size + 1) - DEVICE_SIZE);
      DEVICE_SIZE = n_size + 1;
    }
    else {
      buffer_data[n_size - 1] = '\n';
      buffer_data[n_size] = '\0';
      index = n_size;
      DEVICE_SIZE = n_size + 1;
    }
  }

  return 0;
}

ssize_t circular_proc_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  if (*offp > 0) {
    return 0;
  }

  char k_buff[BUFF_SIZE] = {};

  sprintf(k_buff, "%lu", DEVICE_SIZE);

  size_t copy_size = strlen(k_buff);

  if (copy_to_user(buff, k_buff, copy_size)) {
    printk(KERN_INFO "[CIRCULAR] Copying to userspace has failed at %lld!", *offp);
    return -EFAULT;
  }

  *offp += copy_size;

  return copy_size;
}

ssize_t circular_read(struct file *filep, char *buff, size_t count, loff_t *offp)
{
  if (*offp >= index) {
    return 0;
  }

  size_t copy_size = ((count > index) ? index : count);

  if (copy_to_user(buff, &buffer_data[*offp], copy_size)) {
    printk(KERN_INFO "[CIRCULAR] Copying to userspace has failed at %lld!", *offp);
    return -EFAULT;
  }

  *offp += copy_size;

  return copy_size;
}

ssize_t circular_proc_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  unsigned long parsed = 0;
  int result;
  char k_buff[BUFF_SIZE] = {};

  result = copy_from_user(k_buff, buff, count);
  if (result) {
    printk("[PROC-CIRCULAR] Copying from userspace has failed at %lld!", *offp);
    return -EFAULT;
  }

  result = kstrtoul(k_buff, 10, &parsed);
  if (result) {
    printk("[PROC-CIRCULAR] Provided data is not numeric!\n");
    return result;
  }

  if (parsed == 0) {
    printk("[PROC-CIRCULAR] Size of 0 is forbiden\n");
    return -EINVAL;
  }

  memalloc(parsed);

  return count;
}

ssize_t circular_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  int result;
  char k_buff[BUFF_SIZE] = {};
  ssize_t bytes_read = 0;
  ssize_t read = 0;
  ssize_t iter = 0;

  while (bytes_read < count) {
    read = ((count - bytes_read > BUFF_SIZE) ? BUFF_SIZE : count - bytes_read);

    result = copy_from_user(k_buff, buff + bytes_read, read);
    if (result != 0) {
      printk(KERN_INFO "[CIRCULAR] Copying from userspace has failed!");
      return -EFAULT;
    }
    iter = 0;
    while (iter < read) {
      buffer_data[index] = k_buff[iter];
      index = (index + 1) % DEVICE_SIZE;
      iter++;
    }

    bytes_read += read;
  }

  buffer_data[index] = '\0';

  return count;
}

struct file_operations circular_fops = {
  read : circular_read,
  write : circular_write
};

struct file_operations circular_proc_ops = {
    .read = circular_proc_read,
    .write = circular_proc_write,
};

static struct miscdevice circular_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "circular",
    .fops = &circular_fops
};


static int __init circular_init(void)
{
  struct proc_dir_entry *proc_entry;
  int res = 0;

  res = memalloc(DEVICE_SIZE);
  if (res) {
    printk("[CIRCULAR] Memory allocation failed");
    return res;
  }

  res = misc_register(&circular_misc_device);

  if (res) {
    if (buffer_data) {
      kfree(buffer_data);
    }
    printk("[CIRCULAR] Could not create character device!");
    return res;
  }

  proc_entry = proc_create("circular", 0777, NULL, &circular_proc_ops);
  if (!proc_entry) {
    misc_deregister(&circular_misc_device);
    printk("[CIRCULAR] Proc allocation error");
    return 2;
  }

  printk("[CIRCULAR] Module has been initialized");

  return 0;
}

static void __exit circular_exit(void)
{
  misc_deregister(&circular_misc_device);
  if (buffer_data) {
    kfree(buffer_data);
  }
  remove_proc_entry("circular", NULL);

  printk("[CIRCULAR] Module has been removed");
}

module_init(circular_init);
module_exit(circular_exit);