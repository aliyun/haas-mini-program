/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_platform.h"
#include "amp_fs.h"

#define JSE_FS_USER_DIR "."

int amp_get_user_dir(char *dir)
{
    memcpy(dir, JSE_FS_USER_DIR, strlen(JSE_FS_USER_DIR));
    dir[strlen(JSE_FS_USER_DIR)] = '\0';
    return 0;
}

int amp_fs_init()
{
    return 0;
}

void *amp_fopen(const char *path, const char *mode)
{
    return (void *)fopen(path, mode);
}

int amp_fclose(void *stream)
{
    return fclose((FILE *)stream);
}

int amp_fseek(void *stream, int offset, int whence, int *curpos)
{
    *curpos = -1;
    return fseek((FILE *)stream, offset, whence);
}

int amp_fread(void *buff, unsigned int size, unsigned int count, void *stream)
{
    return fread(buff, (size_t)size, (size_t)count, (FILE *)stream);
}

int amp_fwrite(void *buff, unsigned int size, unsigned int count, void *stream)
{
    return fwrite(buff, size, count, stream);
}

int amp_fsync(void *stream)
{
    return 0;
}

int amp_stat(const char *path, amp_stat_t *st)
{
	struct stat _stat;
    int ret = stat(path, &_stat);
	st->st_mode = _stat.st_mode;
	st->st_size = _stat.st_size;
	return ret;
}

int amp_remove(const char *path)
{
    return unlink(path);
}

int amp_mkdir(const char *path)
{
    return mkdir(path, mkdir);
}

int amp_rmdir(const char *path)
{
    return rmdir(path);
}


long amp_ftell(void *stream)
{
    return ftell((FILE *)stream);
}

int amp_fs_type(unsigned int mode)
{
	return S_ISREG(mode);
}