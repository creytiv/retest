#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <pty.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_pty"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct pty_test {
	int pty;
	pid_t pid;
	int err;
};


static void abort_test(struct pty_test *test, int err)
{
	test->err = err;
	re_cancel();
}


static void pty_hdlr(int flags, void *arg)
{
	struct pty_test *test = arg;
	if (flags & FD_EXCEPT)
		abort_test(test, 0);
}


int test_pty(void)
{
	pid_t pid;
	int pty;
	struct pty_test test = {0};
	int err;

	/* Fork to pseudo-terminal */
	pid = (pid_t) forkpty(&pty, NULL, NULL, NULL);
	if (pid == -1) {
		err = errno;
		goto out;
	}

	test.pty = pty;
	test.pid = pid;

	/* Child process */
	if (pid == 0) {
		char *args[] = {"sh", "-c", "exit", NULL};
		execvp(args[0], args);
	}

	err = fd_listen(test.pty, FD_READ, pty_hdlr, &test);
	if (err)
		goto out;

	err = re_main_timeout(500);
	if (err)
		goto out;

	if (test.err) {
		err = test.err;
		goto out;
	}

 out:
	if (test.pty) {
		fd_close(test.pty);
		close(test.pty);
	}

	if (test.pid) {
		kill(test.pid, SIGTERM);
	}

	return err;
}
