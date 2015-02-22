#define FUSE_USE_VERSION 26
#include "a2lib.c"
#include <fuse.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#define	FALSE	0
#define	TRUE	(!FALSE)
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
#define PRODOS_CALL    		0x0301
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
// ORG @ $301
    0x20, 0x00, 0xBF, // JSR $BF00 (PRODOS)
    0x00,             // DB CMD
    0x08, 0x03,       // DW PARAMS
    0x60,             // RTS
// PARAMS @ $308
    0x00             // PARAM_COUNT
};
/*
 * Apple II Pi connection.
 */
static int pifd = 0;
/*
 * Handy macros.
 */
#define	IS_ROOT_DIR(p)		(dirlen(p) == 0)
/*
 * Cached directory blocks.
 */
#define	IS_RAW_DEV(l,p)		((l)==8&&(p)[1]=='s'&&(p)[3]=='d'&&(p)[5]=='.'&&(p)[6]=='p'&&(p)[7]=='o')
#define CACHE_BLOCKS_MAX	16
static char cachepath[128] = "";
static unsigned char cachedata[CACHE_BLOCKS_MAX][512]; /* !!! Is this enough !!! */
static unsigned char volumes[256];
static unsigned int volblks[16];
/*
 * Apple II Pi semaphore.
 */
pthread_mutex_t a2pi_busy = PTHREAD_MUTEX_INITIALIZER;
#define	A2PI_WAIT	pthread_mutex_lock(&a2pi_busy)
#define	A2PI_RELEASE	pthread_mutex_unlock(&a2pi_busy)
/*
 * Write flags.
 */
int raw_dev_write = FALSE;
int write_perms   = 0;
/*
 * Filename & date/time conversion routines.
 */
static char hexchar(int h)
{
    h &= 0x0F;
    if (h > 9)
	h += 'A' - 10;
    else
	h += '0';
    return (char) h;
}
static char *unix_name(unsigned char *pname, int type, int aux, char *uname)
{
    int l;
    static char filename[24];
    if (uname == NULL)
	uname = filename;
    l = pname[0] & 0x0F;
    strncpy(uname, pname + 1, l);
    if (type != 0x0F && type < 0x100) /* not Directory/Raw type */
    {
	uname[l + 0] = '#';
	uname[l + 1] = hexchar(type >> 4);
	uname[l + 2] = hexchar(type);
	uname[l + 3] = hexchar(aux >> 12);
	uname[l + 4] = hexchar(aux >> 8);
	uname[l + 5] = hexchar(aux >> 4);
	uname[l + 6] = hexchar(aux);
	uname[l + 7] = '\0';
    }
    else
	uname[l] = '\0';
    return uname;
}
static int hexval(char h)
{
    if (h >= 'a')
	h -= 'a' - 10;
    else if (h >= 'A')
	h -= 'A' - 10;
    else
	h -= '0';
    return h;
}
static unsigned char *prodos_path(const char *uname, int *type, int *aux, unsigned char *pname)
{
    static unsigned char filename[72];
    int l = strlen(uname);
    if (type)
	*type = 0;
    if (aux)
	*aux  = 0;
    if (pname == NULL)
	pname = filename;
    if (l > 7 && uname[l - 7] == '#')
    {
	if (type)
	    *type = hexval(uname[l - 6]) * 16   + hexval(uname[l - 5]);
	if (aux)
	    *aux  = hexval(uname[l - 4]) * 4096 + hexval(uname[l - 3]) * 256
	          + hexval(uname[l - 2]) * 16   + hexval(uname[l - 1]);
	l    -= 7;
    }
    if (l > 64)
	l = 64;
    strncpy(pname + 1, uname, l);
    pname[0] = l;
    pname[l + 1] = '\0';
    return pname;
}
static unsigned int prodos_time(int year, int month, int day, int hour, int minute)
{
    if (year > 99)
	year -= 100;
    month += 1;
    return (day & 0x1F) | ((month & 0x0F) << 5) | ((year & 0x7F) << 9)
	|  ((minute & 0x3F) << 16) | ((hour & 0x1F) << 24);
}
static time_t unix_time(unsigned int ptime)
{
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    tm.tm_mday =  ptime        & 0x1F;
    tm.tm_mon  = (ptime >> 5)  & 0x0F;
    tm.tm_year = (ptime >> 9)  & 0x7F;
    tm.tm_min  = (ptime >> 16) & 0x3F;
    tm.tm_hour = (ptime >> 24) & 0x1F;
    if (tm.tm_year < 80)
	tm.tm_year += 100;
    tm.tm_mon -= 1;
    return mktime(&tm);
}
struct stat *unix_stat(struct stat *stbuf, int storage, int access, int blocks, int size, int mod, int create)
{
    memset(stbuf, 0, sizeof(struct stat));
    if (storage == 0x0F || storage == 0x0D)
    {
	stbuf->st_mode = (access & 0xC3) == 0xC3 ? S_IFDIR | (write_perms | 0555) : S_IFDIR | 0555;
	stbuf->st_nlink = 2;
    }
    else
    {
	stbuf->st_mode   = (access & 0xC3) == 0xC3 ? S_IFREG | (write_perms | 0444) : S_IFREG | 0444;
	stbuf->st_nlink  = 1;
	stbuf->st_blocks = blocks;
	stbuf->st_size   = size;
    }
    stbuf->st_atime = stbuf->st_mtime = unix_time(mod);
    stbuf->st_ctime = unix_time(create);
    return stbuf;
}
/*
 * ProDOS calls to Apple II Pi.
 */
#if 1
static int io_buff_mask = 0;
static int prodos_alloc_io_buff(void)
{
    int i;
    for (i = 0; i < PRODOS_IO_BUFFER_NUM; i++)
	if ((io_buff_mask & (1 << i)) == 0)
	{
	    io_buff_mask |= (1 << i);
	    //printf("Alloc io_buff $%04X\n", PRODOS_IO_BUFFER + PRODOS_IO_BUFFER_LEN * i);
	    return (PRODOS_IO_BUFFER + PRODOS_IO_BUFFER_LEN * i);
	}
    return 0;
}

static int prodos_free_io_buff(int buf)
{
    //printf("Free io_buff $%04X\n", buf);
    if (buf < PRODOS_IO_BUFFER || buf > (PRODOS_IO_BUFFER + PRODOS_IO_BUFFER_LEN * PRODOS_IO_BUFFER_NUM))
	return -1;
    int i = (buf - PRODOS_IO_BUFFER) / PRODOS_IO_BUFFER_LEN;
    io_buff_mask &= ~(1 << i);
    return i;
}
#else
#define prodos_alloc_io_buff()	PRODOS_IO_BUFFER
#define	prodos_free_io_buff(b)
#endif

static unsigned int prodos_update_time(void)
{
    char time_buff[4];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    int result, ptime = prodos_time(tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min);
    time_buff[0] = (unsigned char) ptime;
    time_buff[1] = (unsigned char) (ptime >> 8);
    time_buff[2] = (unsigned char) (ptime >> 16);
    time_buff[3] = (unsigned char) (ptime >> 24);
    result = a2write(pifd, 0xBF90, 4, time_buff);
}

static int prodos_open(unsigned char *prodos_path, int *io_buff)
{
    unsigned char refnum;
    int result;
    
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
	prodos_free_io_buff(*io_buff);
    }
    else
	result = PRODOS_ERR_UNKNOWN;
    prodos_free_io_buff(*io_buff);	
    *io_buff = 0;
    return -result;
}
static int prodos_close(int refnum, int *io_buff)
{
    int result;
    
    prodos_update_time();
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
    {
	return -result;
    }
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
	    {
		return (result == PRODOS_ERR_EOF || total_xfer) ? total_xfer : -result;
	    }
	}
	else
	{
	    return -PRODOS_ERR_UNKNOWN;
	}
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
	    {
		return -result;
	    }
	}
	else
	{
	    return -PRODOS_ERR_UNKNOWN;
	}
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
    if (!a2call(pifd, PRODOS_CALL, &result))
	result -PRODOS_ERR_UNKNOWN;
    return -result;
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
    if (!a2call(pifd, PRODOS_CALL, &result))
	result -PRODOS_ERR_UNKNOWN;
    return -result;
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
    }
    else
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_on_line(int unit, char *data_buff)
{
    int result;
    
    prodos_update_time();
    prodos[PRODOS_CMD]        = PRODOS_ON_LINE;
    prodos[PRODOS_PARAM_CNT]  = 2;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) unit;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 3, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    {
	if (result == 0)
	    a2read(pifd, PRODOS_DATA_BUFFER, (unit == 0) ? 256 : 16, data_buff);
    }
    else
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_set_file_info(unsigned char *prodos_path, int access, int type, int aux, int mod, int create)
{
    int result;
    
    prodos_update_time();
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
    if (!a2call(pifd, PRODOS_CALL, &result))
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_get_file_info(unsigned char *prodos_path, int *access, int *type, int *aux, int *storage, int *numblks, int *mod, int *create)
{
    int result;
    
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
    }
    else
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_rename(unsigned char *from, unsigned char *to)
{
    int result;
    
    prodos_update_time();
    a2write(pifd, PRODOS_DATA_BUFFER,       from[0] + 1, from);
    a2write(pifd, PRODOS_DATA_BUFFER + 256, to[0]   + 1, to);
    prodos[PRODOS_CMD]        = PRODOS_RENAME;
    prodos[PRODOS_PARAM_CNT]  = 2;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER + 256);
    prodos[PRODOS_PARAMS + 4] = (unsigned char) (PRODOS_DATA_BUFFER + 256) >> 8;
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 4, prodos);
    if (!a2call(pifd, PRODOS_CALL, &result))
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_destroy(unsigned char *prodos_path)
{
    int result;

    prodos_update_time();
    a2write(pifd, PRODOS_DATA_BUFFER, prodos_path[0] + 1,  prodos_path);
    prodos[PRODOS_CMD]        = PRODOS_DESTROY;
    prodos[PRODOS_PARAM_CNT]  = 1;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 2, prodos);
    if (!a2call(pifd, PRODOS_CALL, &result))
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_create(unsigned char *prodos_path, char access, char type, int aux, int create)
{
    int result;

    prodos_update_time();
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
    if (!a2call(pifd, PRODOS_CALL, &result))
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_read_block(int unit, char *data_buff, int block_num)
{
    int result;
    
    prodos[PRODOS_CMD]        = PRODOS_READ_BLOCK;
    prodos[PRODOS_PARAM_CNT]  = 3;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) unit;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 4] = (unsigned char) block_num;
    prodos[PRODOS_PARAMS + 5] = (unsigned char) (block_num >> 8);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 5, prodos);
    if (a2call(pifd, PRODOS_CALL, &result))
    {
	if (result == 0)
	    a2read(pifd, PRODOS_DATA_BUFFER, 512, data_buff);
    }
    else
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_write_block(int unit, const char *data_buff, int block_num)
{
    int result;
    
    a2write(pifd, PRODOS_DATA_BUFFER, 512, (char *)data_buff);
    prodos[PRODOS_CMD]        = PRODOS_WRITE_BLOCK;
    prodos[PRODOS_PARAM_CNT]  = 3;
    prodos[PRODOS_PARAMS + 1] = (unsigned char) unit;
    prodos[PRODOS_PARAMS + 2] = (unsigned char) PRODOS_DATA_BUFFER;
    prodos[PRODOS_PARAMS + 3] = (unsigned char) (PRODOS_DATA_BUFFER >> 8);
    prodos[PRODOS_PARAMS + 4] = (unsigned char) block_num;
    prodos[PRODOS_PARAMS + 5] = (unsigned char) (block_num >> 8);
    a2write(pifd, PRODOS_CALL, PRODOS_CALL_LEN + 5, prodos);
    if (!a2call(pifd, PRODOS_CALL, &result))
	result = PRODOS_ERR_UNKNOWN;
    return -result;
}
static int prodos_map_errno(int perr)
{
    int uerr = 0;
    if (perr)
    {
        prodos_close(0, 0); /* Close all files */
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
	    case -PRODOS_ERR_VOL_DIR_FULL:
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
static int dirlen(const char *path)
{
    int dl;
    for (dl = strlen(path) - 1; dl >= 0; dl--)
	if (path[dl] == '/')
	    return dl;
    return -1;
}

static int cache_get_file_info(const char *path, int *access, int *type, int *aux, int *storage, int *numblks, int *size, int *mod, int *create)
{
    char dirpath[128], filename[32];
    unsigned char *entry, prodos_name[65], data_buff[512];
    int refnum, iblk, entrylen, entriesblk, filecnt, io_buff = 0;
    int iscached, slot, drive, i, dl, l = strlen(path);
    
    if ((dl = dirlen(path)) == 0)
    {
	if (IS_RAW_DEV(l, path))
	{
	    /*
	     * Raw device.
	     */
	    slot     = (path[2] - '0') & 0x07;
	    drive    = (path[4] - '1') & 0x01;
	    *storage = 0x100;
	    *type    = 0x00;
	    *access  = raw_dev_write ? 0xC3 : 0x01;
	    *aux     = 0;
	    *numblks = volblks[slot | (drive << 3)];
	    *size    = *numblks*512;
	    *mod     = 0;
	    *create  = 0;
	}
	else
	{
	    /*
	     * Volume directory.
	     */
	    *storage = 0x0F;
	    *type    = 0x0F;
	    *access  = 0xC3;
	    *aux     = 0;
	    *numblks = 0;
	    *size    = 0;
	    *mod     = 0;
	    *create  = 0;
	}
    }
    else
    {
	strncpy(dirpath, path, dl);
	dirpath[dl] = '\0';
	//printf("Match path %s to cached dir %s\n", dirpath, cachepath);
	iscached =(strcmp(dirpath, cachepath) == 0);
	if (iscached || (refnum = prodos_open(prodos_path(dirpath, NULL, NULL, NULL), &io_buff)) > 0)
	{
	    strcpy(filename, path + dl + 1);
	    l = l - dl - 1;
	    if (l > 7 && filename[l - 7] == '#')
	    {
		l -= 7;
		filename[l] = '\0';
	    }
	    //printf("Match filename %s len %d\n", filename, l);
	    iblk = 0;
	    do
	    {
		if (iscached || prodos_read(refnum, data_buff, 512) == 512)
		{
		    if (iscached)
		    {
			if (iblk == 0)
			{
			    entrylen   = cachedata[0][0x23];
			    entriesblk = cachedata[0][0x24];
			    filecnt    = cachedata[0][0x25] + cachedata[0][0x26] * 256;
			    entry      = &cachedata[0][4] + entrylen;
			    //printf("Cached entrylen = %d, filecnt = %d\n", entrylen, filecnt);
			}
			else
			    entry      = &cachedata[iblk][4];
		    }
		    else
		    {
			if (iblk == 0)
			{
			    entrylen   = data_buff[0x23];
			    entriesblk = data_buff[0x24];
			    filecnt    = data_buff[0x25] + data_buff[0x26] * 256;
			    entry      = &data_buff[4] + entrylen;
			    //printf("Uncached entrylen = %d, filecnt = %d\n", entrylen, filecnt);
			}
			else
			    entry = &data_buff[4];
		    }
		    for (i = (iblk == 0) ? 1 : 0; i < entriesblk && filecnt; i++)
		    {
			if (entry[0])
			{
			    //entry[(entry[0] & 0x0F) + 1] = 0; printf("Searching directory entry: %s len %d\n", entry + 1, entry[0] & 0x0F);
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
				    //printf("Cache hit: %s access = $%02X, type = $%02X, aux = $%04X, storage = $%02X, size = %d\n", filename, *access, *type, *aux, *storage, *size);
				    if (!iscached)
					prodos_close(refnum, &io_buff);
				    return 0;
				}
			    }
			    entry += entrylen;
			    filecnt--;
			}
		    }
		}
		else
		    filecnt = 0;
		iblk++;
	    } while (filecnt != 0);
	    if (!iscached)
		prodos_close(refnum, &io_buff);
	}
	if (prodos_get_file_info(prodos_path(path, NULL, NULL, prodos_name), access, type, aux, storage, numblks, mod, create) == 0)
	{
	    //printf("get_file_indfo: %s access = $%02X, type = $%02X, aux = $%04X, storage = $%02X\n", path, *access, *type, *aux, *storage);
	    if (*storage == 0x0F || *storage == 0x0D)
		*size = 0;
	    else
	    {
		if ((refnum = prodos_open(prodos_name, &io_buff) > 0))
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
    }
    return 0;
}

static int a2pi_getattr(const char *path, struct stat *stbuf)
{
    int access, type, aux, storage, size, numblks, mod, create, refnum, err = 0, io_buff = 0;
    if (strcmp(path, "/") == 0 || strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
    {
	/*
	 * Root directory of volumes.
	 */
	unix_stat(stbuf, 0x0F, 0xC3, 0, 0, 0, 0);
    }
    else
    {
	/*
	 * Get file info.
	 */
	A2PI_WAIT;
	if (cache_get_file_info(path, &access, &type, &aux, &storage, &numblks, &size, &mod, &create) == 0)
	{
	    if (storage == 0x0F || storage == 0x0D)
		size = 0;
	    unix_stat(stbuf, storage, access, numblks, size, mod, create);
	}
	else
	    err = -ENOENT;
	A2PI_RELEASE;
    }
    return err;
}

static int a2pi_access(const char *path, int mask)
{
    int storage, access, type, numblks, size, aux, mod, create, access_ok = 0;

    A2PI_WAIT;
    if (cache_get_file_info(path, &access, &type, &aux, &storage, &numblks, &size, &mod, &create) == 0)
    {
	//printf("Check access bits $%02X to mask 0x%02X\n", access, mask);
	if ((mask & R_OK) && !(access & 0x01))
	    access_ok = -1;
	if ((mask & W_OK) && ((access & 0xC2) != 0xC2))
	    access_ok = -1;
	if ((mask & X_OK) && (type != 0x0F))
	    access_ok = -1;
    }
    else
	access_ok = -1;
    A2PI_RELEASE;
    return access_ok;
}

static int a2pi_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    unsigned char data_buff[512], filename[16], *entry;
    int storage, access, type, blocks, size, aux, mod, create, err = 0;
    int i, l, iscached, slot, drive, refnum, iblk, entrylen, entriesblk, filecnt, io_buff = 0;
    struct stat stentry, straw;

    (void) offset;
    (void) fi;
    if (strcmp(path, "/") == 0)
    {
	/*
	 * Root directory, fill with volume names.
	 */
	unix_stat(&stentry, 0x0F, 0xC3, 0, 0, 0, 0);
	filler(buf, ".", &stentry, 0);
	filler(buf, "..", &stentry, 0);
	for (i = 0; i < 256; i += 16)
	    if ((l = volumes[i] & 0x0F))
	    {
		/*
		 * Add volume directory.
		 */
		strncpy(filename, volumes + i + 1, l);
		filename[l] = '\0';
		filler(buf, filename, &stentry, 0);
	    }
	memset(&straw, 0, sizeof(struct stat));
	for (i = 0; i < 16; i++)
	    if (volblks[i] > 0)
	    {
		/*
		 * Add raw device.
		 */
		slot  = i & 0x07;
		drive = (i >> 3) & 0x01;
		strcpy(filename, "s0d0.po");
		filename[1] = slot + '0';
		filename[3] = drive + '1';
		straw.st_mode   = raw_dev_write ? S_IFREG | 0644 : S_IFREG | 0444;
		straw.st_nlink  = 1;
		straw.st_blocks = volblks[i];
		straw.st_size   = straw.st_blocks * 512;
		filler(buf, filename, &straw, 0);
	    }
    }
    else
    {
	/*
	 * Read ProDOS directory.
	 */
	iscached = (strcmp(path, cachepath) == 0);
	A2PI_WAIT;
	if (iscached || (refnum = prodos_open(prodos_path(path, NULL, NULL, NULL), &io_buff)) > 0)
 	{
	    unix_stat(&stentry, 0x0F, 0xC3, 0, 0, 0, 0);
	    filler(buf, ".", &stentry, 0);
	    filler(buf, "..", &stentry, 0);
	    iblk = 0;
	    do
	    {
		//if (!iscached) printf("Fill cache block %d\n", iblk);
		if (iscached || prodos_read(refnum, cachedata[iblk], 512) == 512)
		{
		    entry = &cachedata[iblk][4];
		    if (iblk == 0)
		    {
			entrylen   = cachedata[0][0x23];
			entriesblk = cachedata[0][0x24];
			filecnt    = cachedata[0][0x25] + cachedata[0][0x26] * 256;
			entry      = entry + entrylen;
			//printf("Fill cache entrylen = %d, filecnt = %d\n", entrylen, filecnt);
		    }
		    for (i = (iblk == 0) ? 1 : 0; i < entriesblk && filecnt; i++)
		    {
			if ((l = entry[0x00] & 0x0F) != 0)
			{
			    storage     = entry[0x00] >> 4;
			    type        = entry[0x10];
			    access      = entry[0x1E];
			    aux         = entry[0x1F] + entry[0x20] * 256;
			    blocks      = entry[0x13] + entry[0x14] * 256;
			    size        = entry[0x15] + entry[0x16] * 256 + entry[0x17] * 65536;
			    mod         = entry[0x21] | (entry[0x22] << 8)
					| (entry[0x23] << 16) | (entry[0x24] << 24);
			    create      = entry[0x18] | (entry[0x19] << 8)
					| (entry[0x1A] << 16) | (entry[0x1B] << 24);
			    filler(buf, unix_name(entry, type, aux, NULL), unix_stat(&stentry, storage, access, blocks, size, mod, create), 0);
			    entry += entrylen;
			    filecnt--;
			}
		    }
		    if (++iblk > CACHE_BLOCKS_MAX)
		    {
			//printf("Cache overfill!\n");
			path[0] == '\0'; /* invalidate cache path */
			iblk = 1;        /* wrap iblk around      */
		    }
		}
		else
		    filecnt = 0;
	    } while (filecnt != 0);
	    if (!iscached)
	    {
		//printf("Cache %d directory blocks\n", iblk);
		prodos_close(refnum, &io_buff);
		strcpy(cachepath, path);
	    }
	}
	else
	    err = -ENOENT;
	A2PI_RELEASE;
    }
    return err;
}

static int a2pi_mkdir(const char *path, mode_t mode)
{
    int err;
    
    if (IS_ROOT_DIR(path))
	return -EACCES;
    A2PI_WAIT;
    cachepath[0] = '\0';
    err = prodos_map_errno(prodos_create(prodos_path(path, NULL, NULL, NULL), 0xC3, 0x0F, 0, 0));
    A2PI_RELEASE;
    return err;
}

static int a2pi_remove(const char *path)
{
    int err;
    
    if (IS_ROOT_DIR(path))
	return -EACCES;
    A2PI_WAIT;
    cachepath[0] = '\0';
    err = prodos_map_errno(prodos_destroy(prodos_path(path, NULL, NULL, NULL)));
    A2PI_RELEASE;
    return err;
}

static int a2pi_rename(const char *from, const char *to)
{
    unsigned char prodos_from[65], prodos_to[65], err;

    if (IS_ROOT_DIR(from) || IS_ROOT_DIR(to))
	return -EACCES;
    prodos_path(from, NULL, NULL, prodos_from);
    prodos_path(to,   NULL, NULL, prodos_to);
    A2PI_WAIT;
    cachepath[0] = '\0';
    err = prodos_map_errno(prodos_rename(prodos_from, prodos_to));
    A2PI_RELEASE;
    return err;
}

static int a2pi_chmod(const char *path, mode_t mode)
{
    int access, type, aux, storage, size, numblks, mod, create, err;

    if (IS_ROOT_DIR(path))
	return -EACCES;
    A2PI_WAIT;
    if (cache_get_file_info(path, &access, &type, &aux, &storage, &numblks, &size, &mod, &create) == 0)
    {
	access  = (mode & 0x04) ? 0x01 : 0x00;
	access |= (mode & 0x02) ? 0xC2 : 0x00;
	cachepath[0] = '\0';
	err = prodos_map_errno(prodos_set_file_info(prodos_path(path, NULL, NULL, NULL), access, type, aux, 0, 0));
    }
    else
	err = -ENOENT;
    A2PI_RELEASE;
    return err;
}

static int a2pi_truncate(const char *path, off_t size)
{
    int refnum, err, io_buff = 0;
    
    if (IS_ROOT_DIR(path))
	return -EACCES;
    A2PI_WAIT;
    cachepath[0] = '\0';
    if ((refnum = prodos_open(prodos_path(path, NULL, NULL, NULL),  &io_buff)) > 0)
    {
	prodos_set_eof(refnum, size);
	err = prodos_map_errno(prodos_close(refnum, &io_buff));
    }
    else
	err = prodos_map_errno(refnum);
    A2PI_RELEASE;
    return err;
}

static int a2pi_open(const char *path, struct fuse_file_info *fi)
{
    int slot, drive, refnum, err, io_buff = 0;

    if (IS_ROOT_DIR(path))
    {
	if (IS_RAW_DEV(strlen(path), path))
	{
	    slot     = (path[2] - '0') & 0x07;
	    drive    = (path[4] - '1') & 0x01;
	    if (volblks[slot | (drive << 3)] > 0)
		return 0;
	}
	return -ENOENT;
    }
    A2PI_WAIT;
    if ((refnum = prodos_open(prodos_path(path, NULL, NULL, NULL),  &io_buff)) > 0)
	err = prodos_map_errno(prodos_close(refnum, &io_buff));
    else
	err = prodos_map_errno(refnum);
    A2PI_RELEASE;
    return err;
}

static int a2pi_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int slot, drive, unit, devsize, blkcnt, iblk, refnum, io_buff = 0;
    
    if (IS_ROOT_DIR(path))
    {
	if (IS_RAW_DEV(strlen(path), path))
	{
	    slot     = (path[2] - '0') & 0x07;
	    drive    = (path[4] - '1') & 0x01;
	    unit     = (slot << 4) | (drive << 7);
	    devsize  = volblks[slot | (drive << 3)] * 512;
	    if (offset > devsize)
		return 0;
	    if ((offset + size) > devsize)
		size = devsize - offset;
	    if (offset & 0x01FF)
		return 0; /* force block aligned reads */
	    if (size & 0x01FF)
		return 0; /* force block sized reads */
	    blkcnt = size >> 9;
	    iblk = offset >> 9;
	    while (blkcnt--)
	    {
		A2PI_WAIT;
		refnum = prodos_read_block(unit, buf, iblk);
		A2PI_RELEASE;
		if (refnum < 0)
		    return prodos_map_errno(refnum);
		buf += 512;
		iblk++;
	    }
	    return size;
	}
	return -ENOENT;
    }
    A2PI_WAIT;
    if ((refnum = prodos_open(prodos_path(path, NULL, NULL, NULL), &io_buff)) > 0)
    {
	if (offset && prodos_set_mark(refnum, offset) == -PRODOS_ERR_EOF)
	    size = 0;
	if (size)
	    size = prodos_read(refnum, buf, size);
	if (size == -PRODOS_ERR_EOF)
	    size = 0;
	prodos_close(refnum, &io_buff);
    }
    else
	size = prodos_map_errno(refnum);
    A2PI_RELEASE;
    return size;
}

static int a2pi_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int slot, drive, unit, devsize, blkcnt, iblk, refnum, io_buff = 0;

    A2PI_WAIT;
    cachepath[0] = '\0';
    A2PI_RELEASE;
    if (IS_ROOT_DIR(path))
    {
	if (raw_dev_write)
	{
	    if (IS_RAW_DEV(strlen(path), path))
	    {
		slot     = (path[2] - '0') & 0x07;
		drive    = (path[4] - '1') & 0x01;
		unit     = (slot << 4) | (drive << 7);
		devsize  = volblks[slot | (drive << 3)] * 512;
		if (offset > devsize)
		    return 0;
		if ((offset + size) > devsize)
		    size = devsize - offset;
		if (offset & 0x01FF)
		    return 0; /* force block aligned writes */
		if (size & 0x01FF)
		    return 0; /* force block sized writes */
		blkcnt = size >> 9;
		iblk = offset >> 9;
		while (blkcnt--)
		{
		    A2PI_WAIT;
		    refnum = prodos_write_block(unit, buf, iblk);
		    A2PI_RELEASE;
		    if (refnum < 0)
			return prodos_map_errno(refnum);
		    buf += 512;
		    iblk++;
		}
		return size;
	    }
	    return -ENOENT;
	}
	else
	    return -EACCES;
    }
    A2PI_WAIT;
    if ((refnum = prodos_open(prodos_path(path, NULL, NULL, NULL), &io_buff)) > 0)
    {
	if (offset)
	    prodos_set_mark(refnum, offset);
	if (size)
	    size = prodos_write(refnum, buf, size);
	prodos_close(refnum, &io_buff);
    }
    else
	size = prodos_map_errno(refnum);
    A2PI_RELEASE;
    return size;
}

int a2pi_flush(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int a2pi_create(const char * path, mode_t mode, struct fuse_file_info *fi)
{
    unsigned char prodos_name[65];
    int refnum, type, aux, err = 0, io_buff = 0;

    if (IS_ROOT_DIR(path))
	return -EACCES;
    A2PI_WAIT;
    cachepath[0] = '\0';
    if ((refnum = prodos_open(prodos_path(path, &type, &aux, prodos_name), &io_buff)) == -PRODOS_ERR_FILE_NOT_FND)
    {
	err = prodos_map_errno(prodos_create(prodos_name, 0xC3, type, aux, 0));
    }
    else if (refnum > 0)
    {
	prodos_set_eof(refnum, 0);
	prodos_close(refnum, &io_buff);
    }
    else
	err = prodos_map_errno(refnum);
    A2PI_RELEASE;
    return err;
}

static int a2pi_statfs(const char *path, struct statvfs *stbuf)
{
    unsigned char voldir[16], voldata[512];
    int i, storage, access, type, numblks, aux, mod, create, err = 0;
    
    memset(stbuf, 0, sizeof(struct statvfs));
    /*
     * Copy first element of path = volume directory.
     */
    voldir[1] = '/';
    for (i = 1; path[i] && path[i] != '/'; i++)
	voldir[i + 1] = path[i];
    voldir[0] = i;
    A2PI_WAIT;
    if ((prodos_get_file_info(voldir, &access, &type, &aux, &storage, &numblks, &mod, &create)) > 0)
    {
	stbuf->f_bsize   = 512;
	stbuf->f_fsid    = 0xa2;
	stbuf->f_namemax = 22; /* include space for #typeattrib */
	stbuf->f_blocks  = aux;
	stbuf->f_bfree   = 
	stbuf->f_bavail  = aux - numblks;
    }
    else
	err = -1;
    A2PI_RELEASE;
    return err;
}

int a2pi_utimens(const char *path, const struct timespec tv[2])
{
    int access, type, aux, storage, size, numblks, mod, create, refnum, err, io_buff = 0;
    struct tm *tm = localtime(&tv[1].tv_sec);
    cachepath[0] = '\0';
    /*
     * Get file info.
     */
    A2PI_WAIT;
    if (cache_get_file_info(path, &access, &type, &aux, &storage, &numblks, &size, &mod, &create) == 0)
	err = prodos_map_errno(prodos_set_file_info(prodos_path(path, NULL, NULL, NULL),
						    access,
						    type,
						    aux,
						    prodos_time(tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min),
						    0));
    else
	err = -ENOENT;
    A2PI_RELEASE;
    return err;
}

static struct fuse_operations a2pi_oper = {
    .getattr	= a2pi_getattr,
    .access	= a2pi_access,
    .readdir	= a2pi_readdir,
    .mkdir	= a2pi_mkdir,
    .unlink	= a2pi_remove,
    .rmdir	= a2pi_remove,
    .rename	= a2pi_rename,
    .chmod	= a2pi_chmod,
    .truncate	= a2pi_truncate,
    .open	= a2pi_open,
    .read	= a2pi_read,
    .write	= a2pi_write,
    .flush	= a2pi_flush,
    .create	= a2pi_create,
    .statfs	= a2pi_statfs,
    .utimens	= a2pi_utimens,
};

int main(int argc, char *argv[])
{
    int i, l, access, type, aux, storage, size, numblks, mod, create;
    char volpath[17];
    
    pifd = a2open("127.0.0.1");
    if (pifd < 0)
    {
	perror("Unable to connect to Apple II Pi");
	exit(EXIT_FAILURE);
    }
    prodos_close(0, NULL);
    if (prodos_on_line(0, volumes))
	exit(-1);
    for (i = 0; i < 256; i += 16)
	if ((l = volumes[i] & 0x0F) != 0)
	{
	    /*
	     * Get volume size.
	     */
	    strncpy(volpath + 2, volumes + i + 1, l);
	    volpath[0] = l + 1;
	    volpath[1] = '/';
	    prodos_get_file_info(volpath, &access, &type, &aux, &storage, &numblks, &mod, &create);
	    volblks[volumes[i] >> 4] = aux;
	}
    /*
     * Always add 5 1/4 floppy raw devices.
     */
    if (volblks[0x06] == 0)
	volblks[0x06] = 280;
    if (volblks[0x0E] == 0)
	volblks[0x0E] = 280;
    if (strcmp(argv[argc - 1], "+rw") == 0)
    {
	raw_dev_write = TRUE;
        write_perms   = 0220;
	argc--;
    }
    umask(0);
    return fuse_main(argc, argv, &a2pi_oper, NULL);
}
