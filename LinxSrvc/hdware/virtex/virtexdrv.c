#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/poll.h>

#define DEVICE_NUMBER 1                     //设备数量
#define DEVICE_SNAME "stachardevice"        //静态名称
#define DEVICE_DNAME "dynchardevice"        //动态名称
#define DEVICE_MINOR_NUMBER 0               //动态请求的第一个次设备号
#define DEVICE_CLASS_NAME "virtex_dev_class" //创建的类名称
#define DEVICE_NODE_NAME "virtexdma0"       //创建的节点名称

typedef uint64_t dma_addr_t;
typedef uint64_t phys_addr_t;

typedef struct tagDdrBar {
    dma_addr_t busAddr;
    phys_addr_t phyAddr;
    phys_addr_t winSize;
} DdrBar;

typedef struct StVirtex {
    uint32_t offset; // register offset
    uint32_t val;    // register val
} Virtex;

typedef struct StUsrMsg {
    struct StUsrMsg* next;
    union stMsg {
        DdrBar bar;
        Virtex reg;
    } msg;
    uint64_t intSize;
    uint32_t notify;
    uint32_t ddrOk;
    uint32_t chan;
} ST_UsrMsg;

typedef struct Queue {
    ST_UsrMsg* head;
    ST_UsrMsg* tail;
} Queue;

void init_queue(Queue* q)
{
    q->head = q->tail = NULL;
}

void deinit_queue(Queue* q)
{
    ST_UsrMsg* cr = q->head;
    while (cr != NULL) {
        ST_UsrMsg* next = cr->next;
        kfree(cr);
        cr = next;
    }
    q->tail = q->head = NULL;
}

int queue_size(Queue* q)
{
    ST_UsrMsg* cr = q->head;
    int cnt = 0;
    while (cr != NULL) {
        cr = cr->next;
        cnt++;
    }
    return cnt;
}

ST_UsrMsg* msg_front(Queue* q)
{
    ST_UsrMsg* msg = NULL;
    if (q != NULL && q->head != NULL) {
        msg = q->head;
    }
    return msg;
}

void msg_pop(Queue* q)
{
    if (q == NULL || q->head == NULL) {
        return;
    }
    if (q->head->next == NULL) {
        kfree(q->head);
        q->head = q->tail = NULL;
    } else {
        ST_UsrMsg* next = q->head->next;
        kfree(q->head);
        q->head = next;
    }
}

void msg_push(Queue* q, ST_UsrMsg msg)
{
    ST_UsrMsg* st = (ST_UsrMsg*)kmalloc(sizeof(ST_UsrMsg), GFP_ATOMIC);
    if (st != NULL) {
        memset(st, 0, sizeof(ST_UsrMsg));
        st->msg = msg.msg;
        st->next = NULL;
    }
    if (q->tail == NULL) {
        q->head = q->tail = st;
    } else {
        q->tail->next = st;
        q->tail = st;
    }
}

struct cdev g_cdev;
struct class* g_class;
struct device* g_device;
dev_t g_devno;
static int g_majno = 0x123, g_minno;
static Queue g_queue;
static ST_UsrMsg g_msg_user;
static volatile int ev_ok = 0;
static volatile int g_ddrOk = 0;
static DECLARE_WAIT_QUEUE_HEAD(select_waitq);

module_param(g_majno, int, S_IRUSR); //主设备号
module_param(g_minno, int, S_IRUSR); //次设备号

static ssize_t virtex_dev_read(struct file* flip, char* buf, size_t len, loff_t* off)
{
    if (flip->f_flags & O_NONBLOCK) {
    } else {
        // wait_event_interruptible(select_waitq, ev_ok);
    }
#ifdef KNL_TO_USR
    if (copy_to_user(buf, &g_msg_user, sizeof(ST_UsrMsg))) {
        return -EFAULT;
    }
#endif
    printk("virtex_dev_read [%08x, %lu].\n", flip->f_flags, len);
    return sizeof(ST_UsrMsg);
}

static ssize_t virtex_dev_write(struct file* flip, const char* buf, size_t len, loff_t* off)
{
#ifdef USR_TO_KNL
    if (copy_from_user(&g_msg_user, buf, sizeof(ST_UsrMsg))) {
        return -EFAULT;
    }
#endif
    printk("virtex_dev_write [val=%d,bus=%p,phy=%p,size=%llu].\n",
        g_msg_user.msg.reg.val, g_msg_user.msg.bar.busAddr, g_msg_user.msg.bar.phyAddr, g_msg_user.msg.bar.winSize);
    return sizeof(ST_UsrMsg);
}

static int virtex_dev_open(struct inode* inode, struct file* file)
{
    printk("virtex_dev_open!\n");
    return 0;
}

static int virtex_dev_close(struct inode* inode, struct file* file)
{
    printk("virtex_dev_close!\n");
    return 0;
}

static void set_notify(uint32_t* notify, char last)
{
    uint64_t status = 0;
    if (last) {
        status = 1;
    }
    (*notify) |= ((status << 31) | g_msg_user.chan);
}

static void set_chan(uint32_t* chan)
{
    uint64_t value = 0;
    uint32_t offset = *chan;
    while ((offset = offset >> 1) != 0) {
        value++;
    }
    (*chan) = value;
}

static unsigned int virtex_dev_select_poll(struct file* flip, struct poll_table_struct* wait)
{
    unsigned int mask = 0;
    poll_wait(flip, &select_waitq, wait);
    if (ev_ok) {
        mask = POLLIN | POLLRDNORM;
    }
    if (g_msg_user.chan && g_ddrOk > 0 && g_msg_user.ddrOk == 0) {
        mask = POLLOUT;
    }
    printk("virtex_dev_select_poll:%d mask=%08x\n", __LINE__, mask);
    return mask;
}

int virtex_dev_mmap(struct file* flip, struct vm_area_struct* vm)
{
    printk("virtex_dev_mmap:%d flag=%08x\n", __LINE__, flip->f_flags);
    return 0;
}

long virtex_dev_ioctl(struct file* flip, unsigned int cmd, unsigned long arg)
{
    unsigned long value = 0;
    uint32_t notify = 0;
    Virtex reg;
    memset(&reg, 0, sizeof(Virtex));
    switch (cmd) {
    case _IO('V', 0x10): // virtex_get_write_notify
        set_notify(&notify, 0);
        if (copy_to_user(*(void**)(&arg), (void*)&notify, sizeof(uint32_t))) {
            return -EFAULT;
        }
        printk("virtex_dev_ioctl:%d 0x%08x, notify=0x%x\n", __LINE__, cmd, notify);
        break;
    case _IO('V', 0x11): // virtex_get_read_notify
        if (copy_from_user(&reg, *(void**)(&arg), sizeof(Virtex))) {
            return -EFAULT;
        }
        printk("virtex_dev_ioctl:%d 0x%08x, offset=0x%x, value=0x%x\n", __LINE__, cmd, reg.offset, reg.val);
        break;
    case _IO('V', 0x12): // virtex_get_data_bar
        break;
    case _IO('V', 0x13): // write_reg
        if (copy_from_user(&reg, *(void**)(&arg), sizeof(Virtex))) {
            return -EFAULT;
        }
        switch (reg.offset) {
        case 0x4:
        case 0x1004:
            printk("virtex_dev_ioctl:%d 0x%08x, 0x%08x, 0x%08x, soft reset\n", __LINE__, cmd, reg.offset, reg.val);
            break;
        case 0x1010: /*DDR open*/
            g_msg_user.ddrOk = reg.val;
            if (g_msg_user.ddrOk > 0) {
                g_ddrOk++;
            }
            printk("virtex_dev_ioctl:%d 0x%08x, 0x%08x, ddr is ok=0x%08x\n", __LINE__, cmd, reg.offset, g_msg_user.ddrOk);
            break;
        case 0x1014: /*single channel enable*/
            g_msg_user.chan = reg.val;
            set_chan(&g_msg_user.chan);
            printk("virtex_dev_ioctl:%d 0x%08x, 0x%08x, set chan=0x%08x\n", __LINE__, cmd, reg.offset, g_msg_user.chan);
            break;
        case 0x100c:
            g_msg_user.intSize |= (uint64_t)reg.val >> 16;
            printk("virtex_dev_ioctl:%d 0x%08x, offset=0x%x, low_size=0x%llx\n", __LINE__, cmd, reg.offset, g_msg_user.intSize);
            break;
        case 0x10fc:
            g_msg_user.intSize |= (uint64_t)reg.val << 14;
            printk("virtex_dev_ioctl:%d 0x%08x, offset=0x%x, high_size=0x%llx\n", __LINE__, cmd, reg.offset, g_msg_user.intSize);
            break;
        case 0x105c:
        case 0x1050: // simulator data register enable
            printk("virtex_dev_ioctl:%d 0x%08x, offset=0x%x, value=0x%x\n", __LINE__, cmd, reg.offset, reg.val);
            break;
        default:
            printk("virtex_dev_ioctl:%d 0x%08x, 0x%08x, 0x%08x, not impl\n", __LINE__, cmd, reg.offset, reg.val);
            break;
        }
        break;
    case _IO('V', 0x14): // read_reg
        if (copy_from_user(&reg, *(void**)(&arg), sizeof(Virtex))) {
            return -EFAULT;
        }
        switch (reg.offset) {
        case 0:
        case 0x1000: // version
            value = 0x12345;
            if (copy_to_user(*(void**)(&arg), (void*)&value, sizeof(uint32_t))) {
                return -EFAULT;
            }
            printk("virtex_dev_ioctl:%d 0x%08x, 0x00001000, version=%08lx\n", __LINE__, cmd, arg);
            break;
        case 0x100: // dma ok
            printk("virtex_dev_ioctl:%d 0x%08x, 0x00000100, fifo pop\n", __LINE__, cmd);
            break;
        case 0x118: // interrupt size;
            reg.val = g_msg_user.intSize;
            if (copy_to_user(*(void**)(&arg), (void*)&reg, sizeof(Virtex))) {
                return -EFAULT;
            }
            printk("virtex_dev_ioctl:%d 0x%08x, offset=0x%x, size=0x%x\n", __LINE__, cmd, reg.offset, reg.val);
            break;
        case 0x104: // status
            value = 0x1;
            if (copy_to_user(*(void**)(&arg), (void*)&value, sizeof(uint32_t))) {
                return -EFAULT;
            }
            printk("virtex_dev_ioctl:%d 0x%08x, 0x00000104, status=%08lx\n", __LINE__, cmd, arg);
            break;
        case 0x10c: // debug
            value = 0xff;
            if (copy_to_user(*(void**)(&arg), (void*)&value, sizeof(uint32_t))) {
                return -EFAULT;
            }
            printk("virtex_dev_ioctl:%d 0x%08x, 0x0000010c, debug=%08lx\n", __LINE__, cmd, arg);
            break;
        default:
            printk("virtex_dev_ioctl:%d 0x%08x, 0x%08x, not deal\n", __LINE__, cmd, reg.offset);
            break;
        }
        break;
    default:
        if (copy_from_user(&value, *(void**)(&arg), sizeof(unsigned long))) {
            return -EFAULT;
        }
        printk("virtex_dev_ioctl:%d 0x%08x, 0x%lx\n", __LINE__, cmd, value);
        break;
    }
    return 0;
}

struct file_operations virtex_dev_ops = {
    .owner = THIS_MODULE,
    .read = virtex_dev_read,
    .write = virtex_dev_write,
    .open = virtex_dev_open,
    .release = virtex_dev_close,
    .unlocked_ioctl = virtex_dev_ioctl,
    .poll = virtex_dev_select_poll,
    .mmap = virtex_dev_mmap };

static int device_init(void)
{
    int stat;
    if (g_majno) //传入了主设备号用静态的方法
    {
        printk("devno = %d:%d.\n", g_majno, g_minno);                        //主设备号
        g_devno = MKDEV(g_majno, g_minno);                                   //主次设备号合成一个dev_t类型
        stat = register_chrdev_region(g_devno, DEVICE_NUMBER, DEVICE_SNAME); //静态分配
        if (stat < 0) {
            printk("register_virtex_dev_region error!\n");
        } else {
            printk("register_virtex_dev_region ok!\n");
        }
    } else //没有传入主设备号用动态的方法
    {
        stat = alloc_chrdev_region(&g_devno, DEVICE_MINOR_NUMBER, DEVICE_NUMBER, DEVICE_DNAME); //动态分配
        if (stat < 0) {
            printk("alloc_virtex_dev_region error!\n");
        } else {
            printk("alloc_virtex_dev_region ok!\n");
        }
        g_majno = MAJOR(g_devno); //从dev_t中分离出主设备号
        g_minno = MINOR(g_devno); //从dev_t中分离出次设备号
        printk("majno = %d, minno = %d\n", g_majno, g_minno);
    }
    g_cdev.owner = THIS_MODULE;
    cdev_init(&g_cdev, &virtex_dev_ops);                                       //初始化cdev
    cdev_add(&g_cdev, g_devno, DEVICE_NUMBER);                                //注册到内核
    g_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);                   //注册类
    g_device = device_create(g_class, NULL, g_devno, NULL, DEVICE_NODE_NAME); //注册设备
    init_queue(&g_queue);
    return 0;
}

static void device_exit(void)
{
    deinit_queue(&g_queue);
    unregister_chrdev_region(MKDEV(g_majno, g_minno), DEVICE_NUMBER); //申请几个注销几个
    cdev_del(&g_cdev);                                                //注销cdev
    device_destroy(g_class, g_devno);                                 //注销设备
    class_destroy(g_class);                                           //注销类
    printk("unregister_virtex_dev_region ok!\n");
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
