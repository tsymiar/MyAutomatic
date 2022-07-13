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
#define DEVICE_CLASS_NAME "ch_dev_class" //创建的类名称
#define DEVICE_NODE_NAME "ch_dev_node" //创建的节点名称

static int major_num = 230, minor_num;
dev_t dev_num;

struct cdev cdev;
struct class* class;
struct device* device;

module_param(major_num, int, S_IRUSR);   //主设备号
module_param(minor_num, int, S_IRUSR);   //次设备号

int ch_dev_open(struct inode* inode, struct file* file)
{
    printk("ch_dev_open!\n");
    return 0;
}

int ch_dev_close(struct inode* inode, struct file* file)
{
    printk("ch_dev_close!\n");
    return 0;
}

struct file_operations ch_dev_ops = {
    .owner = THIS_MODULE,
    .open = ch_dev_open,
    .release = ch_dev_close
};

static int hello_init(void)
{
    int stat;
    if (major_num)  //传入了主设备号用静态的方法
    {
        printk("major_num = %d\n", major_num);  //主设备号
        printk("minor_num = %d\n", minor_num);  //次设备号
        dev_num = MKDEV(major_num, minor_num);  //主次设备号合成一个dev_t类型
        stat = register_chdev_region(dev_num, DEVICE_NUMBER, DEVICE_SNAME); //静态分配
        if (stat < 0) {
            printk("register_chdev_region error!\n");
        }
        printk("register_chdev_region ok!\n");
    } else   //没有传入主设备号用动态的方法
    {
        stat = alloc_chdev_region(&dev_num, DEVICE_MINOR_NUMBER, DEVICE_NUMBER, DEVICE_DNAME); //动态分配
        if (stat < 0) {
            printk("alloc_chdev_region error!\n");
        }
        printk("alloc_chdev_region ok!\n");
        major_num = MAJOR(dev_num); //从dev_t中分离出主设备号
        minor_num = MINOR(dev_num); //从dev_t中分离出次设备号
        printk("major_num = %d\n", major_num);
        printk("minor_num = %d\n", minor_num);
    }
    cdev.owner = THIS_MODULE;
    cdev_init(&cdev, &ch_dev_ops); //初始化cdev
    cdev_add(&cdev, dev_num, DEVICE_NUMBER); //注册到内核
    class = class_create(THIS_MODULE, DEVICE_CLASS_NAME); //注册类
    device = device_create(class, NULL, dev_num, NULL, DEVICE_NODE_NAME); //注册设备
    return 0;
}

static int hello_exit(void)
{
    unregister_chdev_region(MKDEV(major_num, minor_num), DEVICE_NUMBER); //申请几个注销几个
    cdev_del(&cdev); //注销cdev
    device_destroy(class, dev_num);  //注销设备
    class_destroy(class); //注销类
    printk("unregister_chdev_region ok!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
