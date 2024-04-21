/*
assfile - library for accessing assets with an fopen/fread-like interface
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "assfile_impl.h"

#ifdef BUILD_MOD_URL
#include <pthread.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include "tpool.h"
#include "md4.h"

enum {
	DL_UNKNOWN,
	DL_STARTED,
	DL_ERROR,
	DL_DONE
};

struct file_info {
	char *url;
	char *cache_fname;

	FILE *cache_file;

	/* fopen-thread waits until the state becomes known (request starts transmitting or fails) */
	int state;
	pthread_cond_t state_cond;
	pthread_mutex_t state_mutex;
};

static void *fop_open(const char *fname, void *udata);
static void fop_close(void *fp, void *udata);
static long fop_seek(void *fp, long offs, int whence, void *udata);
static long fop_read(void *fp, void *buf, long size, void *udata);

static void exit_cleanup(void);
static void download(void *data);
static size_t recv_callback(char *ptr, size_t size, size_t nmemb, void *udata);
static const char *get_temp_dir(void);
static int mkdir_path(const char *path);

static char *tmpdir, *cachedir;
static struct thread_pool *tpool;
static CURL **curl;

struct ass_fileops *ass_alloc_url(const char *url)
{
	static int done_init;
	struct ass_fileops *fop;
	int i, len;
	char *ptr;

	if(!done_init) {
		curl_global_init(CURL_GLOBAL_ALL);
		atexit(exit_cleanup);

		if(!*ass_mod_url_cachedir) {
			strcpy(ass_mod_url_cachedir, "assfile_cache");
		}
		tmpdir = (char*)get_temp_dir();
		if(!(cachedir = malloc(strlen(ass_mod_url_cachedir) + strlen(tmpdir) + 2))) {
			fprintf(stderr, "assfile mod_url: failed to allocate cachedir path buffer\n");
			goto init_failed;
		}
		sprintf(cachedir, "%s/%s", tmpdir, ass_mod_url_cachedir);

		if(mkdir_path(cachedir) == -1) {
			fprintf(stderr, "assfile mod_url: failed to create cache directory: %s\n", cachedir);
			goto init_failed;
		}

		if(ass_mod_url_max_threads <= 0) {
			ass_mod_url_max_threads = 8;
		}

		if(!(curl = calloc(ass_mod_url_max_threads, sizeof *curl))) {
			perror("assfile: failed to allocate curl context table");
			goto init_failed;
		}
		for(i=0; i<ass_mod_url_max_threads; i++) {
			if(!(curl[i] = curl_easy_init())) {
				goto init_failed;
			}
			curl_easy_setopt(curl[i], CURLOPT_WRITEFUNCTION, recv_callback);
		}

		if(!(tpool = ass_tpool_create(ass_mod_url_max_threads))) {
			fprintf(stderr, "assfile: failed to create thread pool\n");
			goto init_failed;
		}

		done_init = 1;
	}

	if(!(fop = malloc(sizeof *fop))) {
		return 0;
	}
	len = strlen(url);
	if(!(fop->udata = malloc(len + 1))) {
		free(fop);
		return 0;
	}
	memcpy(fop->udata, url, len + 1);
	if(len) {
		ptr = (char*)fop->udata + len - 1;
		while(*ptr == '/') *ptr-- = 0;
	}

	fop->open = fop_open;
	fop->close = fop_close;
	fop->seek = fop_seek;
	fop->read = fop_read;
	return fop;

init_failed:
	free(cachedir);
	if(curl) {
		for(i=0; i<ass_mod_url_max_threads; i++) {
			if(curl[i]) {
				curl_easy_cleanup(curl[i]);
			}
		}
		free(curl);
	}
	return 0;
}

static void exit_cleanup(void)
{
	int i;

	if(tpool) {
		ass_tpool_destroy(tpool);
	}
	if(curl) {
		for(i=0; i<ass_mod_url_max_threads; i++) {
			if(curl[i]) {
				curl_easy_cleanup(curl[i]);
			}
		}
		free(curl);
	}
	curl_global_cleanup();
}


void ass_free_url(struct ass_fileops *fop)
{
	free(fop->udata);
}

static char *cache_filename(const char *fname, const char *url)
{
	MD4_CTX md4ctx;
	unsigned char sum[16];
	char sumstr[33];
	char *resfname;
	int i, prefix_len;
	int url_len = strlen(url);

	MD4Init(&md4ctx);
	MD4Update(&md4ctx, (unsigned char*)url, url_len);
	MD4Final((unsigned char*)sum, &md4ctx);

	for(i=0; i<16; i++) {
		sprintf(sumstr + i * 2, "%02x", (unsigned int)sum[i]);
	}
	sumstr[32] = 0;

	prefix_len = strlen(cachedir);
	if(!(resfname = malloc(prefix_len + 64))) {
		return 0;
	}
	sprintf(resfname, "%s/%s", cachedir, sumstr);
	return resfname;
}

static void *fop_open(const char *fname, void *udata)
{
	struct file_info *file;
	int state;
	char *prefix = udata;

	if(!fname || !*fname) {
		ass_errno = ENOENT;
		return 0;
	}

	if(!(file = malloc(sizeof *file))) {
		ass_errno = ENOMEM;
		return 0;
	}

	if(!(file->url = malloc(strlen(prefix) + strlen(fname) + 2))) {
		perror("assfile: mod_url: failed to allocate url buffer");
		ass_errno = errno;
		free(file);
		return 0;
	}
	if(prefix && *prefix) {
		sprintf(file->url, "%s/%s", prefix, fname);
	} else {
		strcpy(file->url, fname);
	}

	if(!(file->cache_fname = cache_filename(fname, file->url))) {
		free(file->url);
		free(file);
		ass_errno = ENOMEM;
		return 0;
	}
	if(!(file->cache_file = fopen(file->cache_fname, "wb"))) {
		fprintf(stderr, "assfile: mod_url: failed to open cache file (%s) for writing: %s\n",
				file->cache_fname, strerror(errno));
		ass_errno = errno;
		free(file->url);
		free(file->cache_fname);
		free(file);
		return 0;
	}

	file->state = DL_UNKNOWN;
	pthread_mutex_init(&file->state_mutex, 0);
	pthread_cond_init(&file->state_cond, 0);

	if(ass_verbose) {
		fprintf(stderr, "assfile: mod_url: get \"%s\" -> \"%s\"\n", file->url, file->cache_fname);
	}
	ass_tpool_enqueue(tpool, file, download, 0);

	/* wait until the file changes state */
	pthread_mutex_lock(&file->state_mutex);
	while(file->state == DL_UNKNOWN) {
		pthread_cond_wait(&file->state_cond, &file->state_mutex);
	}
	state = file->state;
	pthread_mutex_unlock(&file->state_mutex);

	if(state == DL_ERROR) {
		/* the worker stopped, so we can safely cleanup and return error */
		fclose(file->cache_file);
		remove(file->cache_fname);
		free(file->cache_fname);
		free(file->url);
		pthread_cond_destroy(&file->state_cond);
		pthread_mutex_destroy(&file->state_mutex);
		free(file);
		ass_errno = ENOENT;	/* TODO: differentiate between 403 and 404 */
		return 0;
	}
	return file;
}

static void wait_done(struct file_info *file)
{
	pthread_mutex_lock(&file->state_mutex);
	while(file->state != DL_DONE && file->state != DL_ERROR) {
		pthread_cond_wait(&file->state_cond, &file->state_mutex);
	}
	pthread_mutex_unlock(&file->state_mutex);
}

static void fop_close(void *fp, void *udata)
{
	struct file_info *file = fp;

	wait_done(file);	/* TODO: stop download instead of waiting to finish */

	fclose(file->cache_file);
	if(file->state == DL_ERROR) remove(file->cache_fname);
	free(file->cache_fname);
	free(file->url);
	pthread_cond_destroy(&file->state_cond);
	pthread_mutex_destroy(&file->state_mutex);
	free(file);
}

static long fop_seek(void *fp, long offs, int whence, void *udata)
{
	struct file_info *file = fp;
	wait_done(file);

	fseek(file->cache_file, offs, whence);
	return ftell(file->cache_file);
}

static long fop_read(void *fp, void *buf, long size, void *udata)
{
	struct file_info *file = fp;
	wait_done(file);

	return fread(buf, 1, size, file->cache_file);
}

/* this is the function called by the worker threads to perform the download
 * signal state changes, and prepare the cache file for reading
 */
static void download(void *data)
{
	int tid, res;
	struct file_info *file = data;

	tid = ass_tpool_thread_id(tpool);

	curl_easy_setopt(curl[tid], CURLOPT_URL, file->url);
	curl_easy_setopt(curl[tid], CURLOPT_WRITEDATA, file);
	res = curl_easy_perform(curl[tid]);

	pthread_mutex_lock(&file->state_mutex);
	if(res == CURLE_OK) {
		file->state = DL_DONE;
		fclose(file->cache_file);
		if(!(file->cache_file = fopen(file->cache_fname, "rb"))) {
			fprintf(stderr, "assfile: failed to reopen cache file (%s) for reading: %s\n",
					file->cache_fname, strerror(errno));
			file->state = DL_ERROR;
		}
	} else {
		file->state = DL_ERROR;
	}
	pthread_cond_broadcast(&file->state_cond);
	pthread_mutex_unlock(&file->state_mutex);
}

/* this function is called by curl to pass along downloaded data chunks */
static size_t recv_callback(char *ptr, size_t size, size_t count, void *udata)
{
	struct file_info *file = udata;

	pthread_mutex_lock(&file->state_mutex);
	if(file->state == DL_UNKNOWN) {
		file->state = DL_STARTED;
		pthread_cond_broadcast(&file->state_cond);
	}
	pthread_mutex_unlock(&file->state_mutex);

	return fwrite(ptr, size, count, file->cache_file);
}

#ifdef WIN32
#include <windows.h>

static const char *get_temp_dir(void)
{
	static char buf[MAX_PATH + 1];
	GetTempPathA(MAX_PATH + 1, buf);
	return buf;
}
#else	/* UNIX */
static const char *get_temp_dir(void)
{
	char *env = getenv("TMPDIR");
	return env ? env : "/tmp";
}
#endif


static int mkdir_path(const char *path)
{
	char *pathbuf, *dptr;
	struct stat st;

	if(!path || !*path) return -1;

	pathbuf = dptr = alloca(strlen(path) + 1);

	while(*path) {
		while(*path) {
			int c = *path++;
			*dptr++ = c;

			if(c == '/' || c == '\\') break;
		}
		*dptr = 0;

		if(stat(pathbuf, &st) == -1) {
			/* path component does not exist, create it */
#ifdef WIN32
			if(mkdir(pathbuf) == -1) {
#else
			if(mkdir(pathbuf, 0777) == -1) {
#endif
				return -1;
			}
		}
	}

	return 0;
}

#else	/* don't build mod_url */
struct ass_fileops *ass_alloc_url(const char *url)
{
	fprintf(stderr, "assfile: compiled without URL asset source support\n");
	return 0;
}

void ass_free_url(struct ass_fileops *fop)
{
}
#endif
