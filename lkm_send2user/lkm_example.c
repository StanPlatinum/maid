/*
 * sending signal from kernel module to user space. Usage:

insmod ./lkm_example.ko
mknod /dev/lkm_example c 249 0
./test1 &
echo 0 > /dev/lkm_example 

 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/sched.h>
#include <asm/siginfo.h>
#include <linux/pid_namespace.h>
#include <linux/pid.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yuwen");
MODULE_DESCRIPTION("A Linux module that sends signal to userspace.");
MODULE_VERSION("0.01");

#define DEVICE_NAME "lkm_example"
#define EXAMPLE_MSG "Hello, World!\n"
#define MSG_BUFFER_LEN 15

typedef enum
{
        THIS_MODULE_IOCTL_SET_OWNER = 0x111,
} MODULE_IOCTL_CMD;

#if 1
#undef SIGRTMIN
#define SIGRTMIN 34
#endif

/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static void send_signal(int sig_num);

static int major_num;
static int device_open_count = 0;
static char msg_buffer[MSG_BUFFER_LEN];
static char *msg_ptr;

/* This structure points to all of the device functions */
static struct file_operations file_ops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
    .compat_ioctl = device_ioctl};

static int owner = 0;
struct task_struct *current_task;

static void send_signal(int sig_num)
{
        //struct siginfo info;
        struct kernel_siginfo info;

        int ret;

        if (owner == 0)
                return;
        printk("%s,%d.sending signal %d to owner %d\n", __func__, __LINE__, sig_num, owner);

        //memset(&info, 0, sizeof(struct siginfo));
        memset(&info, 0, sizeof(struct kernel_siginfo));

        info.si_signo = sig_num;
        info.si_code = 0;
        info.si_int = 1234;
        if (current_task == NULL)
        {
                rcu_read_lock();
                current_task = pid_task(find_vpid(owner), PIDTYPE_PID);
                rcu_read_unlock();
        }
        ret = send_sig_info(sig_num, &info, current_task);
        if (ret < 0)
        {
                printk("error sending signal\n");
        }
}

/* When a process reads from our device, this gets called. */
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset)
{
        /* not implemented yet */
        return 0;
}

/* Called when a process tries to write to our device */
static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset)
{
        /* This is a read-only device */
        printk(KERN_ALERT "This operation is not supported.\n");
        send_signal(SIGRTMIN + 1);
        /*        return -EINVAL;*/
        return len;
}

/* Called when a process opens our device */
static int device_open(struct inode *inode, struct file *file)
{
        /* If device is open, return busy */
        if (device_open_count)
        {
                return -EBUSY;
        }
        device_open_count++;
        try_module_get(THIS_MODULE);
        return 0;
}

/* Called when a process closes our device */
static int device_release(struct inode *inode, struct file *file)
{
        /* Decrement the open counter and usage count. Without this, the module would not unload. */
        device_open_count--;
        module_put(THIS_MODULE);
        return 0;
}

static int __init lkm_example_init(void)
{
        /* Fill buffer with our message */
        strncpy(msg_buffer, EXAMPLE_MSG, MSG_BUFFER_LEN);
        /* Set the msg_ptr to the buffer */
        msg_ptr = msg_buffer;
        /* Try to register character device */
        major_num = register_chrdev(0, "lkm_example", &file_ops);
        if (major_num < 0)
        {
                printk(KERN_ALERT "Could not register device: %d\n", major_num);
                return major_num;
        }
        else
        {
                printk(KERN_INFO "lkm_example module loaded with device major number %d\n", major_num);
                return 0;
        }
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        if (cmd == THIS_MODULE_IOCTL_SET_OWNER)
        {
                printk("%s, owner pid %d\n", __func__, owner);
                if (copy_from_user(&owner, (int *)arg, sizeof(int)))
                { /* ??? */
                        return -EFAULT;
                }
                current_task = NULL;
                return 0;
        }
        else
                return -ENOIOCTLCMD;
}

static void __exit lkm_example_exit(void)
{
        /* Remember — we have to clean up after ourselves. Unregister the character device. */
        unregister_chrdev(major_num, DEVICE_NAME);
        printk(KERN_INFO "Goodbye, World!\n");
}

/* Register module functions */
module_init(lkm_example_init);
module_exit(lkm_example_exit);