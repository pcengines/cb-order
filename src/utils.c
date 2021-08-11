/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "utils.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *format_str(const char format[], ...)
{
	va_list ap;
	va_list aq;
	size_t len;
	char *buf;

	va_start(ap, format);
	va_copy(aq, ap);

	len = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

	buf = malloc(len + 1);
	if (buf != NULL)
		(void)vsprintf(buf, format, aq);
	va_end(aq);

	return buf;
}

bool skip_prefix(const char **str, const char *prefix)
{
	const size_t prefix_len = strlen(prefix);
	if (strncmp(*str, prefix, prefix_len) == 0) {
		*str += prefix_len;
		return true;
	}
	return false;
}

FILE *temp_file(char *template)
{
	int fd;
	FILE *file;

	fd = mkstemp(template);
	if (fd == -1)
		return NULL;

	file = fdopen(fd, "r+");
	if (file == NULL) {
		(void)unlink(template);
		return NULL;
	}

	return file;
}

static void fork_main(int pipe[2], char **argv)
{
	int null_fd;

	/* Close read end of the pipe. */
	(void)close(pipe[0]);

	/* Redirect output stream to write end of the pipe */
	if (dup2(pipe[1], STDERR_FILENO) == -1)
		_Exit(EXIT_FAILURE);
	if (dup2(pipe[1], STDOUT_FILENO) == -1)
		_Exit(EXIT_FAILURE);

	if (pipe[1] != STDERR_FILENO && pipe[1] != STDOUT_FILENO)
		/* Close write end of the pipe after it was duplicated */
		(void)close(pipe[1]);

	null_fd = open("/dev/null", O_RDWR);
	if (null_fd == -1)
		_Exit(EXIT_FAILURE);

	if (dup2(null_fd, STDIN_FILENO) == -1)
		_Exit(EXIT_FAILURE);

	if (null_fd != STDIN_FILENO && null_fd != STDOUT_FILENO)
		(void)close(null_fd);

	execvp(argv[0], argv);
	_Exit(127);
}

static int child_wait(pid_t pid)
{
	while (true) {
		int status;
		if (waitpid(pid, &status, 0) == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		if (WIFEXITED(status) || WIFSIGNALED(status))
			return status;
	}

	return -1;
}

bool run_cmd(char **argv)
{
	FILE *file;
	pid_t pid;
	int out_pipe[2];

	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	int exit_code;

	if (pipe(out_pipe) != 0)
		return false;

	pid = fork();
	if (pid == (pid_t)-1)
		return false;

	if (pid == 0) {
		fork_main(out_pipe, argv);
		return false;
	}

	/* Close write end of pipe. */
	close(out_pipe[1]);

	file = fdopen(out_pipe[0], "r");
	if (file == NULL)
		close(out_pipe[0]);

	while ((read = getline(&line, &len, file)) != -1)
		printf("%s: %s", argv[0], line);
	free(line);

	fclose(file);

	exit_code = child_wait(pid);
	if (exit_code == -1) {
		fprintf(stderr, "Waiting for %s has failed", argv[0]);
		return false;
	}

	return (exit_code == EXIT_SUCCESS);
}

bool is_input_available(FILE *stream)
{
	int c;
	int tty;
	int flags;

	tty = fileno(stream);

	/* Make reading non-blocking */
	flags = fcntl(tty, F_GETFL, 0);
	fcntl(tty, F_SETFL, flags | O_NONBLOCK);

	/* Check whether there is more input */
	c = fgetc(stream);
	if (c == EOF)
		clearerr(stream);
	else
		ungetc(c, stream);

	/* Make reading blocking again */
	fcntl(tty, F_SETFL, flags);

	return (c != EOF);
}

/* vim: set ts=8 sts=8 sw=8 noet : */
