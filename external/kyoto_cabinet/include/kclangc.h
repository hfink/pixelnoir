/*************************************************************************************************
 * C language binding
 *                                                               Copyright (C) 2009-2010 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#ifndef _KCLANGC_H                       /* duplication check */
#define _KCLANGC_H

#if defined(__cplusplus)
extern "C" {
#endif


#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdint.h>


/**
 * C wrapper of polymorphic database.
 */
typedef union {
  void* db;                              /**< dummy member */
} KCDB;


/**
 * Error codes.
 */
enum {
  KCESUCCESS,                            /**< success */
  KCENOIMPL,                             /**< not implemented */
  KCEINVALID,                            /**< invalid operation */
  KCENOREPOS,                            /**< no repository */
  KCENOPERM,                             /**< no permission */
  KCEBROKEN,                             /**< broken file */
  KCEDUPREC,                             /**< record duplication */
  KCENOREC,                              /**< no record */
  KCELOGIC,                              /**< logical inconsistency */
  KCESYSTEM,                             /**< system error */
  KCEMISC = 15                           /**< miscellaneous error */
};


/**
 * Open modes.
 */
enum {
  KCOREADER = 1 << 0,                    /**< open as a reader */
  KCOWRITER = 1 << 1,                    /**< open as a writer */
  KCOCREATE = 1 << 2,                    /**< writer creating */
  KCOTRUNCATE = 1 << 3,                  /**< writer truncating */
  KCOAUTOTRAN = 1 << 4,                  /**< auto transaction */
  KCOAUTOSYNC = 1 << 5,                  /**< auto synchronization */
  KCONOLOCK = 1 << 6,                    /**< open without locking */
  KCOTRYLOCK = 1 << 7,                   /**< lock without blocking */
  KCONOREPAIR = 1 << 8                   /**< open without auto repair */
};


/**
 * Merge modes.
 */
enum {
  KCMSET,                                /**< overwrite the existing value */
  KCMADD,                                /**< keep the existing value */
  KCMREPLACE,                            /**< modify the existing record only */
  KCMAPPEND                              /**< append the new value */
};


/** The package version. */
extern const char* const KCVERSION;


/** Special pointer for no operation by the visiting function. */
extern const char* const KCVISNOP;


/** Special pointer to remove the record by the visiting function. */
extern const char* const KCVISREMOVE;


/**
 * Call back function to visit a full record.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param vbuf the pointer to the value region.
 * @param vsiz the size of the value region.
 * @param sp the pointer to the variable into which the size of the region of the return
 * value is assigned.
 * @param opq an opaque pointer.
 * @return If it is the pointer to a region, the value is replaced by the content.  If it
 * is KCVISNOP, nothing is modified.  If it is KCVISREMOVE, the record is removed.
 */
typedef const char* (*KCVISITFULL)(const char* kbuf, size_t ksiz,
                                   const char* vbuf, size_t vsiz, size_t* sp, void* opq);


/**
 * Call back function to visit an empty record.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param sp the pointer to the variable into which the size of the region of the return
 * value is assigned.
 * @param opq an opaque pointer.
 * @return If it is the pointer to a region, the value is replaced by the content.  If it
 * is KCVISNOP or KCVISREMOVE, nothing is modified.
 */
typedef const char* (*KCVISITEMPTY)(const char* kbuf, size_t ksiz, size_t* sp, void* opq);


/**
 * Call back function to process the database file.
 * @param path the path of the database file.
 * @param count the number of records.
 * @param size the size of the available region.
 * @param opq an opaque pointer.
 * @return true on success, or false on failure.
 */
typedef int32_t (*KCFILEPROC)(const char* path, int64_t count, int64_t size, void* opq);


/**
 * C wrapper of polymorphic cursor.
 */
typedef union {
  void* cur;                             /**< dummy member */
} KCCUR;


/**
 * Allocate a region on memory.
 * @param size the size of the region.
 * @return the pointer to the allocated region.  The region of the return value should be
 * released with the kcfree function when it is no longer in use.
 */
void* kcmalloc(size_t size);


/**
 * Release a region allocated in the library.
 * @param ptr the pointer to the region.
 */
void kcfree(void* ptr);


/**
 * Get the time of day in seconds.
 * @return the time of day in seconds.  The accuracy is in microseconds.
 */
double kctime(void);


/**
 * Convert a string to an integer.
 * @param str specifies the string.
 * @return the integer.  If the string does not contain numeric expression, 0 is returned.
 */
int64_t kcatoi(const char* str);


/**
 * Convert a string with a metric prefix to an integer.
 * @param str the string, which can be trailed by a binary metric prefix.  "K", "M", "G", "T",
 * "P", and "E" are supported.  They are case-insensitive.
 * @return the integer.  If the string does not contain numeric expression, 0 is returned.  If
 * the integer overflows the domain, INT64_MAX or INT64_MIN is returned according to the
 * sign.
 */
int64_t kcatoix(const char* str);


/**
 * Convert a string to a real number.
 * @param str specifies the string.
 * @return the real number.  If the string does not contain numeric expression, 0.0 is
 * returned.
 */
double kcatof(const char* str);


/**
 * Get the hash value by MurMur hashing.
 * @param buf the source buffer.
 * @param size the size of the source buffer.
 * @return the hash value.
 */
uint64_t kchashmurmur(const void* buf, size_t size);


/**
 * Get the hash value by FNV hashing.
 * @param buf the source buffer.
 * @param size the size of the source buffer.
 * @return the hash value.
 */
uint64_t kchashfnv(const void* buf, size_t size);


/**
 * Get the quiet Not-a-Number value.
 * @return the quiet Not-a-Number value.
 */
double kcnan();


/**
 * Get the positive infinity value.
 * @return the positive infinity value.
 */
double kcinf();


/**
 * Check a number is a Not-a-Number value.
 * @return true for the number is a Not-a-Number value, or false if not.
 */
int32_t kcchknan(double num);


/**
 * Check a number is an infinity value.
 * @return true for the number is an infinity value, or false if not.
 */
int32_t kcchkinf(double num);


/**
 * Get the readable string of an error code.
 * @param code the error code.
 * @return the readable string of the error code.
 */
const char* kcecodename(int32_t code);


/**
 * Create a database object.
 * @return the created database object.
 * @note The object of the return value should be released with the kcdbdel function when it is
 * no longer in use.
 */
KCDB* kcdbnew(void);


/**
 * Destroy a database object.
 * @param db the database object.
 */
void kcdbdel(KCDB* db);


/**
 * Open a database file.
 * @param db a database object.
 * @param path the path of a database file.  If it is "-", the database will be a prototype
 * hash database.  If it is "+", the database will be a prototype tree database.  If it is
 * "*", the database will be a cache hash database.  If it is "%", the database will be a
 * cache tree database.  If its suffix is ".kch", the database will be a file hash database.
 * If its suffix is ".kct", the database will be a file tree database.  If its suffix is
 * ".kcd", the database will be a directory hash database.  If its suffix is ".kcf", the
 * database will be a directory tree database.  Otherwise, this function fails.  Tuning
 * parameters can trail the name, separated by "#".  Each parameter is composed of the name
 * and the value, separated by "=".  If the "type" parameter is specified, the database type
 * is determined by the value in "-", "+", "*", "%", "kch", "kct", "kcd", and "kcf".  All
 * database types support the logging parameters of "log", "logkinds", and "logpx".  The
 * prototype hash database and the prototype tree database do not support any other tuning
 * parameter.  The cache hash database supports "opts", "bnum", "zcomp", "capcount", "capsize",
 * and "zkey".  The cache tree database supports all parameters of the cache hash database
 * except for capacity limitation, and supports "psiz", "rcomp", "pccap" in addition.  The
 * file hash database supports "apow", "fpow", "opts", "bnum", "msiz", "dfunit", "zcomp", and
 * "zkey".  The file tree database supports all parameters of the file hash database and
 * "psiz", "rcomp", "pccap" in addition.  The directory hash database supports "opts", "zcomp",
 * and "zkey".  The directory tree database supports all parameters of the directory hash
 * database and "psiz", "rcomp", "pccap" in addition.
 * @param mode the connection mode.  KCOWRITER as a writer, KCOREADER as a reader.
 * The following may be added to the writer mode by bitwise-or: KCOCREATE, which means
 * it creates a new database if the file does not exist, KCOTRUNCATE, which means it
 * creates a new database regardless if the file exists, KCOAUTOTRAN, which means each
 * updating operation is performed in implicit transaction, KCOAUTOSYNC, which means
 * each updating operation is followed by implicit synchronization with the file system.  The
 * following may be added to both of the reader mode and the writer mode by bitwise-or:
 * KCONOLOCK, which means it opens the database file without file locking,
 * KCOTRYLOCK, which means locking is performed without blocking, KCONOREPAIR, which
 * means the database file is not repaired implicitly even if file destruction is detected.
 * @return true on success, or false on failure.
 * @note The tuning parameter "log" is for the original "tune_logger" and the value specifies
 * the path of the log file, or "-" for the standard output, or "+" for the standard error.
 * "logkinds" specifies kinds of logged messages and the value can be "debug", "info", "warn",
 * or "error".  "logpx" specifies the prefix of each log message.  "opts" is for "tune_options"
 * and the value can contain "s" for the small option, "l" for the linear option, and "c" for
 * the compress option.  "bnum" corresponds to "tune_bucket".  "zcomp" is for "tune_compressor"
 * and the value can be "zlib" for the ZLIB raw compressor, "def" for the ZLIB deflate
 * compressor, "gz" for the ZLIB gzip compressor, "lzo" for the LZO compressor, "lzma" for the
 * LZMA compressor, or "arc" for the Arcfour cipher.  "zkey" specifies the cipher key of the
 * compressor.  "capcount" is for "cap_count".  "capsize" is for "cap_size".  "psiz" is for
 * "tune_page".  "rcomp" is for "tune_comparator" and the value can be "lex" for the lexical
 * comparator or "dec" for the decimal comparator.  "pccap" is for "tune_page_cache".  "apow"
 * is for "tune_alignment".  "fpow" is for "tune_fbp".  "msiz" is for "tune_map".  "dfunit" is
 * for "tune_defrag".  Every opened database must be closed by the kcdbclose method when it is
 * no longer in use.  It is not allowed for two or more database objects in the same process to
 * keep their connections to the same database file at the same time.
 */
int32_t kcdbopen(KCDB* db, const char* path, uint32_t mode);


/**
 * Close the database file.
 * @param db a database object.
 * @return true on success, or false on failure.
 */
int32_t kcdbclose(KCDB* db);


/**
 * Get the code of the last happened error.
 * @param db a database object.
 * @return the code of the last happened error.
 */
int32_t kcdbecode(KCDB* db);


/**
 * Get the supplement message of the last happened error.
 * @param db a database object.
 * @return the supplement message of the last happened error.
 */
const char* kcdbemsg(KCDB* db);


/**
 * Accept a visitor to a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param fullproc a call back function to visit a record.
 * @param emptyproc a call back function to visit an empty record space.
 * @param opq an opaque pointer to be given to the call back functions.
 * @param writable true for writable operation, or false for read-only operation.
 * @return true on success, or false on failure.
 * @note the operation for each record is performed atomically and other threads accessing the
 * same record are blocked.
 */
int32_t kcdbaccept(KCDB* db, const char* kbuf, size_t ksiz,
                   KCVISITFULL fullproc, KCVISITEMPTY emptyproc, void* opq, int32_t writable);


/**
 * Iterate to accept a visitor for each record.
 * @param db a database object.
 * @param fullproc a call back function to visit a record.
 * @param opq an opaque pointer to be given to the call back function.
 * @param writable true for writable operation, or false for read-only operation.
 * @return true on success, or false on failure.
 * @note the whole iteration is performed atomically and other threads are blocked.
 */
int32_t kcdbiterate(KCDB* db, KCVISITFULL fullproc, void* opq, int32_t writable);


/**
 * Set the value of a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param vbuf the pointer to the value region.
 * @param vsiz the size of the value region.
 * @return true on success, or false on failure.
 * @note If no record corresponds to the key, a new record is created.  If the corresponding
 * record exists, the value is overwritten.
 */
int32_t kcdbset(KCDB* db, const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz);


/**
 * Add a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param vbuf the pointer to the value region.
 * @param vsiz the size of the value region.
 * @return true on success, or false on failure.
 * @note If no record corresponds to the key, a new record is created.  If the corresponding
 * record exists, the record is not modified and false is returned.
 */
int32_t kcdbadd(KCDB* db, const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz);


/**
 * Replace the value of a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param vbuf the pointer to the value region.
 * @param vsiz the size of the value region.
 * @return true on success, or false on failure.
 * @note If no record corresponds to the key, no new record is created and false is returned.
 * If the corresponding record exists, the value is modified.
 */
int32_t kcdbreplace(KCDB* db, const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz);


/**
 * Append the value of a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param vbuf the pointer to the value region.
 * @param vsiz the size of the value region.
 * @return true on success, or false on failure.
 * @note If no record corresponds to the key, a new record is created.  If the corresponding
 * record exists, the given value is appended at the end of the existing value.
 */
int32_t kcdbappend(KCDB* db, const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz);


/**
 * Add a number to the numeric value of a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param num the additional number.
 * @return the result value, or INT64_MIN on failure.
 */
int64_t kcdbincrint(KCDB* db, const char* kbuf, size_t ksiz, int64_t num);


/**
 * Add a number to the numeric value of a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param num the additional number.
 * @return the result value, or Not-a-number on failure.
 */
double kcdbincrdouble(KCDB* db, const char* kbuf, size_t ksiz, double num);


/**
 * Perform compare-and-swap.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param ovbuf the pointer to the old value region.  NULL means that no record corresponds.
 * @param ovsiz the size of the old value region.
 * @param nvbuf the pointer to the new value region.  NULL means that the record is removed.
 * @param nvsiz the size of new old value region.
 * @return true on success, or false on failure.
 */
int32_t kcdbcas(KCDB* db, const char* kbuf, size_t ksiz,
                const char* nvbuf, size_t nvsiz, const char* ovbuf, size_t ovsiz);


/**
 * Remove a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @return true on success, or false on failure.
 * @note If no record corresponds to the key, false is returned.
 */
int32_t kcdbremove(KCDB* db, const char* kbuf, size_t ksiz);


/**
 * Retrieve the value of a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param sp the pointer to the variable into which the size of the region of the return
 * value is assigned.
 * @return the pointer to the value region of the corresponding record, or NULL on failure.
 * @note If no record corresponds to the key, NULL is returned.  Because an additional zero
 * code is appended at the end of the region of the return value, the return value can be
 * treated as a C-style string.  The region of the return value should be released with the
 * kcfree function when it is no longer in use.
 */
char* kcdbget(KCDB* db, const char* kbuf, size_t ksiz, size_t* sp);


/**
 * Retrieve the value of a record.
 * @param db a database object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @param vbuf the pointer to the buffer into which the value of the corresponding record is
 * written.
 * @param max the size of the buffer.
 * @return the size of the value, or -1 on failure.
 */
int32_t kcdbgetbuf(KCDB* db, const char* kbuf, size_t ksiz, char* vbuf, size_t max);


/**
 * Remove all records.
 * @param db a database object.
 * @return true on success, or false on failure.
 */
int32_t kcdbclear(KCDB* db);


/**
 * Synchronize updated contents with the file and the device.
 * @param db a database object.
 * @param hard true for physical synchronization with the device, or false for logical
 * synchronization with the file system.
 * @param proc a postprocessor call back function.  If it is NULL, no postprocessing is
 * performed.
 * @param opq an opaque pointer to be given to the call back function.
 * @return true on success, or false on failure.
 */
int32_t kcdbsync(KCDB* db, int32_t hard, KCFILEPROC proc, void* opq);


/**
 * Create a copy of the database file.
 * @param db a database object.
 * @param dest the path of the destination file.
 * @return true on success, or false on failure.
 */
int32_t kcdbcopy(KCDB* db, const char* dest);


/**
 * Begin transaction.
 * @param db a database object.
 * @param hard true for physical synchronization with the device, or false for logical
 * synchronization with the file system.
 * @return true on success, or false on failure.
 */
int32_t kcdbbegintran(KCDB* db, int32_t hard);


/**
 * Try to begin transaction.
 * @param db a database object.
 * @param hard true for physical synchronization with the device, or false for logical
 * synchronization with the file system.
 * @return true on success, or false on failure.
 */
int32_t kcdbbegintrantry(KCDB* db, int32_t hard);


/**
 * End transaction.
 * @param db a database object.
 * @param commit true to commit the transaction, or false to abort the transaction.
 * @return true on success, or false on failure.
 */
int32_t kcdbendtran(KCDB* db, int32_t commit);


/**
 * Dump records into a file.
 * @param db a database object.
 * @param dest the path of the destination file.
 * @return true on success, or false on failure.
 */
int32_t kcdbdumpsnap(KCDB* db, const char* dest);


/**
 * Load records from a file.
 * @param db a database object.
 * @param src the path of the source file.
 * @return true on success, or false on failure.
 */
int32_t kcdbloadsnap(KCDB* db, const char* src);


/**
 * Get the number of records.
 * @param db a database object.
 * @return the number of records, or -1 on failure.
 */
int64_t kcdbcount(KCDB* db);


/**
 * Get the size of the database file.
 * @param db a database object.
 * @return the size of the database file in bytes, or -1 on failure.
 */
int64_t kcdbsize(KCDB* db);


/**
 * Get the path of the database file.
 * @param db a database object.
 * @return the path of the database file, or an empty string on failure.
 * @note The region of the return value should be released with the kcfree function when it is
 * no longer in use.
 */
char* kcdbpath(KCDB* db);


/**
 * Get the miscellaneous status information.
 * @param db a database object.
 * @return the result string of tab saparated values, or NULL on failure.  Each line consists of
 * the attribute name and its value separated by a tab character.
 * @note The region of the return value should be released with the kcfree function when it is
 * no longer in use.
 */
char* kcdbstatus(KCDB* db);


/**
 * Get keys matching a prefix string.
 * @param db a database object.
 * @param prefix the prefix string.
 * @param strary an array to contain the result.  Its size must be sufficient.
 * @param max the maximum number to retrieve.
 * @return the number of retrieved keys or -1 on failure.
 * @note The region of each element of the result should be released with the kcfree function
 * when it is no longer in use.
 */
int64_t kcdbmatchprefix(KCDB* db, const char* prefix, char** strary, int64_t max);


/**
 * Get keys matching a regular expression string.
 * @param db a database object.
 * @param regex the regular expression string.
 * @param strary an array to contain the result.  Its size must be sufficient.
 * @param max the maximum number to retrieve.
 * @return the number of retrieved keys or -1 on failure.
 * @note The region of each element of the result should be released with the kcfree function
 * when it is no longer in use.
 */
int64_t kcdbmatchregex(KCDB* db, const char* regex, char** strary, int64_t max);


/**
 * Merge records from other databases.
 * @param db a database object.
 * @param srcary an array of the source detabase objects.
 * @param srcnum the number of the elements of the source array.
 * @param mode the merge mode.  KCMSET to overwrite the existing value, KCMADD to keep the
 * existing value, KCMREPLACE to modify the existing record only, KCMAPPEND to append the new
 * value.
 * @return true on success, or false on failure.
 */
int32_t kcdbmerge(KCDB* db, KCDB** srcary, size_t srcnum, uint32_t mode);


/**
 * Create a cursor object.
 * @param db a database object.
 * @return the return value is the created cursor object.
 * @note The object of the return value should be released with the kccurdel function when it is
 * no longer in use.
 */
KCCUR* kcdbcursor(KCDB* db);


/**
 * Destroy a cursor object.
 * @param cur the cursor object.
 */
void kccurdel(KCCUR* cur);


/**
 * Accept a visitor to the current record.
 * @param cur a cursor object.
 * @param fullproc a call back function to visit a record.
 * @param opq an opaque pointer to be given to the call back functions.
 * @param writable true for writable operation, or false for read-only operation.
 * @param step true to move the cursor to the next record, or false for no move.
 * @return true on success, or false on failure.
 */
int32_t kccuraccept(KCCUR* cur, KCVISITFULL fullproc, void* opq,
                    int32_t writable, int32_t step);


/**
 * Set the value of the current record.
 * @param cur a cursor object.
 * @param vbuf the pointer to the value region.
 * @param vsiz the size of the value region.
 * @param step true to move the cursor to the next record, or false for no move.
 * @return true on success, or false on failure.
 */
int32_t kccursetvalue(KCCUR* cur, const char* vbuf, size_t vsiz, int32_t step);


/**
 * Remove the current record.
 * @param cur a cursor object.
 * @return true on success, or false on failure.
 * @note If no record corresponds to the key, false is returned.  The cursor is moved to the
 * next record implicitly.
 */
int32_t kccurremove(KCCUR* cur);


/**
 * Get the key of the current record.
 * @param cur a cursor object.
 * @param sp the pointer to the variable into which the size of the region of the return value
 * is assigned.
 * @param step true to move the cursor to the next record, or false for no move.
 * @return the pointer to the key region of the current record, or NULL on failure.
 * @note If the cursor is invalidated, NULL is returned.  Because an additional zero
 * code is appended at the end of the region of the return value, the return value can be
 * treated as a C-style string.  The region of the return value should be released with the
 * kcfree function when it is no longer in use.
 */
char* kccurgetkey(KCCUR* cur, size_t* sp, int32_t step);


/**
 * Get the value of the current record.
 * @param cur a cursor object.
 * @param sp the pointer to the variable into which the size of the region of the return value
 * is assigned.
 * @param step true to move the cursor to the next record, or false for no move.
 * @return the pointer to the value region of the current record, or NULL on failure.
 * @note If the cursor is invalidated, NULL is returned.  Because an additional zero
 * code is appended at the end of the region of the return value, the return value can be
 * treated as a C-style string.  The region of the return value should be released with the
 * kcfree function when it is no longer in use.
 */
char* kccurgetvalue(KCCUR* cur, size_t* sp, int32_t step);


/**
 * Get a pair of the key and the value of the current record.
 * @param cur a cursor object.
 * @param ksp the pointer to the variable into which the size of the region of the return
 * value is assigned.
 * @param vbp the pointer to the variable into which the pointer to the value region is
 * assigned.
 * @param vsp the pointer to the variable into which the size of the value region is
 * assigned.
 * @param step true to move the cursor to the next record, or false for no move.
 * @return the pointer to the pair of the key region, or NULL on failure.
 * @note If the cursor is invalidated, NULL is returned.  Because an additional zero code is
 * appended at the end of each region of the key and the value, each region can be treated
 * as a C-style string.  The region of the return value should be released with the kcfree
 * function when it is no longer in use.
 */
char* kccurget(KCCUR* cur, size_t* ksp, const char** vbp, size_t* vsp, int32_t step);


/**
 * Jump the cursor to the first record for forward scan.
 * @param cur a cursor object.
 * @return true on success, or false on failure.
 */
int32_t kccurjump(KCCUR* cur);


/**
 * Jump the cursor to a record for forward scan.
 * @param cur a cursor object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @return true on success, or false on failure.
 */
int32_t kccurjumpkey(KCCUR* cur, const char* kbuf, size_t ksiz);


/**
 * Jump the cursor to the last record for backward scan.
 * @param cur a cursor object.
 * @return true on success, or false on failure.
 * @note This method is dedicated to tree databases.  Some database types, especially hash
 * databases, may provide a dummy implementation.
 */
int32_t kccurjumpback(KCCUR* cur);


/**
 * Jump the cursor to a record for backward scan.
 * @param cur a cursor object.
 * @param kbuf the pointer to the key region.
 * @param ksiz the size of the key region.
 * @return true on success, or false on failure.
 * @note This method is dedicated to tree databases.  Some database types, especially hash
 * databases, will provide a dummy implementation.
 */
int32_t kccurjumpbackkey(KCCUR* cur, const char* kbuf, size_t ksiz);


/**
 * Step the cursor to the next record.
 * @param cur a cursor object.
 * @return true on success, or false on failure.
 */
int32_t kccurstep(KCCUR* cur);


/**
 * Step the cursor to the previous record.
 * @param cur a cursor object.
 * @return true on success, or false on failure.
 * @note This method is dedicated to tree databases.  Some database types, especially hash
 * databases, may provide a dummy implementation.
 */
int32_t kccurstepback(KCCUR* cur);


/**
 * Get the database object.
 * @param cur a cursor object.
 * @return the database object.
 */
KCDB* kccurdb(KCCUR* cur);


/**
 * Get the code of the last happened error.
 * @param cur a cursor object.
 * @return the code of the last happened error.
 */
int32_t kccurecode(KCCUR* cur);


/**
 * Get the supplement message of the last happened error.
 * @param cur a cursor object.
 * @return the supplement message of the last happened error.
 */
const char* kccuremsg(KCCUR* cur);


#if defined(__cplusplus)
}
#endif

#endif                                   /* duplication check */

/* END OF FILE */
