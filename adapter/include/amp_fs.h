/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#ifndef AMP_FS_H
#define AMP_FS_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    unsigned int st_mode;
    unsigned int st_size;
} amp_stat_t;

/**
 * @brief amp_fs_init() initializes vfs system.
 *
 * @param[in] NULL
 *
 * @return  On success, return new file descriptor.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_fs_init();

/**
 * @brief amp_fopen() opens the file or device by its @path.
 *
 * @param[in] path   the path of the file or device to open.
 * @param[in] mode   the mode of open operation.
 *
 * @return  On success, return new file descriptor.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
void *amp_fopen(const char *path, const char *mode);

/**
 * @brief amp_fclose() closes the file or device associated with file
 *        descriptor @fd.
 *
 * @param[in] stream  the file descriptor of the file or device.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_fclose(void *stream);

/**
 * @brief amp_fread() attempts to read up to @nbytes bytes from file
 *        descriptor @fd into the buffer starting at @buf.
 *
 * @param[in]  stream  the file descriptor of the file or device.
 * @param[out] buff    the buffer to read bytes into.
 * @param[in]  count   the number of bytes to read.
 * @param[in]  size    the size of bytes.
 *
 * @return  On success, the number of bytes is returned (0 indicates end
 *          of file) and the file position is advanced by this number.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_fread(void *buff, unsigned int size, unsigned int count, void *stream);

/**
 * @brief amp_fwrite() writes up to @nbytes bytes from the buffer starting
 *        at @buf to the file referred to by the file descriptor @fd.
 *
 * @param[in] stream  the file descriptor of the file or device.
 * @param[in] buff    the buffer to write bytes from.
 * @param[in] count   the number of bytes to write.
 * @param[in]  size    the size of bytes.
 *
 * @return  On success, the number of bytes written is returned, adn the file
 *          position is advanced by this number..
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_fwrite(void *buff, unsigned int size, unsigned int count, void *stream);

/**
 * @brief amp_fseek() repositions the file offset of the open file
 *        description associated with the file descriptor @fd to the
 *        argument @offset according to the directive @whence as follows:
 *
 *        SEEK_SET: The file offset is set to @offset bytes.
 *        SEEK_CUR: The file offset is set to its current location
 *                  plus @offset bytes.
 *        SEEK_END: The file offset is set to the size of the file
 *                  plus @offset bytes.
 *
 * @param[in] stream  The file descriptor of the file.
 * @param[in] offset  The offset relative to @whence directive.
 * @param[in] whence  The start position where to seek.
 * @param[in] curpos  The current position where to seek.
 *
 * @return  On success, return the resulting offset location as measured
 *          in bytes from the beginning of the file.
 *          On error, neagtive error code is returned to indicate the cause
 *          of the error.
 */
int amp_fseek(void *stream, int offset, int whence, int *curpos);

/**
 * @brief amp_fsync causes the pending modifications of the specified file to
 *        be written to the underlying filesystems.
 *
 * @param[in] stream  the file descriptor of the file.
 *
 * @return  On success return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_fsync(void *stream);

/**
 * @brief amp_stat() return information about a file pointed to by @path
 *        in the buffer pointed to by @st.
 *
 * @param[in]  path  The path of the file to be quried.
 * @param[out] st    The buffer to receive information.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_stat(const char *path, amp_stat_t *st);

/**
 * @brief amp_remove() deletes a name from the filesystem.
 *
 * @param[in] path  The path of the file to be deleted.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_remove(const char *path);

/**
 * @brief amp_rename() renames a file, moving it between directories
 *        if required.
 *
 * @param[in] oldpath  The old path of the file to rename.
 * @param[in] newpath  The new path to rename the file to.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_rename(const char *oldpath, const char *newpath);

/**
 * @brief amp_opendir() opens a directory stream corresponding to the
 *        directory @path, and returns a pointer to the directory stream.
 *        The stream is positioned at the first entry in the directory.
 *
 * @param[in] path  the path of the directory to open.
 *
 * @return  On success, return a point of directory stream.
 *          On error, NULL is returned.
 */
void *amp_opendir(const char *path);

/**
 * @brief amp_closedir() closes the directory stream associated with
 *        @dir. A successful call to amp_closedir() also closes the
 *        underlying file descriptor associated with @dir. The directory
 *        stream descriptor @dir is not available after this call.
 *
 * @param[in] dir  The directory stream descriptor to be closed.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_closedir(void *dir);

/**
 * @brief amp_readdir() returns a pointer to an @aos_dirent_t representing
 *        the next directory entry in the directory stream pointed to by
 *        @dir. It returns Null on reaching the end of the directory stream
 *        or if an error occurred.
 *
 * @param[in] dir  The directory stream descriptor to read.
 *
 * @return  On success, amp_readdir() returns a pointer to an @aos_dirent_t
 *          structure. If the end of the directory stream is reached, NULL is
 *          returned.
 *          On error, NULL is returned.
 */
void *amp_readdir(void *dir);

/**
 * @brief amp_mkdir() attempts to create a directory named @path
 *
 * @param[in] path  The name of directory to be created.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_mkdir(const char *path);

/**
 * @brief amp_rmdir() deletes a directory, which must be emtpy.
 *
 * @param[in] path  The directory to be deleted.
 *
 * @return  On success, return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
int amp_rmdir(const char *path);

/**
 * @brief amp_rewinddir() resets the position of the directory stream @dir
 *        to the beginning of the directory.
 *
 * @param[in] dir  The directory stream descriptor pointer.
 *
 * @return  none.
 */
void amp_rewinddir(void *dir);

/**
 * @brief amp_telldir() returns the current location associated with the
 *        directory stream @dir.
 *
 * @param[in] dir  The directory stream descriptor pointer.
 *
 * @return  On success, amp_telldir() returns the current location in the
 *          directory stream.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
long amp_telldir(void *dir);

/**
 * @brief amp_seekdir() sets the location in the directory stram from
 *        which the next amp_readdir() call will start. The @loc argument
 *        should be a value returnned by a previous call to amp_telldir().
 *
 * @param[in] dir  The directory stream descriptor pointer.
 * @param[in] loc  The location in the directory stream from which the next
 *                 amp_readdir() call will start.
 *
 * @return  none.
 */
void amp_seekdir(void *dir, long loc);

/**
 * @brief amp_ftell causes the pending modifications of the specified file to
 *        be written to the underlying filesystems.
 *
 * @param[in] stream  the file descriptor of the file.
 *
 * @return  On success return 0.
 *          On error, negative error code is returned to indicate the cause
 *          of the error.
 */
long amp_ftell(void *stream);

/**
 * @brief       kv componment(key-value) initialize
 *
 * @return      0: success, -1: failed
 */
int amp_fs_type(unsigned int mode);

#if defined(__cplusplus)
}
#endif
#endif /* AMP_FS_H */
