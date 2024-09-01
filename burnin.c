#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/console.h>

#include <linux/tty.h>

#define INITIAL_INTERVAL_IN_MINUTES 480
#define COLOR_CHANGE_INTERVAL_SECONDS 5

static struct fb_info *fb_info;
static struct timer_list main_timer, color_timer;
static struct work_struct fb_update_work;
static int color_value = 0;
static int minute_interval = INITIAL_INTERVAL_IN_MINUTES;


module_param(minute_interval, int, 0644);

static void check_fb_info(void) {
    if (fb_info) {
        printk(KERN_INFO "fb_info->fix.smem_len: %u\n", fb_info->fix.smem_len);
        printk(KERN_INFO "fb_info->var.xres: %u\n", fb_info->var.xres);
        printk(KERN_INFO "fb_info->var.yres: %u\n", fb_info->var.yres);
        printk(KERN_INFO "fb_info->var.bits_per_pixel: %u\n", fb_info->var.bits_per_pixel);
        printk(KERN_INFO "fb_info->screen_base: %p\n", fb_info->screen_base);
    } else {
        printk(KERN_ERR "fb_info is NULL\n");
    }
}


static void clear_tty(void) {
    struct console *console;

    for_each_console(console) {
        if (console->unblank) {
            console->unblank();
        }
    }

}


static int get_framebuffer_info(void) {
    fb_info = registered_fb[0];
    if (!fb_info) {
        printk(KERN_ERR "No registered framebuffer found\n");
        return -ENODEV;
    }
    return 0;
}

static void paint_rgb_fb(unsigned char red, unsigned char green, unsigned char blue) {
    uint32_t i;
    uint32_t *fb_base;
    uint32_t color = (0xFF << 24) | (red << 16) | (green << 8) | blue;

    if (!fb_info) return;

    fb_base = (uint32_t *)(fb_info->screen_base);
    for (i = 0; i < fb_info->fix.smem_len / 4; i++) {
        fb_base[i] = color;
    }

    wmb();
    printk(KERN_INFO "Screen painted with color: R=%d, G=%d, B=%d\n", red, green, blue);
}

static void fb_update_work_function(struct work_struct *work) {
    if (fb_info->fbops && fb_info->fbops->fb_pan_display) {
        int ret = fb_info->fbops->fb_pan_display(&fb_info->var, fb_info);
        if (ret) {
            printk(KERN_ERR "Failed to pan display in fb_update_work_function: %d\n", ret);
        } else {
            printk(KERN_INFO "Pan display successful in fb_update_work_function\n");
            clear_tty();
        }
    }
}

static void main_timer_callback(struct timer_list *t) {
    memset(fb_info->screen_base, 0, fb_info->fix.smem_len);
    mod_timer(&color_timer, jiffies + msecs_to_jiffies(COLOR_CHANGE_INTERVAL_SECONDS * 1000));

}

static void refresh_timer_callback(struct timer_list *t) {
    if (color_value <= 255) {
        paint_rgb_fb(color_value, color_value, color_value);
        schedule_work(&fb_update_work);
        color_value += 51;
        mod_timer(t, jiffies + msecs_to_jiffies(COLOR_CHANGE_INTERVAL_SECONDS * 1000));
    } else {
        color_value = 0;
        printk(KERN_INFO "Color cycle complete and framebuffer restored\n");
    }
}

static int __init burnin_module_init(void) {
    int ret;
    
    ret = get_framebuffer_info();
    if (ret < 0) {
        return ret;
    }
    check_fb_info();

    INIT_WORK(&fb_update_work, fb_update_work_function);
    timer_setup(&main_timer, main_timer_callback, 0);
    timer_setup(&color_timer, refresh_timer_callback, 0);

    mod_timer(&main_timer, jiffies + msecs_to_jiffies(minute_interval * 60 * 1000));
    return 0;
}

static void __exit burnin_module_exit(void) {
    del_timer_sync(&main_timer);
    del_timer_sync(&color_timer);
    flush_work(&fb_update_work);
    
    printk(KERN_INFO "Module exited and resources cleaned up.\n");
}

module_init(burnin_module_init);
module_exit(burnin_module_exit);

MODULE_LICENSE("GPL");


