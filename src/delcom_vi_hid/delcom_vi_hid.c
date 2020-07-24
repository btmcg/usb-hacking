// SPDX-License-Identifier: GPL-2.0
/*
 * Delcom VI Driver
 *
 * Copyright (C) 2020 Brian McGuire
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/usb.h>


/* clang-format off */
#define DRIVER_AUTHOR   "Brian McGuire"
#define DRIVER_DESC     "Delcom VI Driver"

#define VENDOR_ID   0x0fc5
#define PRODUCT_ID  0xb080

/* table of devices that work with this driver */
static const struct usb_device_id id_table[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {},
};
MODULE_DEVICE_TABLE(usb, id_table);
/* clang-format on */


struct usb_leddev
{
    struct usb_device* udev;
    struct usb_interface* interface;
};

static int
led_probe(struct usb_interface* interface, const struct usb_device_id* id)
{
    struct usb_leddev* led_dev = kzalloc(sizeof(struct usb_leddev), GFP_KERNEL);
    if (!led_dev)
        return -ENOMEM;

    led_dev->udev = usb_get_dev(interface_to_usbdev(interface));
    led_dev->interface = usb_get_intf(interface);

    /* save our data pointer in this interface device */
    usb_set_intfdata(interface, led_dev);

    /* we can register the device now, as it is ready */
    /* retval = usb_register_dev(interface, &skel_class); */
    /* if (retval) { */
    /*     /1* something prevented us from registering this driver *1/ */
    /*     dev_err(&interface->dev, "Not able to get a minor for this device.\n"); */
    /*     usb_set_intfdata(interface, NULL); */
    /*     goto error; */
    /* } */


    dev_info(&interface->dev, "Delcom USB VI device now connected\n");
    return 0;
}

static void
led_disconnect(struct usb_interface* interface)
{
    struct usb_leddev* led_dev = usb_get_intfdata(interface);

    usb_set_intfdata(interface, NULL);
    usb_put_dev(led_dev->udev);
    kfree(led_dev);

    dev_info(&interface->dev, "Delcom USB VI now disconnected\n");
}

static int
led_suspend(struct usb_interface* interface, pm_message_t message)
{
    /* struct usb_skel* dev = usb_get_intfdata(intf); */

    /* if (!dev) */
    /*     return 0; */
    /* skel_draw_down(dev); */
    dev_info(&interface->dev, "Delcom USB VI device suspend called\n");
    return 0;
}

static int
led_resume(struct usb_interface* interface)
{
    dev_info(&interface->dev, "Delcom USB VI device resume called\n");
    return 0;
}

static int
led_pre_reset(struct usb_interface* interface)
{
    /* struct usb_skel* dev = usb_get_intfdata(intf); */

    /* mutex_lock(&dev->io_mutex); */
    /* skel_draw_down(dev); */

    dev_info(&interface->dev, "Delcom USB VI device pre_reset called\n");
    return 0;
}

static int
led_post_reset(struct usb_interface* interface)
{
    /* struct usb_skel* dev = usb_get_intfdata(intf); */

    /* /1* we are sure no URBs are active - no locking needed *1/ */
    /* dev->errors = -EPIPE; */
    /* mutex_unlock(&dev->io_mutex); */

    dev_info(&interface->dev, "Delcom USB VI device post_reset called\n");
    return 0;
}

/* clang-format off */
static struct usb_driver led_driver = {
    .name       = "delcom_vi_hid",
    .probe      = led_probe,
    .disconnect = led_disconnect,
    .suspend    = led_suspend,
    .resume     = led_resume,
    .pre_reset  = led_pre_reset,
    .post_reset = led_post_reset,
    .id_table   = id_table,
    .supports_autosuspend = 0,
};
/* clang-format on */

module_usb_driver(led_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");
