/*
 *  libspe2 - A wrapper library to adapt the JSRE SPU usage model to SPUFS
 *
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  Copyright 2007,2008 Sony Corp.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "spu_libspe2_test.h"
#include <spu_mfcio.h>

#define NUM_DMA MFC_COMMAND_QUEUE_DEPTH
#define DMA_SIZE MAX_DMA_SIZE
#define TAG 1

static unsigned char buf[DMA_SIZE] __attribute__((aligned(16)));

int main(unsigned long long spe,
	 unsigned long long argp /* loop */,
	 unsigned long long envp /* EA */ )
{
  unsigned long long i;

  for (i = 0; i < argp; i++) {
    unsigned long long ea = envp;
    int j;

    /* issue DMA */
    for (j = 0; j < NUM_DMA; j++) {
      mfc_getf(buf, ea, DMA_SIZE, TAG, 0, 0);
      ea += DMA_SIZE;
    }

    /* stop here to increase probability of context switch with
       pending DMA. */
    spu_stop(1);

    /* wait for completion */
    mfc_write_tag_mask(1 << TAG);
    mfc_read_tag_status_all();

    if (!data_check(buf, DMA_SIZE * (NUM_DMA - 1), DMA_SIZE)) {
      /* data mismatch */
      return 1;
    }
  }

  return 0;
}
