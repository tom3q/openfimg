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

%cfloat

# 0.0 constant
def c0, 0.0, 0.0, 0.0, 0.0
# 1.0 constant
def c1, 1.0, 1.0, 1.0, 1.0
# 0.5 constant
def c2, 0.5, 0.5, 0.5, 0.5
# 4.0 constant
def c3, 4.0, 4.0, 4.0, 4.0

# Texture environment color
def c4, 0.0, 0.0, 0.0, 0.0
# Alpha combiner scale
def c5, 1.0, 1.0, 1.0, 1.0
# Color combiner scale
def c6, 1.0, 1.0, 1.0, 1.0

%header

# Shader header
label start
	# Get fragment color
	mov r2, v1
	mov r0, v1

# Code is being inserted here dynamically

################################################################################

%texture0

# Sampling function
#
# Output:	r1 - texture value

# Texture 0
	texld r1, v3, s0

%texture1

# Sampling function
#
# Output:	r1 - texture value

# Texture 1
	texld r1, v4, s1

################################################################################

%replace

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r0 - new fragment value

# Replace
	mov r0, r1

%modulate

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r0 - new fragment value

# Modulate
	mul r0, r0, r1

%decal

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r0 - new fragment value

# Decal
	add r3.w, c1, -r1
	mul r0.xyz, r0, r3.w
	mad r0.xyz, r1, r1.w, r0

%blend

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r0 - new fragment value

# Blend
	mov r3.w, r1
	add r3.xyz, c1, -r1
	mul r0, r0, r3
	mad r0.xyz, c4, r1, r0

%add

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r0 - new fragment value

# Add
	mul r0.w, r0, r1
	add r0.xyz, r0, r1

%combine_a

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r0 - new fragment value

# Alpha combiner
	# Here go argument functions for alpha combiner
	# Here go argument modifier functions for alpha combiner
	# Here goes combine function for alpha combiner

	mul_sat r0.w, r3, c5

%combine_col

# Texturing function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r0 - new fragment value

# Color combiner
	# Here go argument functions for color combiner
	# Here go argument modifier functions for color combiner
	# Here goes combine function for color combiner

	mul_sat r0.xyz, r3, c6

################################################################################

%combine_arg0tex

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r3 - argument 0

# Texture
	mov r3, r1

%combine_arg0const

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r3 - argument 0

# Constant
	mov r3, c4

%combine_arg0col

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r3 - argument 0

# Primary color
	mov r3, r2

%combine_arg0prev

# Combine argument 0 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r3 - argument 0

# Previous
	mov r3, r0

################################################################################

%combine_arg1tex

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r4 - argument 1

# Texture
	mov r4, r1

%combine_arg1const

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r4 - argument 1

# Constant
	mov r4, c4

%combine_arg1col

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r4 - argument 1

# Primary color
	mov r4, r2

%combine_arg1prev

# Combine argument 1 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r4 - argument 1

# Previous
	mov r4, r0

################################################################################

%combine_arg2tex

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r5 - argument 2

# Texture
	mov r5, r1

%combine_arg2const

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r5 - argument 2

# Constant
	mov r5, c4

%combine_arg2col

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r5 - argument 2

# Primary color
	mov r5, r2

%combine_arg2prev

# Combine argument 2 function
#
# Inputs:	r0 - current fragment value
#		r1 - texture value
#		r2 - original fragment value
#
# Output:	r5 - argument 2

# Previous
	mov r5, r0

################################################################################

%combine_arg0_sc

# Combine argument 0 modifier
#
# Inputs:	r3 - argument 0
#
# Output:	r3 - modified argument 0

# Source color
	# Do nothing

%combine_arg0_omsc

# Combine argument 0 modifier
#
# Inputs:	r3 - argument 0
#
# Output:	r3 - modified argument 0

# One minus source color
	add r3, c1, -r3

%combine_arg0_sa

# Combine argument 0 modifier
#
# Inputs:	r3 - argument 0
#
# Output:	r3 - modified argument 0

# Source alpha
	mov r3, r3.w

%combine_arg0_omsa

# Combine argument 0 modifier
#
# Inputs:	r3 - argument 0
#
# Output:	r3 - modified argument 0

# One minus source alpha
	add r3, c1, -r3.w

################################################################################

%combine_arg1_sc

# Combine argument 1 modifier
#
# Inputs:	r4 - argument 1
#
# Output:	r4 - modified argument 1

# Source color
	# Do nothing

%combine_arg1_omsc

# Combine argument 1 modifier
#
# Inputs:	r4 - argument 1
#
# Output:	r4 - modified argument 1

# One minus source color
	add r4, c1, -r4

%combine_arg1_sa

# Combine argument 1 modifier
#
# Inputs:	r4 - argument 1
#
# Output:	r4 - modified argument 1

# Source alpha
	mov r4, r4.w

%combine_arg1_omsa

# Combine argument 1 modifier
#
# Inputs:	r4 - argument 1
#
# Output:	r4 - modified argument 1

# One minus source alpha
	add r4, c1, -r4.w

################################################################################

%combine_arg2_sc

# Combine argument 2 modifier
#
# Inputs:	r5 - argument 2
#
# Output:	r5 - modified argument 2

# Source color
	# Do nothing

%combine_arg2_omsc

# Combine argument 2 modifier
#
# Inputs:	r5 - argument 2
#
# Output:	r5 - modified argument 2

# One minus source color
	add r5, c1, -r5

%combine_arg2_sa

# Combine argument 2 modifier
#
# Inputs:	r5 - argument 2
#
# Output:	r5 - modified argument 2

# Source alpha
	mov r5, r5.w

%combine_arg2_omsa

# Combine argument 2 modifier
#
# Inputs:	r5 - argument 2
#
# Output:	r5 - modified argument 2

# One minus source alpha
	add r5, c1, -r5.w

################################################################################

%combine_replace

# Combine function
#
# Inputs:	r3 - argument 0
#		r4 - argument 1
#		r5 - argument 2
#
# Output:	r3 - result

# Replace
	# Do nothing

%combine_modulate

# Combine function
#
# Inputs:	r3 - argument 0
#		r4 - argument 1
#		r5 - argument 2
#
# Output:	r3 - result

# Modulate
	mul r3, r3, r4

%combine_add

# Combine function
#
# Inputs:	r3 - argument 0
#		r4 - argument 1
#		r5 - argument 2
#
# Output:	r3 - result

# Add
	add r3, r3, r4

%combine_adds

# Combine function
#
# Inputs:	r3 - argument 0
#		r4 - argument 1
#		r5 - argument 2
#
# Output:	r3 - result

# Add signed
	add r3, r3, r4
	add r3, r3, -c2

%combine_intepolate

# Combine function
#
# Inputs:	r3 - argument 0
#		r4 - argument 1
#		r5 - argument 2
#
# Output:	r3 - result

# Interpolate
	mul r3, r3, r5
	add r5, c1, -r5
	mad r3, r4, r5, r3

%combine_subtract

# Combine function
#
# Inputs:	r3 - argument 0
#		r4 - argument 1
#		r5 - argument 2
#
# Output:	r3 - result

# Subtract
	add r3, r3, -r4

%combine_dot3

# Combine function
#
# Inputs:	r3 - argument 0
#		r4 - argument 1
#		r5 - argument 2
#
# Output:	r3 - result

# Dot 3
	add r3, r3, -c2
	add r4, r4, -c2
	dp3 r3, r3, r4
	mul r3, r3, c3

################################################################################

%footer

# Shader footer
	# Emit the pixel
	mov oColor, r0
	# Return
	ret

# End of shader code
