/*
 * libspe2 - A wrapper library to adapt the JSRE SPU usage model to SPUFS
 * Copyright (C) 2005 IBM Corp.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "elf_loader.h"
#include "spebase.h"

/**
 * Send data to a SPU in mbox when space is available.
 *
 * Helper function for internal libspe use.
 *
 * @param spe The SPE thread to send the mailbox message to
 * @param data  Data to send to the mailbox
 * @return zero on success, non-zero on failure
 */
static inline int __write_mbox_sync(struct spe_context *spe, unsigned int data)
{
	while (_base_spe_in_mbox_status(spe) == 0);
	return _base_spe_in_mbox_write(spe, &data, 1,
			SPE_MBOX_ALL_BLOCKING) != 1;
}

/**
 * Initiate transfer of an isolated SPE app by the loader kernel.
 *
 * Helper function for internal libspe use.
 *
 * @param thread The SPE thread to load the app to
 * @param handle The handle to the spe program
 * @return zero on success, non-zero on failure;
 */
static int spe_start_isolated_app(struct spe_context *spe,
		spe_program_handle_t *handle)
{
	uint64_t addr;
	uint32_t size, addr_h, addr_l;

	if (spe_parse_isolated_elf(handle, &addr, &size)) {
		DEBUG_PRINTF("%s: invalid isolated image\n", __FUNCTION__);
		errno = ENOEXEC;
		return -errno;
	}

	if (addr & 0xf) {
		DEBUG_PRINTF("%s: isolated image is incorrectly aligned\n",
				__FUNCTION__);
		errno = EINVAL;
		return -errno;
	}

	addr_l = (uint32_t)(addr & 0xffffffff);
	addr_h = (uint32_t)(addr >> 32);

	DEBUG_PRINTF("%s: Sending isolated app params: 0x%08x 0x%08x 0x%08x\n",
			__FUNCTION__, addr_h, addr_l, size);

	if (__write_mbox_sync(spe, addr_h) ||
		__write_mbox_sync(spe, addr_l) ||
		__write_mbox_sync(spe, size)) {
		errno = EIO;
		return -errno;
	}

	return 0;
}

int _base_spe_program_load(spe_context_ptr_t spe, spe_program_handle_t *program)
{
	int rc = 0, objfd;
	struct spe_ld_info ld_info;

	if (spe->base_private->flags & SPE_ISOLATE) {
		rc = spe_start_isolated_app(spe, program);
	} else {
		rc = load_spe_elf(program, spe->base_private->mem_mmap_base,
				&ld_info);
	}

	if (rc != 0) {
		DEBUG_PRINTF ("Load SPE ELF failed..\n");
		return -1;
	}

	/* Register SPE image start address as "object-id" for oprofile.  */
	objfd = openat (spe->base_private->fd_spe_dir,"object-id", O_RDWR);
	if (objfd >= 0) {
		char buf[100];
		sprintf (buf, "%p", program->elf_image);
		write (objfd, buf, strlen (buf) + 1);
		close (objfd);
	}
	
	__spe_context_update_event();	

	spe->base_private->entry=ld_info.entry;
	
	return 0;
}
