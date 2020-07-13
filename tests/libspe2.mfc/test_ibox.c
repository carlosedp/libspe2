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

/* This test checks if the outbound interrupt mailbox support works
 * correctly.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <unistd.h>
#include <sys/time.h>

#include "ppu_libspe2_test.h"

#define COUNT 100000

extern spe_program_handle_t spu_ibox;

typedef struct test_params
{
  const char *name;
  unsigned int flags;
} test_params_t;

typedef struct spe_thread_params
{
  spe_context_ptr_t spe;
  ppe_thread_t *ppe;
} spe_thread_params_t;

static void *spe_thread_proc(void *arg)
{
  spe_thread_params_t *params = (spe_thread_params_t *)arg;
  unsigned int entry = SPE_DEFAULT_ENTRY;
  int ret;
  spe_stop_info_t stop_info;

  if (spe_program_load(params->spe, &spu_ibox)) {
    eprintf("spe_program_load(%p, &spu_ibox): %s\n", params->spe, strerror(errno));
    fatal();
  }

  /* synchronize all threads */
  global_sync(NUM_SPES * 2);

  ret = spe_context_run(params->spe, &entry, 0, (void*)COUNT, NULL, &stop_info);
  if (ret == 0) {
    if (check_exit_code(&stop_info, 0)) {
      fatal();
    }
  }
  else {
    eprintf("spe_context_run(%p, ...): %s\n", params->spe, strerror(errno));
    fatal();
  }

  return NULL;
}

static void *ppe_thread_proc(void *arg)
{
  ppe_thread_t *ppe = (ppe_thread_t*)arg;
  spe_thread_params_t spe_params;
  pthread_t tid;
  test_params_t *params = (test_params_t *)ppe->group->data;
  int ret;
  int i;
  unsigned int ibox_value;
  unsigned int data[IBOX_DEPTH + 1];

  spe_params.ppe = ppe;
  spe_params.spe = spe_context_create(params->flags, 0);
  if (!spe_params.spe) {
    eprintf("%s: spe_context_create(0x%08x, NULL): %s\n",
	    params->name, params->flags, strerror(errno));
    fatal();
  }

  ret = pthread_create(&tid, NULL, spe_thread_proc, &spe_params);
  if (ret) {
    eprintf("%s: pthread_create: %s\n", params->name, strerror(ret));
    fatal();
  }

  /* synchronize all threads */
  global_sync(NUM_SPES * 2);

  for (ibox_value = 0, i = 0; ibox_value < COUNT; i++) {
    int num_data;
    unsigned int behavior;
    int j;

    /* check outboud interrupt mailbox status */
    ret = spe_out_intr_mbox_status(spe_params.spe);
    if (ret == -1) {
      eprintf("%s: spe_out_intr_mbox_status: %s.\n", params->name, strerror(errno));
      fatal();
    }
    if (ret < 0 || ret > IBOX_DEPTH) {
      eprintf("%s: spe_out_intr_mbox_status: Unexpected # of messages (%d).\n",
	      params->name, ret);
      fatal();
    }

    /* choose behavior */
    switch (i % 3) {
    case 0:
      behavior = SPE_MBOX_ANY_BLOCKING;
      break;
    case 1:
      behavior = SPE_MBOX_ALL_BLOCKING;
      break;
    default:
      behavior = SPE_MBOX_ANY_NONBLOCKING;
      break;
    }

    /* determine how many data to be read at once */
    num_data = i % (IBOX_DEPTH + 1) + 1; /* 1 ... IBOX_DEPTH + 1 */
    if (num_data > COUNT - ibox_value)
      num_data = COUNT - ibox_value;

    /* read outbound mailbox */
    ret = spe_out_intr_mbox_read(spe_params.spe, data, num_data, behavior);
    if (ret == -1) {
      eprintf("%s: spe_out_intr_mbox_read(%p, %p, %u, %s): %s.\n",
	      params->name, spe_params.spe, data, num_data, mbox_behavior_symbol(behavior),
	      strerror(errno));
      fatal();
    }
    if (ret > num_data) {
      eprintf("ibox: Too many (%u -> %u) ibox data.\n", num_data, ret);
      fatal();
    }
    switch (behavior) {
    case SPE_MBOX_ANY_BLOCKING:
      if (ret == 0) {
	eprintf("ibox: No ibox data.\n");
	fatal();
      }
      break;
    case SPE_MBOX_ALL_BLOCKING:
      if (ret != num_data) {
	eprintf("ibox: Too few (%u -> %u) ibox data.\n", num_data, ret);
	fatal();
      }
      break;
    default:
      break;
    }

    /* check data */
    for (j = 0; j < ret; j++) {
      if (data[j] != ibox_value + 1) {
	eprintf("ibox: Unexpected data: %u\n", data[j]);
	fatal();
      }
      ibox_value++;
    }
  }

  pthread_join(tid, NULL);
  ret = spe_context_destroy(spe_params.spe);
  if (ret) {
    eprintf("%s: spe_context_destroy: %s\n", params->name, strerror(errno));
    fatal();
  }

  return NULL;
}

static int test(int argc, char **argv)
{
  int ret;
  test_params_t params;

  params.name = "default";
  params.flags = 0;
  ret = ppe_thread_group_run(NUM_SPES, ppe_thread_proc, &params, NULL);
  if (ret) {
    fatal();
  }

  params.name = "SPE_MAP_PS";
  params.flags = SPE_MAP_PS;
  ret = ppe_thread_group_run(NUM_SPES, ppe_thread_proc, &params, NULL);
  if (ret) {
    fatal();
  }

  return 0;
}

int main(int argc, char **argv)
{
  return ppu_main(argc, argv, test);
}
