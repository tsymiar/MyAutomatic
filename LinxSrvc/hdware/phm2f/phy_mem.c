#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/ioctl.h> // _IOW

#define DEVICE_NAME "phy_mem_drv"
#define IOCTL_MAGIC 0x1234 // Magic number

struct phy_addr_params;
struct chunk_params;

// IOCTL macro definitions
#define IOCTL_SET_PHY_ADDR   _IOW(IOCTL_MAGIC, 1, struct phy_addr_params)
#define IOCTL_SET_OUTPUT_FD  _IOW(IOCTL_MAGIC, 2, int)
#define IOCTL_WRITE_CHUNK    _IOW(IOCTL_MAGIC, 3, struct chunk_params)

// IOCTL parameter structures
struct phy_addr_params {
    phys_addr_t phys_addr;
    size_t total_size;
};

struct chunk_params {
    size_t mem_offset;
    size_t chunk_size;
    size_t file_offset;
};

// Device open, close, and IOCTL handling functions
struct phy_mem_private {
    phys_addr_t phys_addr;
    size_t total_size;
    void* vaddr;
    struct file* output_file;
};

static int major_number;
static struct class* dev_class;
static struct device* g_dev;

// Initialize resources when the device is opened
static int device_open(struct inode* inode, struct file* file)
{
    struct phy_mem_private* priv = kzalloc(sizeof(struct phy_mem_private), GFP_KERNEL);
    if (!priv) return -ENOMEM;
    file->private_data = priv;
    return 0;
}

// Release resources when the device is closed
static int device_release(struct inode* inode, struct file* file)
{
    struct phy_mem_private* priv = file->private_data;
    if (priv->vaddr) memunmap(priv->vaddr);
    if (priv->output_file) filp_close(priv->output_file, NULL);
    kfree(priv);
    return 0;
}

// IOCTL handling function
static long device_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    struct phy_mem_private* priv = file->private_data;
    long status = 0;

    switch (cmd) {
    case IOCTL_SET_PHY_ADDR: {
        struct phy_addr_params params;
        if (copy_from_user(&params, (void __user*)arg, sizeof(params))) {
            status = -EFAULT;
            break;
        }
        if (priv->vaddr) {
            memunmap(priv->vaddr);
            priv->vaddr = NULL;
        }
        priv->phys_addr = params.phys_addr;
        priv->total_size = params.total_size;
        priv->vaddr = memremap(priv->phys_addr, priv->total_size, MEMREMAP_WB);
        if (!priv->vaddr) {
            /* On failure, reset parameters so no stale state is kept */
            priv->phys_addr = 0;
            priv->total_size = 0;
            status = -ENOMEM;
        }
        break;
    }

    case IOCTL_SET_OUTPUT_FD: {
        int fd;
        if (copy_from_user(&fd, (void __user*)arg, sizeof(fd))) {
            status = -EFAULT;
            break;
        }
        {
            struct file* new_file = fget(fd);
            if (!new_file) {
                status = -EBADF;
                break;
            }
            if (priv->output_file)
                filp_close(priv->output_file, NULL);
            priv->output_file = new_file;
        }
        break;
    }

    case IOCTL_WRITE_CHUNK: {
        struct chunk_params params;
        loff_t pos;
        ssize_t written;
        if (!priv->vaddr || !priv->output_file) {
            status = -EINVAL;
            break;
        }
        if (copy_from_user(&params, (void __user*)arg, sizeof(params))) {
            status = -EFAULT;
            break;
        }
        if (params.mem_offset + params.chunk_size > priv->total_size) {
            status = -EINVAL;
            break;
        }

        pos = params.file_offset;
        written = kernel_write(priv->output_file,
                               priv->vaddr + params.mem_offset,
                               params.chunk_size,
                               &pos);
        if (written != params.chunk_size) {
            status = -EIO;
        }
        break;
    }

    default:
        status = -ENOTTY;
        break;
    }
    return status;
}

static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

// Driver initialization function
static int __init phy_mem_driver_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        pr_err("Failed to register device\n");
        return major_number;
    }

    dev_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(dev_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("Failed to create class\n");
        return PTR_ERR(dev_class);
    }

    g_dev = device_create(dev_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(g_dev)) {
        class_destroy(dev_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        pr_err("Failed to create device\n");
        return PTR_ERR(g_dev);
    }

    pr_info("Driver loaded successfully\n");
    return 0;
}

// Driver cleanup function
static void __exit phy_mem_driver_exit(void)
{
    device_destroy(dev_class, MKDEV(major_number, 0));
    class_destroy(dev_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("Driver unloaded\n");
}

module_init(phy_mem_driver_init);
module_exit(phy_mem_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tsymiar");
MODULE_DESCRIPTION("Physical Memory to File Driver");
MODULE_VERSION("0.1");
