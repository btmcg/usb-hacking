// SPDX-License-Identifier: GPL-2.0
/*
 * Delcom VI Driver
 *
 * Copyright (C) 2020 Brian McGuire
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/usb.h>


/* clang-format off */
#define DRIVER_AUTHOR   "Brian McGuire"
#define DRIVER_DESC     "Delcom VI Driver"

#define VENDOR_ID   0x0fc5
#define PRODUCT_ID  0xb080
#define MAXLEN      8

/* table of devices that work with this driver */
static const struct usb_device_id id_table[] = {
    { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
    {},
};
MODULE_DEVICE_TABLE(usb, id_table);
/* clang-format on */


struct usb_leddev
{
    struct usb_device* dev;
    struct usb_interface* iface;
};

static int
led_probe(struct usb_interface* interface, const struct usb_device_id* id)
{
    struct usb_device* usb_dev = NULL;
    struct usb_leddev* led_dev = kzalloc(sizeof(struct usb_leddev), GFP_KERNEL);
    if (!led_dev)
        return -ENOMEM;

    usb_dev = interface_to_usbdev(interface);
    led_dev->dev = usb_get_dev(usb_dev);
    led_dev->iface = interface;
    usb_set_intfdata(interface, led_dev);

    dev_info(&interface->dev, "Delcom USB VI device now connected\n");
    return 0;
}

static void
led_disconnect(struct usb_interface* interface)
{
    struct usb_leddev* led_dev = usb_get_intfdata(interface);

    usb_set_intfdata(interface, NULL);
    usb_put_dev(led_dev->dev);
    kfree(led_dev);

    dev_info(&interface->dev, "Delcom USB VI now disconnected\n");
}


/* clang-format off */
static struct usb_driver led_driver = {
    .name       = "delcom",
    .probe      = led_probe,
    .disconnect = led_disconnect,
    .id_table   = id_table,
};
/* clang-format on */

module_usb_driver(led_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
