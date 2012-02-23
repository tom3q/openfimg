#
# Vertex shader code blocks for fixed pipeline emulation
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
vs_3_0

# FIMG version >= 1.5
fimg_version	0x01050000

% v cfloat

# Transformation matrix
# def c0, 1.0, 0.0, 0.0, 0.0
# def c1, 0.0, 1.0, 0.0, 0.0
# def c2, 0.0, 0.0, 1.0, 0.0
# def c3, 0.0, 0.0, 0.0, 1.0

# Lighting matrix
# def c4, 1.0, 0.0, 0.0, 0.0
# def c5, 0.0, 1.0, 0.0, 0.0
# def c6, 0.0, 0.0, 1.0, 0.0
# def c7, 0.0, 0.0, 0.0, 1.0

# Texture 0 matrix
# def c8,  1.0, 0.0, 0.0, 0.0
# def c9,  0.0, 1.0, 0.0, 0.0
# def c10, 0.0, 0.0, 1.0, 0.0
# def c11, 0.0, 0.0, 0.0, 1.0

# Texture 1 matrix
# def c12, 1.0, 0.0, 0.0, 0.0
# def c13, 0.0, 1.0, 0.0, 0.0
# def c14, 0.0, 0.0, 1.0, 0.0
# def c15, 0.0, 0.0, 0.0, 1.0

% v header

# Shader header
label start
	# Transform position by transformation matrix
	mul r0.xyzw, c0.xyzw, v0.xxxx
	mad r0.xyzw, c1.xyzw, v0.yyyy, r0.xyzw
	mad r0.xyzw, c2.xyzw, v0.zzzz, r0.xyzw
	mad o0.xyzw, c3.xyzw, v0.wwww, r0.xyzw

	# Pass vertex color
	mov o1, v2

# Code is being inserted here dynamically

################################################################################

% v texture0

# Texture 0
	# Transform texture0 coordinates
	mul r1.xyzw, c8.xyzw,  v4.xxxx
	mad r1.xyzw, c9.xyzw,  v4.yyyy, r1.xyzw
	mad r1.xyzw, c10.xyzw, v4.zzzz, r1.xyzw
	mad o2.xyzw, c11.xyzw, v4.wwww, r1.xyzw

% v texture1

# Texture 1
	# Transform texture1 coordinates
	mul r2.xyzw, c12.xyzw, v5.xxxx
	mad r2.xyzw, c13.xyzw, v5.yyyy, r2.xyzw
	mad r2.xyzw, c14.xyzw, v5.zzzz, r2.xyzw
	mad o3.xyzw, c15.xyzw, v5.wwww, r2.xyzw

################################################################################

% v footer

# Shader footer
	# Return
	ret

# End of shader code
