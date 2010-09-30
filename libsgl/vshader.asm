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

	# Skip if texture0 is disabled
	bf notexture0, b0

	# Transform texture0 coordinates
	mul r1.xyzw, c8.xyzw,  v4.xxxx
	mad r1.xyzw, c9.xyzw,  v4.yyyy, r1.xyzw
	mad r1.xyzw, c10.xyzw, v4.zzzz, r1.xyzw
	mad o4.xyzw, c11.xyzw, v4.wwww, r1.xyzw

label notexture0
	# Skip if texture1 is disabled
	bf notexture1, b1

	# Transform texture1 coordinates
	mul r2.xyzw, c12.xyzw, v5.xxxx
	mad r2.xyzw, c13.xyzw, v5.yyyy, r2.xyzw
	mad r2.xyzw, c14.xyzw, v5.zzzz, r2.xyzw
	mad o5.xyzw, c15.xyzw, v5.wwww, r2.xyzw

label notexture1
	# Return
	ret
# End of shader code
