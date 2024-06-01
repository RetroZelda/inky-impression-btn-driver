#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/kobject.h>

#define NUM_BUTTONS 4
static int gpio_pins[NUM_BUTTONS] = {5, 6, 16, 24};
static int button_irqs[NUM_BUTTONS];
static bool button_states[NUM_BUTTONS] = {false, false, false, false};
static const char* button_names[NUM_BUTTONS] = {"A", "B", "C", "D"};
static struct kobject *kobj_ref;

static irqreturn_t button_irq_handler(int irq, void *dev_id) 
{
    int i;
    for (i = 0; i < NUM_BUTTONS; i++) 
    {
        if (irq == button_irqs[i]) 
        {
            button_states[i] = !button_states[i];
            printk(KERN_INFO "Inky Impression: Interrupt! Button %s state is %d\n", button_names[i], button_states[i]);
            break;
        }
    }
    return IRQ_HANDLED;
}

static ssize_t buttons_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    int i;
    size_t len = 0;
    for (i = 0; i < NUM_BUTTONS; i++) 
    {
        len += sprintf(buf + len, "Button %s state: %d\n", button_names[i], button_states[i]);
    }
    return len;
}

static ssize_t button_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) 
{
    int index = attr->attr.name[5] - 'a';  // Extract button index from attribute name (btn_a, btn_b, ...)
    if (index >= 0 && index < NUM_BUTTONS) 
    {
        return sprintf(buf, "%d\n", button_states[index]);
    }
    return -EINVAL;
}

#define CREATE_BUTTON_ATTR(index) static struct kobj_attribute btn_##index##_attr = __ATTR(btn_##index, 0444, button_show, NULL)
CREATE_BUTTON_ATTR(a);
CREATE_BUTTON_ATTR(b);
CREATE_BUTTON_ATTR(c);
CREATE_BUTTON_ATTR(d);
#undef CREATE_BUTTON_ATTR

static struct kobj_attribute buttons_attr = __ATTR(buttons, 0444, buttons_show, NULL);

static struct attribute *attrs[] = 
{
    &buttons_attr.attr,
    &btn_a_attr.attr,
    &btn_b_attr.attr,
    &btn_c_attr.attr,
    &btn_d_attr.attr,
    NULL,
};

static struct attribute_group attr_group = 
{
    .attrs = attrs,
};

static int __init gpio_button_init(void) 
{
    int result = 0, i;

    for (i = 0; i < NUM_BUTTONS; i++) 
    {
        if (!gpio_is_valid(gpio_pins[i])) 
        {
            printk(KERN_INFO "Inky Impression: Invalid GPIO %d\n", gpio_pins[i]);
            return -ENODEV;
        }

        result = gpio_request(gpio_pins[i], "GPIO Button");
        if (result) 
        {
            printk(KERN_INFO "Inky Impression: GPIO request failed for GPIO %d with result %d\n", gpio_pins[i], result);
            return result;
        }

        result = gpio_direction_input(gpio_pins[i]);
        if (result) 
        {
            printk(KERN_INFO "Inky Impression: GPIO direction set failed for GPIO %d with result %d\n", gpio_pins[i], result);
            gpio_free(gpio_pins[i]);
            return result;
        }

        button_irqs[i] = gpio_to_irq(gpio_pins[i]);
        result = request_irq(button_irqs[i], (irq_handler_t) button_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpio_button_irq", NULL);
        if (result) 
        {
            printk(KERN_INFO "Inky Impression: IRQ request failed for GPIO %d with result %d\n", gpio_pins[i], result);
            gpio_free(gpio_pins[i]);
            return result;
        }
    }

    kobj_ref = kobject_create_and_add("inky-impression", kernel_kobj);
    if (!kobj_ref) 
    {
        printk(KERN_INFO "Inky Impression: Failed to create sysfs directory\n");
        for (i = 0; i < NUM_BUTTONS; i++) 
        {
            free_irq(button_irqs[i], NULL);
            gpio_free(gpio_pins[i]);
        }
        return -ENOMEM;
    }

    result = sysfs_create_group(kobj_ref, &attr_group);
    if (result) 
    {
        printk(KERN_INFO "Inky Impression: Failed to create sysfs files\n");
        kobject_put(kobj_ref);
        for (i = 0; i < NUM_BUTTONS; i++) 
        {
            free_irq(button_irqs[i], NULL);
            gpio_free(gpio_pins[i]);
        }
        return result;
    }

    printk(KERN_INFO "Inky Impression: Module loaded successfully\n");
    return 0;
}

static void __exit gpio_button_exit(void) 
{
    int i;
    kobject_put(kobj_ref);
    for (i = 0; i < NUM_BUTTONS; i++) 
    {
        free_irq(button_irqs[i], NULL);
        gpio_free(gpio_pins[i]);
    }
    printk(KERN_INFO "Inky Impression: Module unloaded successfully\n");
}

module_init(gpio_button_init);
module_exit(gpio_button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RetroZelda");
MODULE_DESCRIPTION("A simple driver for the Inky Impression GPIO buttons on Raspberry Pi");
