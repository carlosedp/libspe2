/*
 *  libspe2 - A wrapper library to adapt the JSRE SPU usage model to SPUFS
 *
 *  Copyright (C) 2007 Sony Computer Entertainment Inc.
 *  Copyright 2007 Sony Corp.
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

/* This program calls a user-defined PPE assisted call.
 */

#include <spu_intrinsics.h>

#include "spu_libspe2_test.h"

int main(unsigned long long spe,
	 unsigned long long argp, unsigned long long envp)
{
  __vector unsigned int prog = {
    (unsigned int)(argp + 0x2100), /* stop instruction */
    PPE_ASSISTED_CALL_DATA,        /* test data */
    0x4020007f, /* nop */
    0x35000000  /* bi $0 */
  };

  asm("sync");
  ((void (*)())&prog)();

  return 0;
}
