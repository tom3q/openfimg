#
# Fragment shader code blocks for fixed pipeline emulation
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

# FIMG version >= 1.5
fimg_version	0x01050000

% f cfloat

# 0.0 constant
def c0, 0.0, 0.0, 0.0, 0.0
# 1.0 constant
def c1, 1.0, 1.0, 1.0, 1.0
# 0.5 constant
def c2, 0.5, 0.5, 0.5, 0.5
# 4.0 constant
def c3, 4.0, 4.0, 4.0, 4.0

# Texture environment color 0
# def c4, 0.0, 0.0, 0.0, 0.0
# Combiner scale 0
# def c5, 1.0, 1.0, 1.0, 1.0

# Texture environment color 1
# def c6, 0.0, 0.0, 0.0, 0.0
# Combiner scale 1
# def c7, 1.0, 1.0, 1.0, 1.0

% f header

# Shader header
label start
	# Get fragment color
	mov r0, v0

# Code is being inserted here dynamically

################################################################################

% f texture0

# Sampling function
#
# Output:	r1 - texture value

# Texture 0
	texld r1, v1, s0
	bf noswap0, b0
	mov r1.xyzw, r1.wzyx
label noswap0
	mov r2, c4
	mov r3, c5

% f texture1

# Sampling function
#
# Output:	r1 - texture value

# Texture 1
	texld r1, v2, s1
	bf noswap1, b1
	mov r1.xyzw, r1.wzyx
label noswap1
	mov r2, c6
	mov r3, c7

################################################################################

% f replace

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Replace
	mov r0, r1

% f modulate

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Modulate
	mul r0, r0, r1

% f decal

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Decal
	add r3.w, c1, -r1
	mul r0.xyz, r0, r3.w
	mad r0.xyz, r1, r1.w, r0

% f blend

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r4 - combiner scale
#
# Output:	r0 - new fragment value

# Blend
	mov r4.w, r1
	add r4.xyz, c1, -r1
	mul r0, r0, r4
	mad r0.xyz, c4, r1, r0

% f add

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Add
	mul r0.w, r0, r1
	add r0.xyz, r0, r1

% f combine

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Combine
	# Do nothing

% f combine_col

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Color combiner
	# Here go argument functions for color combiner
	# Here go argument modifier functions for color combiner
	# Here goes combine function for color combiner

	mul_sat r0.xyz, r4, r3

% f combine_a

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Alpha combiner
	# Here go argument functions for alpha combiner
	# Here go argument modifier functions for alpha combiner
	# Here goes combine function for alpha combiner

	mul_sat r0.w, r4, r3

% f combine_uni

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r0 - new fragment value

# Unified combiner
	# Here go argument functions for unified combiner
	# Here go argument modifier functions for unified combiner
	# Here goes combine function for unified combiner

	mul_sat r0, r4, r3

################################################################################

% f combine_arg0tex

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r4 - argument 0

# Texture
	mov r4, r1

% f combine_arg0const

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r4 - argument 0

# Constant
	mov r4, r2

% f combine_arg0col

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r4 - argument 0

# Primary color
	mov r4, v0

% f combine_arg0prev

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#
# Output:	r4 - argument 0

# Previous
	mov r4, r0

################################################################################

% f combine_arg1tex

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r5 - argument 1

# Texture
	mov r5, r1

% f combine_arg1const

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r5 - argument 1

# Constant
	mov r5, r2

% f combine_arg1col

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r5 - argument 1

# Primary color
	mov r5, v0

% f combine_arg1prev

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r5 - argument 1

# Previous
	mov r5, r0

################################################################################

% f combine_arg2tex

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r6 - argument 2

# Texture
	mov r6, r1

% f combine_arg2const

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r6 - argument 2

# Constant
	mov r6, r2

% f combine_arg2col

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r6 - argument 2

# Primary color
	mov r6, v0

% f combine_arg2prev

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - environment color
#		r3 - combiner scale
#
# Output:	r6 - argument 2

# Previous
	mov r6, r0

################################################################################

% f combine_arg0sc

# Combine argument 0 modifier
#
# Inputs:	r4 - argument 0
#
# Output:	r4 - modified argument 0

# Source color
	# Do nothing

% f combine_arg0omsc

# Combine argument 0 modifier
#
# Inputs:	r4 - argument 0
#
# Output:	r4 - modified argument 0

# One minus source color
	add r4, c1, -r4

% f combine_arg0sa

# Combine argument 0 modifier
#
# Inputs:	r4 - argument 0
#
# Output:	r4 - modified argument 0

# Source alpha
	mov r4, r4.w

% f combine_arg0omsa

# Combine argument 0 modifier
#
# Inputs:	r4 - argument 0
#
# Output:	r4 - modified argument 0

# One minus source alpha
	add r4, c1, -r4.w

################################################################################

% f combine_arg1sc

# Combine argument 1 modifier
#
# Inputs:	r5 - argument 1
#
# Output:	r5 - modified argument 1

# Source color
	# Do nothing

% f combine_arg1omsc

# Combine argument 1 modifier
#
# Inputs:	r5 - argument 1
#
# Output:	r5 - modified argument 1

# One minus source color
	add r5, c1, -r5

% f combine_arg1sa

# Combine argument 1 modifier
#
# Inputs:	r5 - argument 1
#
# Output:	r5 - modified argument 1

# Source alpha
	mov r5, r5.w

% f combine_arg1omsa

# Combine argument 1 modifier
#
# Inputs:	r5 - argument 1
#
# Output:	r5 - modified argument 1

# One minus source alpha
	add r5, c1, -r5.w

################################################################################

% f combine_arg2sc

# Combine argument 2 modifier
#
# Inputs:	r6 - argument 2
#
# Output:	r6 - modified argument 2

# Source color
	# Do nothing

% f combine_arg2omsc

# Combine argument 2 modifier
#
# Inputs:	r6 - argument 2
#
# Output:	r6 - modified argument 2

# One minus source color
	add r6, c1, -r6

% f combine_arg2sa

# Combine argument 2 modifier
#
# Inputs:	r6 - argument 2
#
# Output:	r6 - modified argument 2

# Source alpha
	mov r6, r6.w

% f combine_arg2omsa

# Combine argument 2 modifier
#
# Inputs:	r6 - argument 2
#
# Output:	r6 - modified argument 2

# One minus source alpha
	add r6, c1, -r6.w

################################################################################

% f combine_replace

# Combine function
#
# Inputs:	r4 - argument 0
#		r5 - argument 1
#		r6 - argument 2
#
# Output:	r4 - result

# Replace
	# Do nothing

% f combine_modulate

# Combine function
#
# Inputs:	r4 - argument 0
#		r5 - argument 1
#		r6 - argument 2
#
# Output:	r4 - result

# Modulate
	mul r4, r4, r5

% f combine_add

# Combine function
#
# Inputs:	r4 - argument 0
#		r5 - argument 1
#		r6 - argument 2
#
# Output:	r4 - result

# Add
	add r4, r4, r5

% f combine_adds

# Combine function
#
# Inputs:	r4 - argument 0
#		r5 - argument 1
#		r6 - argument 2
#
# Output:	r4 - result

# Add signed
	add r4, r4, r5
	add r4, r4, -c2

% f combine_interpolate

# Combine function
#
# Inputs:	r4 - argument 0
#		r5 - argument 1
#		r6 - argument 2
#
# Output:	r4 - result

# Interpolate
	mul r4, r4, r6
	add r6, c1, -r6
	mad r4, r5, r6, r4

% f combine_subtract

# Combine function
#
# Inputs:	r4 - argument 0
#		r5 - argument 1
#		r6 - argument 2
#
# Output:	r4 - result

# Subtract
	add r4, r4, -r5

% f combine_dot3

# Combine function
#
# Inputs:	r4 - argument 0
#		r5 - argument 1
#		r6 - argument 2
#
# Output:	r4 - result

# Dot 3
	add r4, r4, -c2
	add r5, r5, -c2
	dp3 r4, r4, r5
	mul r4, r4, c3

################################################################################

% f footer

# Shader footer
	# Emit the pixel
	mov oColor, r0
	# Return
	ret

################################################################################

% f clear

# Shader for glClear
	mov_sat oColor, c255
	ret

# End of shader code
