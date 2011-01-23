#ifndef _FRAG_H_
#define _FRAG_H_

static const unsigned int frag_cfloat[] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
	0x3f000000, 0x3f000000, 0x3f000000, 0x3f000000,
	0x40800000, 0x40800000, 0x40800000, 0x40800000,
};

static const unsigned int frag_header[] = {
	0x00000000, 0x00000000, 0x00f820e4, 0x00000000,
};

static const unsigned int frag_texture0[] = {
	0x00000000, 0x0001e407, 0x107821e4, 0x00000000,
	0x00000000, 0x05000000, 0x188001e4, 0x00000000,
	0x00000000, 0x01010000, 0x00f8211b, 0x00000000,
	0x00000000, 0x02040000, 0x00f822e4, 0x00000000,
	0x00000000, 0x02050000, 0x00f823e4, 0x00000000,
};

static const unsigned int frag_texture1[] = {
	0x01000000, 0x0002e407, 0x107821e4, 0x00000000,
	0x00000000, 0x05010000, 0x188001e4, 0x00000000,
	0x00000000, 0x01010000, 0x00f8211b, 0x00000000,
	0x00000000, 0x02060000, 0x00f822e4, 0x00000000,
	0x00000000, 0x02070000, 0x00f823e4, 0x00000000,
};

static const unsigned int frag_replace[] = {
	0x00000000, 0x01010000, 0x00f820e4, 0x00000000,
};

static const unsigned int frag_modulate[] = {
	0x01000000, 0x0100e401, 0x037820e4, 0x00000000,
};

static const unsigned int frag_decal[] = {
	0x01000000, 0x0201e441, 0x024023e4, 0x00000000,
	0x03000000, 0x0100ff01, 0x233820e4, 0x00000000,
	0x01e40100, 0x0101ff01, 0x0eb820e4, 0x00000000,
};

static const unsigned int frag_blend[] = {
	0x00000000, 0x01010000, 0x00c024e4, 0x00000000,
	0x01000000, 0x0201e441, 0x023824e4, 0x00000000,
	0x04000000, 0x0100e401, 0x237820e4, 0x00000000,
	0x01e40100, 0x0204e401, 0x0eb820e4, 0x00000000,
};

static const unsigned int frag_add[] = {
	0x01000000, 0x0100e401, 0x034020e4, 0x00000000,
	0x01000000, 0x0100e401, 0x023820e4, 0x00000000,
};

static const unsigned int frag_combine[] = {
};

static const unsigned int frag_combine_col[] = {
	0x03000000, 0x0104e401, 0x033a20e4, 0x00000000,
};

static const unsigned int frag_combine_a[] = {
	0x03000000, 0x0104e401, 0x034220e4, 0x00000000,
};

static const unsigned int frag_combine_uni[] = {
	0x03000000, 0x0104e401, 0x037a20e4, 0x00000000,
};

static const unsigned int frag_combine_arg0tex[] = {
	0x00000000, 0x01010000, 0x00f824e4, 0x00000000,
};

static const unsigned int frag_combine_arg0const[] = {
	0x00000000, 0x01020000, 0x00f824e4, 0x00000000,
};

static const unsigned int frag_combine_arg0col[] = {
	0x00000000, 0x00000000, 0x00f824e4, 0x00000000,
};

static const unsigned int frag_combine_arg0prev[] = {
	0x00000000, 0x01000000, 0x00f824e4, 0x00000000,
};

static const unsigned int frag_combine_arg1tex[] = {
	0x00000000, 0x01010000, 0x00f825e4, 0x00000000,
};

static const unsigned int frag_combine_arg1const[] = {
	0x00000000, 0x01020000, 0x00f825e4, 0x00000000,
};

static const unsigned int frag_combine_arg1col[] = {
	0x00000000, 0x00000000, 0x00f825e4, 0x00000000,
};

static const unsigned int frag_combine_arg1prev[] = {
	0x00000000, 0x01000000, 0x00f825e4, 0x00000000,
};

static const unsigned int frag_combine_arg2tex[] = {
	0x00000000, 0x01010000, 0x00f826e4, 0x00000000,
};

static const unsigned int frag_combine_arg2const[] = {
	0x00000000, 0x01020000, 0x00f826e4, 0x00000000,
};

static const unsigned int frag_combine_arg2col[] = {
	0x00000000, 0x00000000, 0x00f826e4, 0x00000000,
};

static const unsigned int frag_combine_arg2prev[] = {
	0x00000000, 0x01000000, 0x00f826e4, 0x00000000,
};

static const unsigned int frag_combine_arg0sc[] = {
};

static const unsigned int frag_combine_arg0omsc[] = {
	0x04000000, 0x0201e441, 0x027824e4, 0x00000000,
};

static const unsigned int frag_combine_arg0sa[] = {
	0x00000000, 0x01040000, 0x00f824ff, 0x00000000,
};

static const unsigned int frag_combine_arg0omsa[] = {
	0x04000000, 0x0201ff41, 0x027824e4, 0x00000000,
};

static const unsigned int frag_combine_arg1sc[] = {
};

static const unsigned int frag_combine_arg1omsc[] = {
	0x05000000, 0x0201e441, 0x027825e4, 0x00000000,
};

static const unsigned int frag_combine_arg1sa[] = {
	0x00000000, 0x01050000, 0x00f825ff, 0x00000000,
};

static const unsigned int frag_combine_arg1omsa[] = {
	0x05000000, 0x0201ff41, 0x027825e4, 0x00000000,
};

static const unsigned int frag_combine_arg2sc[] = {
};

static const unsigned int frag_combine_arg2omsc[] = {
	0x06000000, 0x0201e441, 0x027826e4, 0x00000000,
};

static const unsigned int frag_combine_arg2sa[] = {
	0x00000000, 0x01060000, 0x00f826ff, 0x00000000,
};

static const unsigned int frag_combine_arg2omsa[] = {
	0x06000000, 0x0201ff41, 0x027826e4, 0x00000000,
};

static const unsigned int frag_combine_replace[] = {
};

static const unsigned int frag_combine_modulate[] = {
	0x05000000, 0x0104e401, 0x037824e4, 0x00000000,
};

static const unsigned int frag_combine_add[] = {
	0x05000000, 0x0104e401, 0x027824e4, 0x00000000,
};

static const unsigned int frag_combine_adds[] = {
	0x05000000, 0x0104e401, 0x027824e4, 0x00000000,
	0x02000000, 0x0104e442, 0x027824e4, 0x00000000,
};

static const unsigned int frag_combine_interpolate[] = {
	0x06000000, 0x0104e401, 0x037824e4, 0x00000000,
	0x06000000, 0x0201e441, 0x227826e4, 0x00000000,
	0x06e40104, 0x0105e401, 0x0ef824e4, 0x00000000,
};

static const unsigned int frag_combine_subtract[] = {
	0x05000000, 0x0104e441, 0x027824e4, 0x00000000,
};

static const unsigned int frag_combine_dot3[] = {
	0x02000000, 0x0104e442, 0x027824e4, 0x00000000,
	0x02000000, 0x0105e442, 0x027825e4, 0x00000000,
	0x05000000, 0x0104e401, 0x047824e4, 0x00000000,
	0x03000000, 0x0104e402, 0x037824e4, 0x00000000,
};

static const unsigned int frag_footer[] = {
	0x00000000, 0x01000000, 0x00f810e4, 0x00000000,
	0x00000000, 0x00000000, 0x1e000000, 0x00000000,
};

static const unsigned int frag_clear[] = {
	0x00000000, 0x02ff0000, 0x00fa10e4, 0x00000000,
	0x00000000, 0x00000000, 0x1e000000, 0x00000000,
};

#endif
