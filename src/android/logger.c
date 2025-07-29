#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <android/log.h>
#include "logger.h"

#ifndef APP_NAME
#define APP_NAME	"alephnull"
#endif

static void *thread_func(void *arg);

static int pfd[2];
static pthread_t thr;
static int initialized;

int start_logger(void)
{
	if(initialized) {
		return 1;
	}

	/* set stdout to line-buffered, and stderr to unbuffered */
	setvbuf(stdout, 0, _IOLBF, 0);
	setvbuf(stderr, 0, _IONBF, 0);

	if(pipe(pfd) == -1) {
		perror("failed to create logging pipe");
		return -1;
	}
	assert(pfd[0] > 2 && pfd[1] > 2);

	/* redirect stdout & stderr to the write-end of the pipe */
	dup2(pfd[1], 1);
	dup2(pfd[1], 2);

	/* start the logging thread */
	if(pthread_create(&thr, 0, thread_func, 0) == -1) {
		perror("failed to spawn logging thread");
		return -1;
	}
	pthread_detach(thr);
	return 0;
}

static void *thread_func(void *arg)
{
	ssize_t rdsz;
	char buf[257];

	__android_log_print(ANDROID_LOG_DEBUG, APP_NAME, "logger starting up...");

	while((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
		if(buf[rdsz - 1] == '\n') {
			--rdsz;
		}
		buf[rdsz] = 0;
		__android_log_write(ANDROID_LOG_DEBUG, APP_NAME, buf);
	}

	__android_log_print(ANDROID_LOG_DEBUG, APP_NAME, "logger shutting down...");
	return 0;
}
