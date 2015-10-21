#include <signal.h>

static GC_state stashed;


int paused_threads_count(GC_state s)
{
	int c = 0;
	for (int i = 0 ; i < MAXPRI ; i++) {
		if (i == 1) continue;
		if (s->threadPaused[i] == 1) c++;
	}
	return c;
}

void resume_threads(GC_state s)
{
	for(int i = 0 ; i < MAXPRI ; i++) {
		if (i == 1) continue;
		if (s->threadPaused[i] == 1) {
			pthread_kill(*(s->realtimeThreads[i]), SIGUSR2);
		}
	}
}

void quiesce_threads(GC_state s)
{
	for(uint32_t i = 0; i < MAXPRI ; i++) {
		if (i == PTHREAD_NUM) continue; // skip ourself
		if (s->threadPaused[i] == 0) {
			fprintf(stderr, "%d] Telling thread %d to pause.\n", PTHREAD_NUM, i);
			pthread_kill(*(s->realtimeThreads[i]), SIGUSR1);
		}
		else {
			fprintf(stderr, "%d] Thread %d is not ready(%d), skipping.\n", PTHREAD_NUM, i, s->threadPaused[i]);
		}
	}

	bool fp;
	do {
		fp = TRUE;
		for(uint32_t i = 0 ; i < MAXPRI ; i++) {
			if (i == PTHREAD_NUM) continue;
			fprintf(stderr, "quiesce_threads check %d (=%d)\n", i, s->threadPaused[i]);
			if (s->threadPaused[i] == 0) fp = FALSE;
			fprintf(stderr, "quiesce_threads fp = %d\n", fp);
			ssleep(1, 0); // TODO remove before collecting stats
		}
	} while (fp == FALSE);

	fprintf(stderr, "%d] all threads are paused\n", PTHREAD_NUM);
}

static void handle_resume_signal(int signum)
{
	fprintf(stderr, "%d] caught signal(%d). resuming.\n", PTHREAD_NUM, signum);
	stashed->threadPaused[PTHREAD_NUM] = 0; // TODO probably a race, but can't use mutex inside a handler.
}

static void handle_suspend_signal(int signum)
{
	sigset_t signal_set;
	fprintf(stderr, "%d] caught signal(%d). pausing.\n", PTHREAD_NUM, signum);
	sigfillset(&signal_set);
	sigdelset(&signal_set, SIGUSR2);
	stashed->threadPaused[PTHREAD_NUM] = 1; // TODO probably a race, but can't use mutex inside a handler.
	sigsuspend(&signal_set);
	fprintf(stderr, "%d] back from sigsuspend\n", PTHREAD_NUM);
}

void install_signal_handler(GC_state s)
{
	struct sigaction sigusr1, sigusr2;
	if (s) stashed = s;

	fprintf(stderr, "installing SIGUSR1 handler\n");
	sigusr1.sa_handler = handle_suspend_signal;
	sigemptyset (&sigusr1.sa_mask);
	sigusr1.sa_flags = 0;
	sigaddset (&sigusr1.sa_mask, SIGUSR2);
	if (sigaction (SIGUSR1, &sigusr1, NULL) == -1) {
		fprintf(stderr, "failed to install SIGUSR1 handler\n");
		exit(-1);
	}

	fprintf(stderr, "installing SIGUSR2 handler\n");
	sigusr2.sa_handler = handle_resume_signal;
	sigemptyset (&sigusr2.sa_mask);
	sigusr2.sa_flags = 0;
	sigaction (SIGUSR2, &sigusr2, NULL);
}
