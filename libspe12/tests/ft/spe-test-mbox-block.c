/*
 * libspe - A wrapper library to adapt the JSRE SPU usage model to SPUFS
 * Copyright (C) 2005 IBM Corp.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 *  License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this library; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <spu_intrinsics.h>

int main(void)
{
	unsigned int data = 0xaabb;

	spu_writech(SPU_WrOutIntrMbox, data);
	
	si_stop(0x1188);

	data = 0xccdd;

	spu_writech(SPU_WrOutIntrMbox, data);
	
	si_stop(0x1189);
	return 0;
}
