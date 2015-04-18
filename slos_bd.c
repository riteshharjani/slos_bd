/*
 * sample updated block device driver (3.10 kernel)
 *
 * inspired from https://lwn.net/Articles/58719/
 * which was based on 2.6 kernel.
 *
 * Copyright (C) 2015 Ritesh Harjani <ritesh.list@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * Redistributable under the terms of the GNU GPL.
 *
 *
 */

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

#define SLOS_BD_NAME "slos"

static int major		= 0;
static int nr_sectors	= 2048;
static int blk_size		= 512;
static int nr_parts		= 16;

struct slos_bd {
	spinlock_t lock;
	unsigned long size;
	struct gendisk *disk;
	struct request_queue *queue;
	u8 *data; /* byte access imp */
};
static struct slos_bd bd;

static void slos_data_transfer(struct slos_bd *bd, sector_t sector,
			unsigned long nr_sect, char *buff, int write)
{
	unsigned long offset = sector * blk_size;
	unsigned long size = nr_sect * blk_size;

	if (offset + size > bd->size) {
		pr_err("slos_bd: Write data beyond supported size\n");
		return;
	}
	if (write)
		memcpy(bd->data + offset, buff, size);
	else
		memcpy(buff, bd->data + offset, size);
}

static void slos_bd_request(struct request_queue *q)
{
  struct request *req;
	struct slos_bd *bd = q->queuedata;

	while ((req = blk_fetch_request(q)) != NULL) {
		if (req->cmd_type != REQ_TYPE_FS) {
			pr_crit("slos_bd: req->cmdq_type != REQ_TYPE_FS\n");
			__blk_end_request_all(req, -EIO);
			continue;
		}
		slos_data_transfer(bd, blk_rq_pos(req), blk_rq_cur_sectors(req),
				req->buffer, rq_data_dir(req));
		__blk_end_request(req, 0, blk_rq_bytes(req));
		pr_crit("slos_bd: req->cmdq_type == REQ_TYPE_FS\n");
		}
}

int slos_bd_getgeo(struct block_device *bdev,
		struct hd_geometry *geo)
{
//	geo->cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
	long size = bd.size;
	geo->cylinders = (size & ~0x3f) >> 6;
	geo->heads = 4;
	geo->sectors = 16;
	geo->start = 0;
	return 0;
}

static const struct block_device_operations slos_bd_fops = {
	.owner			= THIS_MODULE,
	.getgeo			= slos_bd_getgeo
};

static int __init slos_bd_init(void)
{
	int ret = 0;

	bd.size = nr_sectors * blk_size;
	bd.data = vmalloc(bd.size);
	if (!bd.data) {
		ret = -ENOMEM;
		return ret;
	}

	major = register_blkdev(major, SLOS_BD_NAME);
	if (major < 0) {
		pr_err("slos_bd: Unable to allocate major number\n");
		ret = -ENOMEM;
		goto out;
	}

	spin_lock_init(&bd.lock);
	bd.disk = alloc_disk(nr_parts);
	if (!bd.disk) {
		pr_err("slos_bd: Unable to allocate gendisk\n");
		unregister_blkdev(major, SLOS_BD_NAME);
		ret = -ENOMEM;
		goto out;
	}


	bd.queue = blk_init_queue(slos_bd_request, &bd.lock);
	if (!bd.queue) {
		ret = -ENOMEM;
		del_gendisk(bd.disk);
		put_disk(bd.disk);
		unregister_blkdev(major, SLOS_BD_NAME);
		goto out;
	}

	blk_queue_logical_block_size(bd.queue, blk_size);
	bd.queue->queuedata = &bd;

	bd.disk->major = major;
	bd.disk->first_minor = 0;
	bd.disk->fops = &slos_bd_fops;
	bd.disk->private_data = &bd;
	bd.disk->queue = bd.queue;
	strcpy(bd.disk->disk_name, SLOS_BD_NAME);
	set_capacity(bd.disk, nr_sectors);

	add_disk(bd.disk);
	return 0;
out:
	vfree(bd.data);
	return ret;
}

static void __exit slos_bd_exit(void)
{
	del_gendisk(bd.disk);
	put_disk(bd.disk);
	unregister_blkdev(major, SLOS_BD_NAME);
	blk_cleanup_queue(bd.queue);
	vfree(bd.data);
}

module_init(slos_bd_init);
module_exit(slos_bd_exit);
MODULE_LICENSE("GPL");
