/* g3d/s3c_g3d.h
 *
 * Copyright (c) 2010 Tomasz Figa <tomasz.figa at gmail.com>
 *
 * Samsung S3C G3D driver
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _S3C_G3D_H_
#define _S3C_G3D_H_

#include <linux/ioctl.h>

#define G3D_IOCTL_MAGIC			'S'

/*
 * S3C_G3D_LOCK
 * Claim the hardware for exclusive access.
 * Returns:	> 0, when context restoring is needed,
 * 		  0, if not,
 * 		< 0, on error
 */
#define S3C_G3D_LOCK			_IO(G3D_IOCTL_MAGIC, 0)
/*
 * S3C_G3D_UNLOCK
 * Release the hardware.
 */
#define S3C_G3D_UNLOCK			_IO(G3D_IOCTL_MAGIC, 1)
/*
 * S3C_G3D_FLUSH
 * Flush hardware pipeline to the requested state.
 * Argument:	Requested pipeline value. (as in FGGB_PIPESTATE)
 * Returns:	0, on success,
 * 		< 0, on error
 */
#define S3C_G3D_FLUSH			_IO(G3D_IOCTL_MAGIC, 2)

#endif
