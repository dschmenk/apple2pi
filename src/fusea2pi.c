#define FUSE_USE_VERSION 26
#include "a2lib.c"
#include <fuse.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

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
		return -result;
	}
	else
	    return -PRODOS_ERR_UNKNOWN;
    }
    return total_xfer;
}
static int prodos_write(int refnum, char *data_buff, int req_xfer)
{
    int result, short_req, short_xfer, total_xfer = 0;

    prodos[PRODOS_CMD]        = PRODOS_WRITE;
    prodos[PRODOS_PARAM_CNT]  = 4;
    prodos[PRODOS_PARAMS + 1] = refnum;
    while (req_xfer)
    {
	short_req = (req_xfer > PRODOS_DATA_BUFFER_LEN) ? PRODOS_DATA_BUFFER_LEN : req_xfer;
	a2write(pifd, PRODOS_DATA_BUFFER, short_req, data_buff + total_xfer);
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
static int prodos_destroy(char *path)
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
/*
 * FUSE functions.
 */
static int basenamelen(const char *path)
{
    int i, l = strlen(path);
    for (i = l - 1; i >= 0 && path[i] != '/'; i--);
    return l - i - 1;
}
static int a2pi_getattr(const char *path, struct stat *stbuf)
{
    int access, type, aux, storage, numblks, mod, create, refnum, io_buff = 0;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
	/*
	 * Root directory of volumes.
	 */
	stbuf->st_mode = S_IFDIR | 0777;
	stbuf->st_nlink = 2;
    }
    else
    {
	/*
	 * Get file info.
	 */
	if (prodos_get_file_info(path, &access, &type, &aux, &storage, &numblks, &mod, &create) == 0)
	{
	    printf("prodos access = $%02X, type = $%02X, aux = $%04X, storage = $%02X\n", access, type, aux, storage);
	    if (storage == 0x0F || storage == 0x0D)
	    {
		stbuf->st_mode = (access & 0xC3 == 0xC3) ? S_IFDIR | 0777 : S_IFDIR | 0444;
		stbuf->st_nlink = 2;
	    }
	    else
	    {
		stbuf->st_mode    = (access & 0xC3 == 0xC3) ? S_IFREG | 0666 : S_IFREG | 0444;
		stbuf->st_nlink   = 1;
		stbuf->st_blksize = 512;
		stbuf->st_blocks  = numblks;
 #if 0
		stbuf->st_size  = numblks * 512;
 #else
		if ((refnum = prodos_open(path, &io_buff) > 0))
		{
		    stbuf->st_size  = prodos_get_eof(refnum);
		    prodos_close(refnum, &io_buff);
		}
		else
		    return -PRODOS_ERR_UNKNOWN;    
#endif
	    }
	}
	else
	    return -ENOENT;
    }
    return 0;
}

static int a2pi_access(const char *path, int mask)
{
    return 0;
}

static int a2pi_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    unsigned char data_buff[512], filename[16], *entry, type;
    int i, l, refnum, firstblk, entrylen, entriesblk, filecnt, io_buff = 0;
    (void) offset;
    (void) fi;
    
    if (strcmp(path, "/") == 0)
    {
	/*
	 * Root directory, fill with volume names.
	 */
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	if (prodos_on_line(data_buff) == 0)
	{
	    for (i = 0; i < 256; i += 16)
		if ((l = data_buff[i] & 0x0F))
		{
		    strncpy(filename, data_buff + i + 1, l);
		    filename[l] = '\0';
		    filler(buf, filename, NULL, 0);
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
	printf("Read ProDOS directory %s\n", path);
	if ((refnum = prodos_open(path, &io_buff)) > 0)
	{
	    firstblk = 1;
	    do
	    {
		if (prodos_read(refnum, data_buff, 512) == 512)
		{
		    entry = data_buff + 4;
		    if (firstblk)
		    {
			entrylen   = data_buff[0x23];
			entriesblk = data_buff[0x24];
			filecnt    = data_buff[0x25] + data_buff[0x26] * 256;
			entry      = entry + entrylen;
		    }
		    for (i = firstblk; i < entriesblk; i++)
		    {
			if ((type = entry[0]) != 0)
			{
			    l = type & 0x0F;
			    strncpy(filename, entry + 1, l);
			    filename[l] = '\0';
			    filler(buf, filename, NULL, 0);
			    // if type & $F0 == $D0 ; Is it a directory?
			    filecnt = filecnt - 1;
			    entry  += entrylen;
			}
		    }
		    firstblk = 0;
		}
		else
		    filecnt = 0;
	    } while (filecnt != 0);
	    prodos_close(refnum, &io_buff);
	}
	else
	    return -ENOENT;
    }
    return 0;
}

static int a2pi_mkdir(const char *path, mode_t mode)
{
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int a2pi_unlink(const char *path)
{
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int a2pi_rmdir(const char *path)
{
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int a2pi_rename(const char *from, const char *to)
{
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int a2pi_chmod(const char *path, mode_t mode)
{
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int a2pi_truncate(const char *path, off_t size)
{
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int a2pi_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int a2pi_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int a2pi_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int a2pi_statfs(const char *path, struct statvfs *stbuf)
{
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int a2pi_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int a2pi_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int a2pi_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int a2pi_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations a2pi_oper = {
	.getattr	= a2pi_getattr,
	.access		= a2pi_access,
	.readdir	= a2pi_readdir,
	.mkdir		= a2pi_mkdir,
	.unlink		= a2pi_unlink,
	.rmdir		= a2pi_rmdir,
	.rename		= a2pi_rename,
	.chmod		= a2pi_chmod,
	.truncate	= a2pi_truncate,
	.open		= a2pi_open,
	.read		= a2pi_read,
	.write		= a2pi_write,
	.statfs		= a2pi_statfs,
#ifdef HAVE_SETXATTR
	.setxattr	= a2pi_setxattr,
	.getxattr	= a2pi_getxattr,
	.listxattr	= a2pi_listxattr,
	.removexattr	= a2pi_removexattr,
#endif
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
