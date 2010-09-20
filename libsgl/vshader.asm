# Vertex shader code for fixed pipeline emulation
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

# Vertex shader version
vs_3_0

# FIMG version >= 1.2
fimg_version	0x01020000

# Shader code
label start
	# Transform position by transformation matrix
	mul r0.xyzw, c0.xyzw, v0.xxxx
	mad r0.xyzw, c1.xyzw, v0.yyyy, r0.xyzw
	mad r0.xyzw, c2.xyzw, v0.zzzz, r0.xyzw
	mad o0.xyzw, c3.xyzw, v0.wwww, r0.xyzw

	# Pass vertex normal
	mov o1, v1

	# Pass vertex color
	mov o2, v2

	# Pass vertex point size
	mov o3, v3

	# Pass vertex texcoord 0
	mov o4, v4

	# Pass vertex texcoord 1
	mov o5, v5

	# Return
	ret
# End of shader code
