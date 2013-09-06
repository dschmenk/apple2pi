#define FUSE_USE_VERSION 26
#include "a2lib.c"
#include <fuse.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

/*
 * ProDOS Commands.
 */
#define	PRODOS_CREATE		0xC0
#define	PRODOS_DESTROY		0xC1
#define	PRODOS_RENAME		0xC2
#define	PRODOS_SET_FILE_INFO	0xC3
#define	PRODOS_GET_FILE_INFO	0xC4
#define	PRODOS_ON_LINE		0xC5
#define	PRODOS_SET_PREFIX	0xC6
#define	PRODOS_GET_PREFIX	0xC7
#define	PRODOS_OPEN		0xC8
#define	PRODOS_NEWLINE		0xC9
#define	PRODOS_READ		0xCA
#define	PRODOS_WRITE		0xCB
#define	PRODOS_CLOSE		0xCC
#define	PRODOS_FLUSH		0xCD
#define	PRODOS_SET_MARK		0xCE
#define	PRODOS_MARK		0xCF
#define	PRODOS_SET_EOF		0xD0
#define	PRODOS_GET_EOF		0xD1
#define	PRODOS_SET_BUF		0xD2
#define	PRODOS_GET_BUF		0xD3
#define	PRODOS_READ_BLOCK	0x80
#define	PRODOS_WRITE_BLOCK	0x81
/*
 * ProDOS Errors.
 */
#define	PRODOS_OK		0x00
#define	PRODOS_ERR_BAD_CMD	0x01
#define	PRODOS_ERR_BAD_COUNT	0x04
#define	PRODOS_ERR_INT_TBL_FULL	0x25
#define	PRODOS_ERR_IO		0x27
#define	PRODOS_ERR_NO_DEV	0x28
#define	PRODOS_ERR_WR_PROT	0x2B
#define	PRODOS_ERR_DSK_SWITCH	0x2E
#define	PRODOS_ERR_INVLD_PATH	0x40
#define	PRODOS_ERR_FCB_FULL	0x42
#define	PRODOS_ERR_INVLD_REFNUM	0x43
#define	PRODOS_ERR_PATH_NOT_FND	0x44
#define	PRODOS_ERR_VOL_NOT_FND	0x45
#define	PRODOS_ERR_FILE_NOT_FND	0x46
#define	PRODOS_ERR_DUP_FILENAME	0x47
#define	PRODOS_ERR_OVERRUN	0x48
#define	PRODOS_ERR_VOL_DIR_FULL	0x49
#define	PRODOS_ERR_INCOMPAT_FMT	0x4A
#define	PRODOS_ERR_UNSUPP_TYPE	0x4B
#define	PRODOS_ERR_EOF		0x4C
#define	PRODOS_ERR_POS_RANGE	0x4D
#define	PRODOS_ERR_ACCESS	0x4E
#define	PRODOS_ERR_FILE_OPEN	0x50
#define	PRODOS_ERR_DIR_COUNT	0x51
#define	PRODOS_ERR_NOT_PRODOS	0x52
#define	PRODOS_ERR_INVLD_PARAM	0x53
#define	PRODOS_ERR_VCB_FULL	0x55
#define	PRODOS_ERR_BAD_BUF_ADDR	0x56
#define	PRODOS_ERR_DUP_VOL	0x57
#define	PRODOS_ERR_BAD_BITMAP	0x5A
#define PRODOS_ERR_UNKNOWN	0x100
/*
 * ProDOS call template.
 */
#define PRODOS_CALL    		0x0300
#define PRODOS_CALL_LEN    	0x08
#define PRODOS_CMD	     	0x03
#define	PRODOS_PARAMS		0x07
#define	PRODOS_PARAM_CNT	0x07
#define	PRODOS_PARAM_BUFFER	(PRODOS_CALL+PRODOS_PARAMS)
#define PRODOS_DATA_BUFFER	0x4000
#define PRODOS_DATA_BUFFER_LEN	0x2000
#define PRODOS_IO_BUFFER	0x6000
#define	PRODOS_IO_BUFFER_LEN	0x0400
#define PRODOS_IO_BUFFER_NUM	8
static unsigned char prodos[32] = {
// ORG @ $300
    0x20, 0x00, 0xBF, // JSR $BF00 (PRODOS)
    0x00,             // DB CMD
    0x07, 0x03,       // DW PARAMS
    0x60,             // RTS
// PARAMS @ $307
    0x00             // PARAM_COUNT
};
/*
 * Apple II Pi connection.
 */
static int pifd = 0;
/*
 * Cached directory blocks.
 */
#define CACHE_BLOCKS_MAX	16
static char cachepath[128] = "";
static unsigned char cachedata[512][CACHE_BLOCKS_MAX]; /* !!! Is this enough !!! */
/*
 * Filename & date/time conversion routines.
 */
static char *unix_name(unsigned char *pname, int type, int aux)
{
    int l;
    static char filename[24];
    char extname[8];
    l = pname[0] & 0x0F;
    strncpy(filename, pname + 1, l);
//    if (type != 0x0F) /* Directory type */
//    {
//	sprintf(extname, "#%02X%04X", type, aux);
//	strcpy(filename + l, extname);
//    }
//    else
	filename[l] = '\0';
    return filename;
}

static unsigned char *prodos_name(char *uname, int *type, int *aux)
{
    int l;
    static unsigned char filename[25];
    if ((l = strlen(uname)) > 15)
	l = 15;
    strncpy(filename + 1, uname, l);
    filename[0] = l;
    filename[l + 1] = '\0';
    return filename;
}
/*
 * ProDOS calls to Apple II Pi.
 */
static int io_buff_mask = 0;
static int prodos_alloc_io_buff(void)
{
    int i;
    for (i = 0; i < PRODOS_IO_BUFFER_NUM; i++)
	if ((io_buff_mask & (1 << i)) == 0)
	{
	    io_buff_mask |= (1 << i);
	    return (PRODOS_IO_BUFFER + PRODOS_IO_BUFFER_LEN * i);
	}
    return 0;
}
static int prodos_free_io_buff(int buf)
{
    if (buf < PRODOS_IO_BUFFER || buf > (PRODOS_IO_BUFFER + PRODOS_IO_BUFFER_LEN * PRODOS_IO_BUFFER_NUM))
	return -1;
    int i = (buf - PRODOS_IO_BUFFER) / PRODOS_IO_BUFFER_LEN;
    io_buff_mask &= ~(1 << i);
    return i;
}
static int prodos_open(const char *path, int *io_buff)
{
    char prodos_path[65], refnum = 0;
    int result;
    
    prodos_path[0] = strlen(path);
    if (path[0] > 64)
	return -PRODOS_ERR_INVLD_PATH;
    strcpy(prodos_path+1, path);
    a2write(pifd, PRODOS_DATA_BUFFER, prodos_path[0] + 1,  prodos_path);
    if (*io_buff == 0)
    {
	if ((*io_buff = prodos_alloc_io_buff()) == 0)
	    return -PRODOS_ERR_FCB_FULL;
    }
    prodos[PRODOS_CMD]        = PRODOS_OPEN;
    prodos[PRODOS_PARAM_CNT]  = 3;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 3] = (unsigned char) *io_buff;
    prodos[PRODOS_PARAMS + 4] = (unsigned char) (*io_buff >> 8);
    prodos[PRODOS_PARAMS + 5] = 0;
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 5, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    {
	if (result == 0)
	{
	    a2read(pifd, PRODOS_PARAM_BUFFER + 5, 1, &refnum);
	    return refnum;
	}
    	return -result;
    }
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_close(int refnum, int *io_buff)
{
    int result;
    
    if (io_buff && *io_buff != 0)
    {
	prodos_free_io_buff(*io_buff);
	*io_buff = 0;
    }
    prodos[PRODOS_CMD]        = PRODOS_CLOSE;
    prodos[PRODOS_PARAM_CNT]  = 1;
    prodos[PRODOS_PARAMS + 1] = refnum;
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 1, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    	return -result;
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_read(int refnum, char *data_buff, int req_xfer)
{
    int result, short_req, short_xfer, total_xfer = 0;

    prodos[PRODOS_CMD]        = PRODOS_READ;
    prodos[PRODOS_PARAM_CNT]  = 4;
    prodos[PRODOS_PARAMS + 1] = refnum;
    while (req_xfer)
    {
	short_req = (req_xfer > PRODOS_DATA_BUFFER_LEN) ? PRODOS_DATA_BUFFER_LEN : req_xfer;
	prodos[PRODOS_PARAMS + 2] = (unsigned char) PRODOS_DATA_BUFFER;
	prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
	prodos[PRODOS_PARAMS + 4] = (unsigned char) short_req;
	prodos[PRODOS_PARAMS + 5] = (unsigned char) (short_req >> 8);
	prodos[PRODOS_PARAMS + 6] = 0;
	prodos[PRODOS_PARAMS + 7] = 0;
	a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 7, prodos);
	if (a2call(pifd, PRODOS_CALL, &result))
	{
	    if (result == 0)
	    {
		a2read(pifd, PRODOS_PARAM_BUFFER + 6, 2, prodos + PRODOS_PARAMS + 6);
		if ((short_xfer = (unsigned char) prodos[PRODOS_PARAMS + 6] + (unsigned char)prodos[PRODOS_PARAMS + 7] * 256) > 0)
		    a2read(pifd, PRODOS_DATA_BUFFER, short_xfer, data_buff + total_xfer);
		req_xfer   -= short_xfer;
		total_xfer += short_xfer;
	    }
	    else
		return (result == PRODOS_ERR_EOF || total_xfer) ? total_xfer : -result;
	}
	else
	    return -PRODOS_ERR_UNKNOWN;
    }
    return total_xfer;
}
static int prodos_write(int refnum, const char *data_buff, int req_xfer)
{
    int result, short_req, short_xfer, total_xfer = 0;

    prodos[PRODOS_CMD]        = PRODOS_WRITE;
    prodos[PRODOS_PARAM_CNT]  = 4;
    prodos[PRODOS_PARAMS + 1] = refnum;
    while (req_xfer)
    {
	short_req = (req_xfer > PRODOS_DATA_BUFFER_LEN) ? PRODOS_DATA_BUFFER_LEN : req_xfer;
	a2write(pifd, PRODOS_DATA_BUFFER, short_req, (char *)(data_buff + total_xfer));
	prodos[PRODOS_PARAMS + 2] = (unsigned char) PRODOS_DATA_BUFFER;
	prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
	prodos[PRODOS_PARAMS + 4] = (unsigned char) short_req;
	prodos[PRODOS_PARAMS + 5] = (unsigned char) (short_req >> 8);
	prodos[PRODOS_PARAMS + 6] = 0;
	prodos[PRODOS_PARAMS + 7] = 0;
	a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 7, prodos);
	if (a2call(pifd, PRODOS_CALL, &result))
	{
	    if (result == 0)
	    {
		a2read(pifd, PRODOS_PARAM_BUFFER + 6, 2, prodos + PRODOS_PARAMS + 6);
		short_xfer = (unsigned char) prodos[PRODOS_PARAMS + 6] + (unsigned char)prodos[PRODOS_PARAMS + 7] * 256;
		req_xfer -= short_xfer;
		total_xfer += short_xfer;
	    }
	    else
		return -result;
	}
	else
	    return -PRODOS_ERR_UNKNOWN;
    }
    return total_xfer;
}
static int prodos_set_mark(int refnum, int position)
{
    int result;
    
    prodos[PRODOS_CMD]        = PRODOS_SET_MARK;
    prodos[PRODOS_PARAM_CNT]  = 2;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) refnum;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) position;
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (position >> 8);
    prodos[PRODOS_PARAMS + 4] = (unsigned char) (position >> 16);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 4, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    	return -result;
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_set_eof(int refnum, int position)
{
    int result;
    
    prodos[PRODOS_CMD]        = PRODOS_SET_EOF;
    prodos[PRODOS_PARAM_CNT]  = 2;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) refnum;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) position;
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (position >> 8);
    prodos[PRODOS_PARAMS + 4] = (unsigned char) (position >> 16);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 4, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    	return -result;
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_get_eof(int refnum)
{
    int position, result;
    
    prodos[PRODOS_CMD]        = PRODOS_GET_EOF;
    prodos[PRODOS_PARAM_CNT]  = 2;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) refnum;
    prodos[PRODOS_PARAMS + 2] = 0;
    prodos[PRODOS_PARAMS + 3] = 0;
    prodos[PRODOS_PARAMS + 4] = 0;
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 4, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    {
	if (result == 0)
	{
	    a2read(pifd, PRODOS_PARAM_BUFFER + 2, 3, prodos + PRODOS_PARAMS + 2);
	    position = prodos[PRODOS_PARAMS + 2]
		    + (prodos[PRODOS_PARAMS + 3] << 8)
		    + (prodos[PRODOS_PARAMS + 4] << 16);
	    return position;
	}
    	return -result;
    }
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_on_line(char *data_buff)
{
    int result;
    
    prodos[PRODOS_CMD]        = PRODOS_ON_LINE;
    prodos[PRODOS_PARAM_CNT]  = 2;
    prodos[PRODOS_PARAMS + 1] = 0;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 3, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    {
	if (result == 0)
	    a2read(pifd, PRODOS_DATA_BUFFER, 256, data_buff);
    	return -result;
    }
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_set_file_info(const char *path, int access, int type, int aux, int mod, int create)
{
    char prodos_path[65];
    int result;
    
    prodos_path[0] = strlen(path);
    if (prodos_path[0] > 64)
	return -PRODOS_ERR_INVLD_PATH;
    strcpy(prodos_path+1, path);
    a2write(pifd, PRODOS_DATA_BUFFER, prodos_path[0] + 1,  prodos_path);
    prodos[PRODOS_CMD]         = PRODOS_SET_FILE_INFO;
    prodos[PRODOS_PARAM_CNT]   = 7;
    prodos[PRODOS_PARAMS + 1]  = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2]  = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 3]  = access;
    prodos[PRODOS_PARAMS + 4]  = type;
    prodos[PRODOS_PARAMS + 5]  = (unsigned char) aux;
    prodos[PRODOS_PARAMS + 6]  = (unsigned char) (aux >> 8);
    prodos[PRODOS_PARAMS + 7]  = 0;
    prodos[PRODOS_PARAMS + 8]  = 0;
    prodos[PRODOS_PARAMS + 9]  = 0;
    prodos[PRODOS_PARAMS + 10] = (unsigned char) mod;
    prodos[PRODOS_PARAMS + 11] = (unsigned char) (mod >> 8);
    prodos[PRODOS_PARAMS + 12] = (unsigned char) create;
    prodos[PRODOS_PARAMS + 13] = (unsigned char) (create >> 8);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 13, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    	return -result;
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_get_file_info(const char *path, int *access, int *type, int *aux, int *storage, int *numblks, int *mod, int *create)
{
    char prodos_path[65];
    int result;
    
    prodos_path[0] = strlen(path);
    if (prodos_path[0] > 64)
	return -PRODOS_ERR_INVLD_PATH;
    strcpy(prodos_path+1, path);
    a2write(pifd, PRODOS_DATA_BUFFER, prodos_path[0] + 1,  prodos_path);
    prodos[PRODOS_CMD]         = PRODOS_GET_FILE_INFO;
    prodos[PRODOS_PARAM_CNT]   = 10;
    prodos[PRODOS_PARAMS + 1]  = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2]  = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 3]  = 0;
    prodos[PRODOS_PARAMS + 4]  = 0;
    prodos[PRODOS_PARAMS + 5]  = 0;
    prodos[PRODOS_PARAMS + 6]  = 0;
    prodos[PRODOS_PARAMS + 7]  = 0;
    prodos[PRODOS_PARAMS + 8]  = 0;
    prodos[PRODOS_PARAMS + 9]  = 0;
    prodos[PRODOS_PARAMS + 10] = 0;
    prodos[PRODOS_PARAMS + 11] = 0;
    prodos[PRODOS_PARAMS + 12] = 0;
    prodos[PRODOS_PARAMS + 13] = 0;
    prodos[PRODOS_PARAMS + 14] = 0;
    prodos[PRODOS_PARAMS + 15] = 0;
    prodos[PRODOS_PARAMS + 16] = 0;
    prodos[PRODOS_PARAMS + 17] = 0;
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 17, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    {
	if (result == 0)
	{
	    a2read(pifd, PRODOS_PARAM_BUFFER + 3, 15, prodos + PRODOS_PARAMS + 3);
	    *access  = prodos[PRODOS_PARAMS + 3];
	    *type    = prodos[PRODOS_PARAMS + 4];
	    *aux     = prodos[PRODOS_PARAMS + 5]
		    + (prodos[PRODOS_PARAMS + 6] << 8);
	    *storage = prodos[PRODOS_PARAMS + 7];
	    *numblks = prodos[PRODOS_PARAMS + 8]
		    + (prodos[PRODOS_PARAMS + 9] << 8);
	    *mod     = prodos[PRODOS_PARAMS + 10]
		    + (prodos[PRODOS_PARAMS + 11] << 8)
		    + (prodos[PRODOS_PARAMS + 12] << 16)
	   	    + (prodos[PRODOS_PARAMS + 13] << 24);
	    *create  = prodos[PRODOS_PARAMS + 14]
		    + (prodos[PRODOS_PARAMS + 15] << 8)
		    + (prodos[PRODOS_PARAMS + 16] << 16)
		    + (prodos[PRODOS_PARAMS + 17] << 24);
	}
    	return -result;
    }
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_rename(const char *filename, const char *new_filename)
{
    char prodos_path[65];
    int result;
    
    prodos_path[0] = strlen(filename);
    if (prodos_path[0] > 64)
	return -PRODOS_ERR_INVLD_PATH;
    strcpy(prodos_path+1, filename);
    a2write(pifd, PRODOS_DATA_BUFFER, prodos_path[0] + 1,  prodos_path);
    prodos_path[0] = strlen(new_filename);
    if (prodos_path[0] > 64)
	return -PRODOS_ERR_INVLD_PATH;
    strcpy(prodos_path+1, new_filename);
    a2write(pifd, PRODOS_DATA_BUFFER+256, prodos_path[0] + 1,  prodos_path);
    prodos[PRODOS_CMD]        = PRODOS_RENAME;
    prodos[PRODOS_PARAM_CNT]  = 2;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER + 256);
    prodos[PRODOS_PARAMS + 4] = (unsigned char) (PRODOS_DATA_BUFFER + 256) >> 8;
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 4, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    	return -result;
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_destroy(const char *path)
{
    char prodos_path[65];
    int result;

    prodos_path[0] = strlen(path);
    if (prodos_path[0] > 64)
	return -PRODOS_ERR_INVLD_PATH;
    strcpy(prodos_path+1, path);
    a2write(pifd, PRODOS_DATA_BUFFER, prodos_path[0] + 1,  prodos_path);
    prodos[PRODOS_CMD]        = PRODOS_DESTROY;
    prodos[PRODOS_PARAM_CNT]  = 1;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 2, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    	return -result;
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_create(const char *path, char access, char type, int aux, int create)
{
    char prodos_path[65];
    int result;

    prodos_path[0] = strlen(path);
    if (prodos_path[0] > 64)
	return -PRODOS_ERR_INVLD_PATH;
    strcpy(prodos_path+1, path);
    a2write(pifd, PRODOS_DATA_BUFFER, prodos_path[0] + 1,  prodos_path);
    prodos[PRODOS_CMD]         = PRODOS_CREATE;
    prodos[PRODOS_PARAM_CNT]   = 7;
    prodos[PRODOS_PARAMS + 1]  = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2]  = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 3]  = (unsigned char) access;
    prodos[PRODOS_PARAMS + 4]  = (unsigned char) type;
    prodos[PRODOS_PARAMS + 5]  = (unsigned char) aux;
    prodos[PRODOS_PARAMS + 6]  = (unsigned char) (aux >> 8);
    prodos[PRODOS_PARAMS + 7]  = type == 0x0F ? 0x0D : 0x01; // directory if type == 0x0F
    prodos[PRODOS_PARAMS + 8]  = (unsigned char) create;
    prodos[PRODOS_PARAMS + 9]  = (unsigned char) (create >> 8);
    prodos[PRODOS_PARAMS + 10] = (unsigned char) (create >> 16);
    prodos[PRODOS_PARAMS + 11] = (unsigned char) (create >> 24);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 11, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    	return -result;
    return -PRODOS_ERR_UNKNOWN;
}
static int prodos_map_errno(int perr)
{
    int uerr = 0;
    if (perr)
    {
	switch (perr) /* Map ProDOS error to unix errno */
	{
	    case -PRODOS_ERR_INVLD_PATH:
	    case -PRODOS_ERR_PATH_NOT_FND:
	    case -PRODOS_ERR_VOL_NOT_FND:
	    case -PRODOS_ERR_FILE_NOT_FND:
	    case -PRODOS_ERR_DUP_FILENAME:
	    case -PRODOS_ERR_UNSUPP_TYPE:
	    case -PRODOS_ERR_EOF:
		uerr = -ENOENT;
		break;
	    case -PRODOS_ERR_WR_PROT:
	    case -PRODOS_ERR_DSK_SWITCH:
	    case -PRODOS_ERR_ACCESS:
	    case -PRODOS_ERR_FILE_OPEN:
	    case -PRODOS_ERR_DUP_VOL:
		uerr = -EACCES;
		break;
	    case -PRODOS_ERR_BAD_CMD:
	    case -PRODOS_ERR_BAD_COUNT:
	    case -PRODOS_ERR_INT_TBL_FULL:
	    case -PRODOS_ERR_IO:
	    case -PRODOS_ERR_INVLD_REFNUM:
	    case -PRODOS_ERR_NO_DEV:
	    case -PRODOS_ERR_INCOMPAT_FMT:
	    case -PRODOS_ERR_OVERRUN:
	    case -PRODOS_ERR_DIR_COUNT:
	    case -PRODOS_ERR_NOT_PRODOS:
	    case -PRODOS_ERR_INVLD_PARAM:
	    case -PRODOS_ERR_BAD_BITMAP:
	    case -PRODOS_ERR_BAD_BUF_ADDR:
	    case -PRODOS_ERR_UNKNOWN:

	    case -PRODOS_ERR_FCB_FULL:
	    case -PRODOS_ERR_VOL_DIR_FULL:
	    case -PRODOS_ERR_VCB_FULL:
	    case -PRODOS_ERR_POS_RANGE:
		uerr = -1;
	}
    }
    return uerr;
}
/*
 * FUSE functions.
 */
struct stat *fillstat(struct stat *stbuf, int storage, int access, int blocks, int size, int mod, int create)
{
    memset(stbuf, 0, sizeof(struct stat));
    if (storage == 0x0F || storage == 0x0D)
    {
	stbuf->st_mode = (access & 0xC3 == 0xC3) ? S_IFDIR | 0777 : S_IFDIR | 0444;
	stbuf->st_nlink = 2;
    }
    else
    {
	stbuf->st_mode   = (access & 0xC3 == 0xC3) ? S_IFREG | 0666 : S_IFREG | 0444;
	stbuf->st_nlink  = 1;
	stbuf->st_blocks = blocks;
	stbuf->st_size   = size;
    }
    return stbuf;
}
static int cache_get_file_info(const char *path, int *access, int *type, int *aux, int *storage, int *numblks, int *size, int *mod, int *create)
{
    char dirpath[128], filename[32];
    unsigned char *entry;
    int refnum, iblk, entrylen, entriesblk, filecnt, io_buff = 0;
    int i, dl, l = strlen(path);
    
    for (dl = l - 1; dl; dl--)
	if (path[dl] == '/')
	    break;
    strncpy(dirpath, path, dl);
    dirpath[dl] = '\0';
    //printf("Match path %s to cached dir %s\n", dirpath, cachepath);
    if (strcmp(dirpath, cachepath) == 0)
    {
	strcpy(filename, path + dl + 1);
	l = l - dl - 1;
	//printf("Match filename %s len %d\n", filename, l);
	iblk = 0;
	entrylen   = cachedata[0][0x23];
	entriesblk = cachedata[0][0x24];
	filecnt    = cachedata[0][0x25] + cachedata[0][0x26] * 256;
	entry      = &cachedata[0][4] + entrylen;
	do
	{
	    for (i = (iblk == 0) ? 1 : 0; i < entriesblk && filecnt; i++)
	    {
		if (entry[0])
		{
		    entry[(entry[0] & 0x0F) + 1] = 0;
		    //printf("Searching directory entry: %s len %d\n", entry + 1, entry[0] & 0x0F);
		    if ((entry[0] & 0x0F) == l)
		    {
			//printf("Compare %s with %s\n", entry + 1, filename);
			if (strncmp(entry + 1, filename, l) == 0)
			{
			    *storage = entry[0x00] >> 4;
			    *type    = entry[0x10];
			    *access  = entry[0x1E];
			    *aux     = entry[0x1F] + entry[0x20] * 256;
			    *numblks = entry[0x13] + entry[0x14] * 256;
			    *size    = entry[0x15] + entry[0x16] * 256 + entry[0x17] * 65536;
			    *mod     = entry[0x21] | (entry[0x22] << 8)
			        | (entry[0x23] << 16) | (entry[0x24] << 24);
			    *create  = entry[0x18] | (entry[0x19] << 8)
			        | (entry[0x1A] << 16) | (entry[0x1B] << 24);
			    return 0;
			}
		    }
		    entry += entrylen;
		    filecnt--;
		}
	    }
	    if (++iblk > CACHE_BLOCKS_MAX)
		break;
	    entry = &cachedata[iblk][4];
	} while (filecnt != 0);
    }
    if (dl == 0)
    {
	*storage = 0x0F;
	*type    = 0x0F;
	*access  = 0xC3;
	*aux     = 0;
	*numblks = 0;
	*size    = 0;
	*mod     = 0;
	*create  = 0;
    }
    else if (prodos_get_file_info(path, access, type, aux, storage, numblks, mod, create) == 0)
    {
	//printf("prodos: %s access = $%02X, type = $%02X, aux = $%04X, storage = $%02X\n", path, *access, *type, *aux, *storage);
	if (*storage == 0x0F || *storage == 0x0D)
	    *size = 0;
	else
	{
	    if ((refnum = prodos_open(path, &io_buff) > 0))
	    {
		*size  = prodos_get_eof(refnum);
		prodos_close(refnum, &io_buff);
	    }
	    else
		return prodos_map_errno(refnum);
	}
    }
    else
	return -ENOENT;
    return 0;
}

static int a2pi_getattr(const char *path, struct stat *stbuf)
{
    int access, type, aux, storage, size, numblks, mod, create, refnum, io_buff = 0;
    if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
    {
	/*
	 * Root directory of volumes.
	 */
	fillstat(stbuf, 0x0F, 0xE3, 0, 0, 0, 0);
    }
    else
    {
	/*
	 * Get file info.
	 */
	if (cache_get_file_info(path, &access, &type, &aux, &storage, &numblks, &size, &mod, &create) == 0)
	{
	    if (storage == 0x0F || storage == 0x0D)
		size = 0;
	    fillstat(stbuf, storage, access, numblks, size, mod, create);
	}
	else
	    return -ENOENT;
    }
    return 0;
}

static int a2pi_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    unsigned char data_buff[512], filename[16], *entry;
    int storage, access, type, numblks, size, aux, mod, create;
    int i, l, iscached, refnum, iblk, entrylen, entriesblk, filecnt, io_buff = 0;
    struct stat stentry;
    (void) offset;
    (void) fi;
    if (strcmp(path, "/") == 0)
    {
	/*
	 * Root directory, fill with volume names.
	 */
	fillstat(&stentry, 0x0F, 0xC3, 0, 0, 0, 0);
	filler(buf, ".", &stentry, 0);
	filler(buf, "..", &stentry, 0);
	if (prodos_on_line(data_buff) == 0)
	{
	    for (i = 0; i < 256; i += 16)
		if ((l = data_buff[i] & 0x0F))
		{
		    strncpy(filename, data_buff + i + 1, l);
		    filename[l] = '\0';
		    filler(buf, filename, &stentry, 0);
		}
	}
	else
	    return -ENOENT;
    }
    else
    {
	/*
	 * Read ProDOS directory.
	 */
	iscached = (strcmp(path, cachepath) == 0);
	if (iscached || (refnum = prodos_open(path, &io_buff)) > 0)
	{
	    fillstat(&stentry, 0x0F, 0xC3, 0, 0, 0, 0);
	    filler(buf, ".", &stentry, 0);
	    filler(buf, "..", &stentry, 0);
	    iblk = 0;
	    do
	    {
		if (iscached || prodos_read(refnum, cachedata[iblk], 512) == 512)
		{
		    entry = &cachedata[iblk][4];
		    if (iblk == 0)
		    {
			entrylen   = cachedata[0][0x23];
			entriesblk = cachedata[0][0x24];
			filecnt    = cachedata[0][0x25] + cachedata[0][0x26] * 256;
			entry      = entry + entrylen;
		    }
		    for (i = (iblk == 0) ? 1 : 0; i < entriesblk && filecnt; i++)
		    {
			if ((l = entry[0x00] & 0x0F) != 0)
			{
			    storage     = entry[0x00] >> 4;
			    type        = entry[0x10];
			    access      = entry[0x1E];
			    aux         = entry[0x1F] + entry[0x20] * 256;
			    numblks     = entry[0x13] + entry[0x14] * 256;
			    size        = entry[0x15] + entry[0x16] * 256 + entry[0x17] * 65536;
			    mod         = entry[0x21] | (entry[0x22] << 8)
					| (entry[0x23] << 16) | (entry[0x24] << 24);
			    create      = entry[0x18] | (entry[0x19] << 8)
					| (entry[0x1A] << 16) | (entry[0x1B] << 24);
			    filler(buf, unix_name(entry, type, aux), fillstat(&stentry, storage, access, numblks, size, mod, create), 0);
			    entry += entrylen;
			    filecnt--;
			}
		    }
		    if (++iblk > CACHE_BLOCKS_MAX)
			cachepath[0] == '\0';
		}
		else
		    filecnt = 0;
	    } while (filecnt != 0);
	    if (!iscached)
	    {
		prodos_close(refnum, &io_buff);
		strcpy(cachepath, path);
	    }
	}
	else
	    return -ENOENT;
    }
    return 0;
}

static int a2pi_mkdir(const char *path, mode_t mode)
{
    cachepath[0] = '\0';
    return prodos_map_errno(prodos_create(path, 0xE3, 0x0F, 0, 0));
}

static int a2pi_destroy(const char *path)
{
    cachepath[0] = '\0';
    return prodos_map_errno(prodos_destroy(path));
}

static int a2pi_rename(const char *from, const char *to)
{
    cachepath[0] = '\0';
    return prodos_map_errno(prodos_rename(from, to));
}

static int a2pi_chmod(const char *path, mode_t mode)
{
    int access, type, aux, storage, size, numblks, mod, create;

    if (cache_get_file_info(path, &access, &type, &aux, &storage, &numblks, &size, &mod, &create) == 0)
    {
	access  = (mode & 0x04) ? 0x01 : 0x00;
	access |= (mode & 0x02) ? 0xC2 : 0x00;
	cachepath[0] = '\0';
	return prodos_map_errno(prodos_set_file_info(path, access, type, aux, mod, create));
    }
    return -ENOENT;
}

static int a2pi_truncate(const char *path, off_t size)
{
    int refnum, io_buff = 0;
    
    cachepath[0] = '\0';
    if ((refnum = prodos_open(path,  &io_buff)) > 0)
    {
	prodos_set_eof(refnum, size);
	return prodos_map_errno(prodos_close(refnum, &io_buff));
    }
    return prodos_map_errno(refnum);
}

static int a2pi_open(const char *path, struct fuse_file_info *fi)
{
    int refnum, io_buff = 0;
    
    if ((refnum = prodos_open(path,  &io_buff)) > 0)
    {
	return prodos_map_errno(prodos_close(refnum, &io_buff));
    }
    return prodos_map_errno(refnum);
}

static int a2pi_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int refnum, io_buff = 0;
    
    if ((refnum = prodos_open(path,  &io_buff)) > 0)
    {
	if (offset && prodos_set_mark(refnum, offset) == -PRODOS_ERR_EOF)
	    size = 0;
	if (size)
	    size = prodos_read(refnum, buf, size);
	if (size == -PRODOS_ERR_EOF)
	    size = 0;
	prodos_close(refnum, &io_buff);
	return size;
    }
    return prodos_map_errno(refnum);
}

static int a2pi_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int refnum, io_buff = 0;
    cachepath[0] = '\0';
    
    if ((refnum = prodos_open(path,  &io_buff)) > 0)
    {
	if (offset && prodos_set_mark(refnum, offset) == -PRODOS_ERR_EOF)
	    size = 0;
	if (size)
	    size = prodos_write(refnum, buf, size);
	prodos_close(refnum, &io_buff);
	return size;
    }
    return prodos_map_errno(refnum);
}

static int a2pi_create(const char * path, mode_t mode, struct fuse_file_info *fi)
{
    int refnum, io_buff = 0;
    cachepath[0] = '\0';
    
    if ((refnum = prodos_open(path,  &io_buff)) == -PRODOS_ERR_FILE_NOT_FND)
	return prodos_map_errno(prodos_create(path, 0xC3, 0x04, 0, 0));
    return prodos_map_errno(refnum);
}

static struct fuse_operations a2pi_oper = {
	.getattr	= a2pi_getattr,
	.readdir	= a2pi_readdir,
	.mkdir		= a2pi_mkdir,
	.unlink		= a2pi_destroy,
	.rmdir		= a2pi_destroy,
	.rename		= a2pi_rename,
	.chmod		= a2pi_chmod,
	.truncate	= a2pi_truncate,
	.open		= a2pi_open,
	.read		= a2pi_read,
	.write		= a2pi_write,
    	.create		= a2pi_create,
};

int main(int argc, char *argv[])
{
    pifd = a2open("127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    prodos_close(0, NULL);
    umask(0);
    return fuse_main(argc, argv, &a2pi_oper, NULL);
}
