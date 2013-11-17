/* exynos_drm.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *	Joonyoung Shim <jy0922.shim@samsung.com>
 *	Seung-Woo Kim <sw0312.kim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _EXYNOS_DRM_H_
#define _EXYNOS_DRM_H_

#include <drm/drm.h>

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_exynos_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
};

/**
 * A structure for getting buffer offset.
 *
 * @handle: a pointer to gem object created.
 * @pad: just padding to be 64-bit aligned.
 * @offset: relatived offset value of the memory region allocated.
 *	- this value should be set by user.
 */
struct drm_exynos_gem_map_off {
	unsigned int handle;
	unsigned int pad;
	uint64_t offset;
};

/**
 * A structure for mapping buffer.
 *
 * @handle: a handle to gem object created.
 * @pad: just padding to be 64-bit aligned.
 * @size: memory size to be mapped.
 * @mapped: having user virtual address mmaped.
 *	- this variable would be filled by exynos gem module
 *	of kernel side with user virtual address which is allocated
 *	by do_mmap().
 */
struct drm_exynos_gem_mmap {
	unsigned int handle;
	unsigned int pad;
	uint64_t size;
	uint64_t mapped;
};

/**
 * A structure to gem information.
 *
 * @handle: a handle to gem object created.
 * @flags: flag value including memory type and cache attribute and
 *	this value would be set by driver.
 * @size: size to memory region allocated by gem and this size would
 *	be set by driver.
 */
struct drm_exynos_gem_info {
	unsigned int handle;
	unsigned int flags;
	uint64_t size;
};

/**
 * A structure for user connection request of virtual display.
 *
 * @connection: indicate whether doing connetion or not by user.
 * @extensions: if this value is 1 then the vidi driver would need additional
 *	128bytes edid data.
 * @edid: the edid data pointer from user side.
 */
struct drm_exynos_vidi_connection {
	unsigned int connection;
	unsigned int extensions;
	uint64_t edid;
};

/* memory type definitions. */
enum e_drm_exynos_gem_mem_type {
	/* Physically Continuous memory and used as default. */
	EXYNOS_BO_CONTIG	= 0 << 0,
	/* Physically Non-Continuous memory. */
	EXYNOS_BO_NONCONTIG	= 1 << 0,
	/* non-cachable mapping and used as default. */
	EXYNOS_BO_NONCACHABLE	= 0 << 1,
	/* cachable mapping. */
	EXYNOS_BO_CACHABLE	= 1 << 1,
	/* write-combine mapping. */
	EXYNOS_BO_WC		= 1 << 2,
	/* No kernel mapping */
	EXYNOS_BO_UNMAPPED	= 1 << 3,
	EXYNOS_BO_MASK		= EXYNOS_BO_NONCONTIG | EXYNOS_BO_CACHABLE |
					EXYNOS_BO_WC | EXYNOS_BO_UNMAPPED
};

struct drm_exynos_g2d_get_ver {
	__u32	major;
	__u32	minor;
};

struct drm_exynos_g2d_cmd {
	__u32	offset;
	__u32	data;
};

enum drm_exynos_g2d_buf_type {
	G2D_BUF_USERPTR = 1 << 31,
};

enum drm_exynos_g2d_event_type {
	G2D_EVENT_NOT,
	G2D_EVENT_NONSTOP,
	G2D_EVENT_STOP,		/* not yet */
};

struct drm_exynos_g2d_userptr {
	unsigned long userptr;
	unsigned long size;
};

struct drm_exynos_g2d_set_cmdlist {
	__u64					cmd;
	__u64					cmd_buf;
	__u32					cmd_nr;
	__u32					cmd_buf_nr;

	/* for g2d event */
	__u64					event_type;
	__u64					user_data;
};

struct drm_exynos_g2d_exec {
	__u64					async;
};

enum g3d_register {
	/*
	 * Protected registers
	 */

	FGPF_FRONTST,
	FGPF_DEPTHT,
	FGPF_DBMSK,
	FGPF_FBCTL,
	FGPF_DBADDR,
	FGPF_CBADDR,
	FGPF_FBW,

	G3D_NUM_PROT_REGISTERS,

	/*
	 * Public registers
	 */

	/* Host interface */
	FGHI_ATTRIB0 = G3D_NUM_PROT_REGISTERS,
	FGHI_ATTRIB1,
	FGHI_ATTRIB2,
	FGHI_ATTRIB3,
	FGHI_ATTRIB4,
	FGHI_ATTRIB5,
	FGHI_ATTRIB6,
	FGHI_ATTRIB7,
	FGHI_ATTRIB8,
	FGHI_ATTRIB9,
	FGHI_ATTRIB_VBCTRL0,
	FGHI_ATTRIB_VBCTRL1,
	FGHI_ATTRIB_VBCTRL2,
	FGHI_ATTRIB_VBCTRL3,
	FGHI_ATTRIB_VBCTRL4,
	FGHI_ATTRIB_VBCTRL5,
	FGHI_ATTRIB_VBCTRL6,
	FGHI_ATTRIB_VBCTRL7,
	FGHI_ATTRIB_VBCTRL8,
	FGHI_ATTRIB_VBCTRL9,
	FGHI_ATTRIB_VBBASE0,
	FGHI_ATTRIB_VBBASE1,
	FGHI_ATTRIB_VBBASE2,
	FGHI_ATTRIB_VBBASE3,
	FGHI_ATTRIB_VBBASE4,
	FGHI_ATTRIB_VBBASE5,
	FGHI_ATTRIB_VBBASE6,
	FGHI_ATTRIB_VBBASE7,
	FGHI_ATTRIB_VBBASE8,
	FGHI_ATTRIB_VBBASE9,

	/* Primitive engine */
	FGPE_VERTEX_CONTEXT,
	FGPE_VIEWPORT_OX,
	FGPE_VIEWPORT_OY,
	FGPE_VIEWPORT_HALF_PX,
	FGPE_VIEWPORT_HALF_PY,
	FGPE_DEPTHRANGE_HALF_F_SUB_N,
	FGPE_DEPTHRANGE_HALF_F_ADD_N,

	/* Raster engine */
	FGRA_PIX_SAMP,
	FGRA_D_OFF_EN,
	FGRA_D_OFF_FACTOR,
	FGRA_D_OFF_UNITS,
	FGRA_BFCULL,
	FGRA_YCLIP,
	FGRA_LODCTL,
	FGRA_XCLIP,
	FGRA_PWIDTH,
	FGRA_PSIZE_MIN,
	FGRA_PSIZE_MAX,
	FGRA_COORDREPLACE,
	FGRA_LWIDTH,

	/* Per-fragment unit */
	FGPF_ALPHAT,
	FGPF_BACKST,
	FGPF_CCLR,
	FGPF_BLEND,
	FGPF_LOGOP,
	FGPF_CBMSK,

	G3D_NUM_REGISTERS
};

enum g3d_request_id {
	G3D_REQUEST_REGISTER_WRITE,
	G3D_REQUEST_SHADER_PROGRAM,
	G3D_REQUEST_SHADER_DATA,
	G3D_REQUEST_TEXTURE,
	G3D_REQUEST_COLORBUFFER,
	G3D_REQUEST_DEPTHBUFFER,
	G3D_REQUEST_DRAW,

	G3D_NUM_REQUESTS
};

enum g3d_shader_type {
	G3D_SHADER_VERTEX,
	G3D_SHADER_PIXEL,

	G3D_NUM_SHADERS
};

enum g3d_shader_data_type {
	G3D_SHADER_DATA_FLOAT,
	G3D_SHADER_DATA_INT,
	G3D_SHADER_DATA_BOOL,

	G3D_NUM_SHADER_DATA_TYPES
};

struct g3d_state_entry {
	__u32 reg;
	__u32 val;
};

#define G3D_NUM_MIPMAPS		11
#define G3D_TEXTURE_DIRTY	(1 << 0)
#define G3D_TEXTURE_DETACH	(1 << 1)

struct g3d_req_texture_setup {
	__u32 control;
	__u32 width;
	__u32 height;
	__u32 depth;
	__u32 offset[G3D_NUM_MIPMAPS];
	__u32 min_level;
	__u32 max_level;
	__u32 base_offset;
	__u32 handle;
	__u32 flags;
};

#define G3D_CBUFFER_DIRTY	(1 << 0)
#define G3D_CBUFFER_DETACH	(1 << 1)

struct g3d_req_colorbuffer_setup {
	__u32 fbctl;
	__u32 offset;
	__u32 width;
	__u32 handle;
	__u32 flags;
};

#define G3D_DBUFFER_DIRTY	(1 << 0)
#define G3D_DBUFFER_DETACH	(1 << 1)

struct g3d_req_depthbuffer_setup {
	__u32 offset;
	__u32 handle;
	__u32 flags;
};

struct g3d_req_draw_buffer {
	__u32 vertices;
	__u32 handle;
	__u32 offset;
	__u32 length;
};

struct g3d_req_shader_program {
	__u32 unit_nattrib;
	__u32 dcount[2];
	__u32 handle;
	__u32 offset;
	__u32 length;
};

struct g3d_req_shader_data {
	__u32 unit_type_offs;
	__u32 data[];
};

struct drm_exynos_g3d_submit {
	__u32 handle;
	__u32 offset;
	__u32 length;
	__u32 fence;
};

/* timeouts are specified in clock-monotonic absolute times (to simplify
 * restarting interrupted ioctls).  The following struct is logically the
 * same as 'struct timespec' but 32/64b ABI safe.
 */
struct drm_exynos_timespec {
	int64_t tv_sec;          /* seconds */
	int64_t tv_nsec;         /* nanoseconds */
};

struct drm_exynos_g3d_wait {
	__u32 fence;
	__u32 pad;
	struct drm_exynos_timespec timeout;
};

enum drm_exynos_ops_id {
	EXYNOS_DRM_OPS_SRC,
	EXYNOS_DRM_OPS_DST,
	EXYNOS_DRM_OPS_MAX,
};

struct drm_exynos_sz {
	__u32	hsize;
	__u32	vsize;
};

struct drm_exynos_pos {
	__u32	x;
	__u32	y;
	__u32	w;
	__u32	h;
};

enum drm_exynos_flip {
	EXYNOS_DRM_FLIP_NONE = (0 << 0),
	EXYNOS_DRM_FLIP_VERTICAL = (1 << 0),
	EXYNOS_DRM_FLIP_HORIZONTAL = (1 << 1),
	EXYNOS_DRM_FLIP_BOTH = EXYNOS_DRM_FLIP_VERTICAL |
			EXYNOS_DRM_FLIP_HORIZONTAL,
};

enum drm_exynos_degree {
	EXYNOS_DRM_DEGREE_0,
	EXYNOS_DRM_DEGREE_90,
	EXYNOS_DRM_DEGREE_180,
	EXYNOS_DRM_DEGREE_270,
};

enum drm_exynos_planer {
	EXYNOS_DRM_PLANAR_Y,
	EXYNOS_DRM_PLANAR_CB,
	EXYNOS_DRM_PLANAR_CR,
	EXYNOS_DRM_PLANAR_MAX,
};

/**
 * A structure for ipp supported property list.
 *
 * @version: version of this structure.
 * @ipp_id: id of ipp driver.
 * @count: count of ipp driver.
 * @writeback: flag of writeback supporting.
 * @flip: flag of flip supporting.
 * @degree: flag of degree information.
 * @csc: flag of csc supporting.
 * @crop: flag of crop supporting.
 * @scale: flag of scale supporting.
 * @refresh_min: min hz of refresh.
 * @refresh_max: max hz of refresh.
 * @crop_min: crop min resolution.
 * @crop_max: crop max resolution.
 * @scale_min: scale min resolution.
 * @scale_max: scale max resolution.
 */
struct drm_exynos_ipp_prop_list {
	__u32	version;
	__u32	ipp_id;
	__u32	count;
	__u32	writeback;
	__u32	flip;
	__u32	degree;
	__u32	csc;
	__u32	crop;
	__u32	scale;
	__u32	refresh_min;
	__u32	refresh_max;
	__u32	reserved;
	struct drm_exynos_sz	crop_min;
	struct drm_exynos_sz	crop_max;
	struct drm_exynos_sz	scale_min;
	struct drm_exynos_sz	scale_max;
};

/**
 * A structure for ipp config.
 *
 * @ops_id: property of operation directions.
 * @flip: property of mirror, flip.
 * @degree: property of rotation degree.
 * @fmt: property of image format.
 * @sz: property of image size.
 * @pos: property of image position(src-cropped,dst-scaler).
 */
struct drm_exynos_ipp_config {
	enum drm_exynos_ops_id ops_id;
	enum drm_exynos_flip	flip;
	enum drm_exynos_degree	degree;
	__u32	fmt;
	struct drm_exynos_sz	sz;
	struct drm_exynos_pos	pos;
};

enum drm_exynos_ipp_cmd {
	IPP_CMD_NONE,
	IPP_CMD_M2M,
	IPP_CMD_WB,
	IPP_CMD_OUTPUT,
	IPP_CMD_MAX,
};

/**
 * A structure for ipp property.
 *
 * @config: source, destination config.
 * @cmd: definition of command.
 * @ipp_id: id of ipp driver.
 * @prop_id: id of property.
 * @refresh_rate: refresh rate.
 */
struct drm_exynos_ipp_property {
	struct drm_exynos_ipp_config config[EXYNOS_DRM_OPS_MAX];
	enum drm_exynos_ipp_cmd	cmd;
	__u32	ipp_id;
	__u32	prop_id;
	__u32	refresh_rate;
};

enum drm_exynos_ipp_buf_type {
	IPP_BUF_ENQUEUE,
	IPP_BUF_DEQUEUE,
};

/**
 * A structure for ipp buffer operations.
 *
 * @ops_id: operation directions.
 * @buf_type: definition of buffer.
 * @prop_id: id of property.
 * @buf_id: id of buffer.
 * @handle: Y, Cb, Cr each planar handle.
 * @user_data: user data.
 */
struct drm_exynos_ipp_queue_buf {
	enum drm_exynos_ops_id	ops_id;
	enum drm_exynos_ipp_buf_type	buf_type;
	__u32	prop_id;
	__u32	buf_id;
	__u32	handle[EXYNOS_DRM_PLANAR_MAX];
	__u32	reserved;
	__u64	user_data;
};

enum drm_exynos_ipp_ctrl {
	IPP_CTRL_PLAY,
	IPP_CTRL_STOP,
	IPP_CTRL_PAUSE,
	IPP_CTRL_RESUME,
	IPP_CTRL_MAX,
};

/**
 * A structure for ipp start/stop operations.
 *
 * @prop_id: id of property.
 * @ctrl: definition of control.
 */
struct drm_exynos_ipp_cmd_ctrl {
	__u32	prop_id;
	enum drm_exynos_ipp_ctrl	ctrl;
};

#define DRM_EXYNOS_GEM_CREATE		0x00
#define DRM_EXYNOS_GEM_MAP_OFFSET	0x01
#define DRM_EXYNOS_GEM_MMAP		0x02
/* Reserved 0x03 ~ 0x05 for exynos specific gem ioctl */
#define DRM_EXYNOS_GEM_GET		0x04
#define DRM_EXYNOS_VIDI_CONNECTION	0x07

/* G2D */
#define DRM_EXYNOS_G2D_GET_VER		0x20
#define DRM_EXYNOS_G2D_SET_CMDLIST	0x21
#define DRM_EXYNOS_G2D_EXEC		0x22

/* IPP - Image Post Processing */
#define DRM_EXYNOS_IPP_GET_PROPERTY	0x30
#define DRM_EXYNOS_IPP_SET_PROPERTY	0x31
#define DRM_EXYNOS_IPP_QUEUE_BUF	0x32
#define DRM_EXYNOS_IPP_CMD_CTRL	0x33

#define DRM_EXYNOS_G3D_SUBMIT		0x40
#define DRM_EXYNOS_G3D_WAIT		0x41

#define DRM_IOCTL_EXYNOS_GEM_CREATE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_CREATE, struct drm_exynos_gem_create)

#define DRM_IOCTL_EXYNOS_GEM_MAP_OFFSET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_MAP_OFFSET, struct drm_exynos_gem_map_off)

#define DRM_IOCTL_EXYNOS_GEM_MMAP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_MMAP, struct drm_exynos_gem_mmap)

#define DRM_IOCTL_EXYNOS_GEM_GET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_GET,	struct drm_exynos_gem_info)

#define DRM_IOCTL_EXYNOS_VIDI_CONNECTION	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_VIDI_CONNECTION, struct drm_exynos_vidi_connection)

#define DRM_IOCTL_EXYNOS_G2D_GET_VER		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G2D_GET_VER, struct drm_exynos_g2d_get_ver)
#define DRM_IOCTL_EXYNOS_G2D_SET_CMDLIST	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G2D_SET_CMDLIST, struct drm_exynos_g2d_set_cmdlist)
#define DRM_IOCTL_EXYNOS_G2D_EXEC		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G2D_EXEC, struct drm_exynos_g2d_exec)
		
#define DRM_IOCTL_EXYNOS_IPP_GET_PROPERTY	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_GET_PROPERTY, struct drm_exynos_ipp_prop_list)
#define DRM_IOCTL_EXYNOS_IPP_SET_PROPERTY	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_SET_PROPERTY, struct drm_exynos_ipp_property)
#define DRM_IOCTL_EXYNOS_IPP_QUEUE_BUF	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_QUEUE_BUF, struct drm_exynos_ipp_queue_buf)
#define DRM_IOCTL_EXYNOS_IPP_CMD_CTRL		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_IPP_CMD_CTRL, struct drm_exynos_ipp_cmd_ctrl)

#define DRM_IOCTL_EXYNOS_G3D_SUBMIT		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G3D_SUBMIT, struct drm_exynos_g3d_submit)
#define DRM_IOCTL_EXYNOS_G3D_WAIT		DRM_IOW(DRM_COMMAND_BASE + \
		DRM_EXYNOS_G3D_WAIT, struct drm_exynos_g3d_wait)

/* EXYNOS specific events */
#define DRM_EXYNOS_G2D_EVENT		0x80000000
#define DRM_EXYNOS_IPP_EVENT		0x80000001
#define DRM_EXYNOS_G3D_EVENT		0x80000002

struct drm_exynos_g2d_event {
	struct drm_event	base;
	__u64			user_data;
	__u32			tv_sec;
	__u32			tv_usec;
	__u32			cmdlist_no;
	__u32			reserved;
};

struct drm_exynos_ipp_event {
	struct drm_event	base;
	__u64			user_data;
	__u32			tv_sec;
	__u32			tv_usec;
	__u32			prop_id;
	__u32			reserved;
	__u32			buf_id[EXYNOS_DRM_OPS_MAX];
};

#endif /* _EXYNOS_DRM_H_ */
