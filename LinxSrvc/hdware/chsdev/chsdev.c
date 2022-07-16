#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/fs.h> 
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NUMBER 1 //设备数量
#define DEVICE_SNAME "stachardevice" //静态名称
#define DEVICE_DNAME "dynchardevice" //动态名称
#define DEVICE_MINOR_NUMBER 0 //动态请求的第一个次设备号
#define DEVICE_CLASS_NAME "chs_dev_class" //创建的类名称
#define DEVICE_NODE_NAME "chars-node"      //创建的节点名称

struct cdev cdev;
struct class* class;
struct device* device;
dev_t devNo;
static int majNo = 234, minNo;
static char chsDat[1024];

module_param(majNo, int, S_IRUSR);   //主设备号
module_param(minNo, int, S_IRUSR);   //次设备号

static ssize_t chs_dev_read(struct file* flip, char* buf, size_t len, loff_t* off)
{
    if (copy_to_user(buf, chsDat, sizeof(int))) {
        return -EFAULT;
    }
    printk("chs_dev_read [%s].\n", buf);
    return sizeof(int);
}

static ssize_t chs_dev_write(struct file* flip, const char* buf, size_t len, loff_t* off)
{
    if (copy_from_user(chsDat, buf, sizeof(int))) {
        return -EFAULT;
    }
    printk("chs_dev_write [%s].\n", chsDat);
    return sizeof(int);
}

static int chs_dev_open(struct inode* inode, struct file* file)
{
    printk("chs_dev_open!\n");
    return 0;
}

static int chs_dev_close(struct inode* inode, struct file* file)
{
    printk("chs_dev_close!\n");
    return 0;
}

struct file_operations chs_dev_ops = {
    .owner = THIS_MODULE,
    .read = chs_dev_read,
    .write = chs_dev_write,
    .open = chs_dev_open,
    .release = chs_dev_close,
};

static int device_init(void)
{
    int stat;
    if (majNo)  //传入了主设备号用静态的方法
    {
        printk("majNo = %d\n", majNo);  //主设备号
        printk("minNo = %d\n", minNo);  //次设备号
        devNo = MKDEV(majNo, minNo);   //主次设备号合成一个dev_t类型
        stat = register_chrdev_region(devNo, DEVICE_NUMBER, DEVICE_SNAME); //静态分配
        if (stat < 0) {
            printk("register_chs_dev_region error!\n");
        }
        printk("register_chs_dev_region ok!\n");
    } else   //没有传入主设备号用动态的方法
    {
        stat = alloc_chrdev_region(&devNo, DEVICE_MINOR_NUMBER, DEVICE_NUMBER, DEVICE_DNAME); //动态分配
        if (stat < 0) {
            printk("alloc_chs_dev_region error!\n");
        }
        printk("alloc_chs_dev_region ok!\n");
        majNo = MAJOR(devNo); //从dev_t中分离出主设备号
        minNo = MINOR(devNo); //从dev_t中分离出次设备号
        printk("majNo = %d\n", majNo);
        printk("minNo = %d\n", minNo);
    }
    cdev.owner = THIS_MODULE;
    cdev_init(&cdev, &chs_dev_ops); //初始化cdev
    cdev_add(&cdev, devNo, DEVICE_NUMBER); //注册到内核
    class = class_create(THIS_MODULE, DEVICE_CLASS_NAME); //注册类
    device = device_create(class, NULL, devNo, NULL, DEVICE_NODE_NAME); //注册设备
    return 0;
}

static void device_exit(void)
{
    unregister_chrdev_region(MKDEV(majNo, minNo), DEVICE_NUMBER); //申请几个注销几个
    cdev_del(&cdev); //注销cdev
    device_destroy(class, devNo); //注销设备
    class_destroy(class); //注销类
    printk("unregister_chs_dev_region ok!\n");
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
