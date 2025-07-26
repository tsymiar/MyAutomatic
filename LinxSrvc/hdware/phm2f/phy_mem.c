#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/file.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/version.h>

#define DEVICE_NAME "phy_mem_drv"
#define IOCTL_MAGIC 0x1234
#define MAX_FILE_ID 256  // Maximum number of file handles supported

struct phy_addr_params;
struct chunk_params;

// IOCTL command definitions
#define IOCTL_SET_PHY_ADDR   _IOW(IOCTL_MAGIC, 1, struct phy_addr_params)
#define IOCTL_ADD_USER_FD    _IOW(IOCTL_MAGIC, 2, int)
#define IOCTL_WRITE_CHUNK    _IOW(IOCTL_MAGIC, 3, struct chunk_params)
#define IOCTL_CLOSE_FILE     _IOW(IOCTL_MAGIC, 4, int)
#define IOCTL_SYNC_FILE      _IOW(IOCTL_MAGIC, 5, int)

// IOCTL parameter structures
struct phy_addr_params {
    phys_addr_t phys_addr;
    size_t total_size;
};

struct chunk_params {
    int usr_id;
    size_t mem_offset;
    size_t chunk_size;
    size_t file_offset;
};

// File handle management structure
struct user_handle {
    int usr_id;         // File handle identifier
    struct file* file;  // Associated file pointer
    struct mutex lock;  // Independent lock for each file
    struct list_head list;
};

// Device private data structure
struct phy_mem_private {
    struct kref kref;
    struct mutex global_lock;   // Global lock to protect data structures
    phys_addr_t phys_addr;
    size_t total_size;
    void* vaddr;
    struct list_head files;     // File handle list
    int next_file_id;           // Next available file ID
};

static int major_number;
static struct class* dev_class;
static struct device* device;

// Resource release function
static void release_private(struct kref* kref)
{
    struct phy_mem_private* priv = container_of(kref, struct phy_mem_private, kref);
    struct user_handle* uh, * tmp;

    // Clean up all file handles
    list_for_each_entry_safe(uh, tmp, &priv->files, list)
    {
        if (uh->file) {
            filp_close(uh->file, NULL);
            mutex_destroy(&uh->lock);
        }
        list_del(&uh->list);
        kfree(uh);
    }

    if (priv->vaddr)
        memunmap(priv->vaddr);

    kfree(priv);
}

// Find file handle by ID
static struct user_handle* find_file_handle(struct phy_mem_private* priv, int id)
{
    struct user_handle* uh;

    list_for_each_entry(uh, &priv->files, list)
    {
        if (uh->usr_id == id)
            return uh;
    }
    return NULL;
}

// Device open function
static int device_open(struct inode* inode, struct file* file)
{
    struct phy_mem_private* priv;

    priv = kzalloc(sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    kref_init(&priv->kref);
    mutex_init(&priv->global_lock);
    INIT_LIST_HEAD(&priv->files);
    priv->next_file_id = 1;  // IDs start from 1
    file->private_data = priv;

    return 0;
}

// Device close function
static int device_release(struct inode* inode, struct file* file)
{
    struct phy_mem_private* priv = file->private_data;
    kref_put(&priv->kref, release_private);
    return 0;
}

// IOCTL handler function
static long device_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    struct phy_mem_private* priv = file->private_data;
    long stats = 0;

    switch (cmd) {
    case IOCTL_SET_PHY_ADDR: {
        struct phy_addr_params params;

        if (copy_from_user(&params, (void __user*)arg, sizeof(params)))
            return -EFAULT;

        mutex_lock(&priv->global_lock);

        // If memory is already mapped, unmap it first
        if (priv->vaddr) {
            memunmap(priv->vaddr);
            priv->vaddr = NULL;
        }

        priv->phys_addr = params.phys_addr;
        priv->total_size = params.total_size;
        priv->vaddr = memremap(priv->phys_addr, priv->total_size, MEMREMAP_WB);

        if (!priv->vaddr) {
            priv->phys_addr = 0;
            priv->total_size = 0;
            stats = -ENOMEM;
        }

        mutex_unlock(&priv->global_lock);
        break;
    }

    case IOCTL_ADD_USER_FD: {
        int fd;
        struct file* filp;
        struct user_handle* uh;
        int new_id;

        if (copy_from_user(&fd, (void __user*)arg, sizeof(fd)))
            return -EFAULT;

        filp = fget(fd);
        if (!filp)
            return -EBADF;

        uh = kzalloc(sizeof(*uh), GFP_KERNEL);
        if (!uh) {
            fput(filp);
            return -ENOMEM;
        }

        mutex_lock(&priv->global_lock);
        new_id = priv->next_file_id++;
        if (new_id > MAX_FILE_ID) {
            kfree(uh);
            fput(filp);
            stats = -EMFILE;
            goto out;
        }

        uh->usr_id = new_id;
        uh->file = filp;
        mutex_init(&uh->lock);
        INIT_LIST_HEAD(&uh->list);
        list_add_tail(&uh->list, &priv->files);

        // Return the allocated file ID to user space
        if (copy_to_user((void __user*)arg, &new_id, sizeof(new_id))) {
            list_del(&uh->list);
            kfree(uh);
            fput(filp);
            stats = -EFAULT;
            goto out;
        }

    out:
        mutex_unlock(&priv->global_lock);
        break;
    }

    case IOCTL_WRITE_CHUNK: {
        struct chunk_params params;
        struct user_handle* uh;
        loff_t pos;
        ssize_t written;

        if (copy_from_user(&params, (void __user*)arg, sizeof(params)))
            return -EFAULT;

        mutex_lock(&priv->global_lock);
        uh = find_file_handle(priv, params.usr_id);
        if (!uh || !priv->vaddr) {
            mutex_unlock(&priv->global_lock);
            return -EINVAL;
        }

        // Check memory range validity
        if (params.mem_offset + params.chunk_size > priv->total_size) {
            mutex_unlock(&priv->global_lock);
            return -EINVAL;
        }

        // Acquire file-level lock
        mutex_lock(&uh->lock);
        mutex_unlock(&priv->global_lock);

        pos = params.file_offset;
        written = kernel_write(uh->file,
            (char*)priv->vaddr + params.mem_offset,
            params.chunk_size,
            &pos);

        mutex_unlock(&uh->lock);

        if (written < 0)
            return written;
        if (written != params.chunk_size)
            return -EIO;
        break;
    }

    case IOCTL_SYNC_FILE: {
        int file_id;
        struct user_handle* uh;

        if (copy_from_user(&file_id, (void __user*)arg, sizeof(file_id)))
            return -EFAULT;

        mutex_lock(&priv->global_lock);
        uh = find_file_handle(priv, file_id);
        if (!uh) {
            mutex_unlock(&priv->global_lock);
            return -EINVAL;
        }

        get_file(uh->file);
        mutex_unlock(&priv->global_lock);

        mutex_lock(&uh->lock);
        stats = vfs_fsync(uh->file, 0);
        mutex_unlock(&uh->lock);

        fput(uh->file);
        break;
    }

    case IOCTL_CLOSE_FILE: {
        int file_id;
        struct user_handle* uh;

        if (copy_from_user(&file_id, (void __user*)arg, sizeof(file_id)))
            return -EFAULT;

        mutex_lock(&priv->global_lock);
        uh = find_file_handle(priv, file_id);
        if (!uh) {
            mutex_unlock(&priv->global_lock);
            return -EINVAL;
        }

        list_del(&uh->list);
        mutex_unlock(&priv->global_lock);

        mutex_lock(&uh->lock);
        filp_close(uh->file, NULL);
        mutex_unlock(&uh->lock);

        mutex_destroy(&uh->lock);
        kfree(uh);
        break;
    }

    default:
        return -ENOTTY;
    }

    return stats;
}

static struct file_operations fops = {
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static int __init phy_mem_drv_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
        return major_number;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
    dev_class = class_create(DEVICE_NAME);
#else
    dev_class = class_create(THIS_MODULE, DEVICE_NAME);
#endif
    if (IS_ERR(dev_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(dev_class);
    }

    device = device_create(dev_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        class_destroy(dev_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(device);
    }

    pr_info("Multi-file physical memory driver loaded\n");
    return 0;
}

static void __exit phy_mem_drv_exit(void)
{
    device_destroy(dev_class, MKDEV(major_number, 0));
    class_destroy(dev_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("Driver unloaded\n");
}

module_init(phy_mem_drv_init);
module_exit(phy_mem_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tsymiar");
MODULE_DESCRIPTION("Multi-file Physical Memory Driver");
MODULE_VERSION("0.2");
MODULE_ALIAS("platform:" DEVICE_NAME);
