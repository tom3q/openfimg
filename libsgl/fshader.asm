# Fragment shader code for fixed pipeline emulation
# S3C6410 FIMG-3DSE v.1.5
#
# Copyright 2010 Tomasz Figa <tomasz.figa@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# Fragment shader version
ps_3_0

# FIMG version >= 1.2
fimg_version	0x01020000

# Shader code
label start
	# Get fragment color
	mov r0, v1

	# Skip if texture0 is disabled
	bf notexture0, b0

	# Get texel value from texture0
	texld r1, v3, s0
	mul r0, r0, r1

label notexture0
	# Skip if texture1 is disabled
	bf notexture1, b1

	# Get texel value from texture1
	texld r1, v4, s1
	mul r0, r0, r1

label notexture1
	# Store the pixel
	mov_sat oColor, r0

	# Return
	ret
# End of shader code
