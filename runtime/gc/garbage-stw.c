#include <signal.h>

static GC_state stashed;

void resume_threads(GC_state s)
{
	for(int i = 0 ; i < MAXPRI ; i++) {
		if (i == 1) continue;
		if (s->threadPaused[i] == 1)
			s->threadPaused[i] = 0;
	}
}

int paused_threads_count(GC_state s)
{
	int c = 0;
	for (int i = 0 ; i < MAXPRI ; i++) {
		if (i == 1) continue;
		if (s->threadPaused[i] == 1) c++;
	}
	return c;
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
			fprintf(stderr, "%d] Thread %d is not ready, skipping.\n", PTHREAD_NUM, i);
		}
	}

	bool fp;
	do {
		fp = TRUE;
		for(uint32_t i = 0 ; i < MAXPRI ; i++) {
			fprintf(stderr, "check %d\n", i);
			if (i == PTHREAD_NUM) continue;
			if (s->threadPaused[i] == 0) fp = FALSE;
			fprintf(stderr, "fp = %d\n", fp);
		}
	} while (fp == FALSE);

	fprintf(stderr, "%d] all threads are paused\n", PTHREAD_NUM);
}

static void handle_sigusr1(int signum)
{
	fprintf(stderr, "%d] caught signal(%d). pausing.\n", PTHREAD_NUM, signum);
	stashed->threadPaused[PTHREAD_NUM] = 1; // TODO probably a race, but can't use mutex inside a handler.
	while(stashed->threadPaused[PTHREAD_NUM] == 1) {
		pthread_yield();
	}
	fprintf(stderr, "%d] resuming\n", PTHREAD_NUM);
}

void install_signal_handler(GC_state s)
{
	struct sigaction new_action;
	stashed = s;
	fprintf(stderr, "installing SIGUSR1 handler\n");
	new_action.sa_handler = handle_sigusr1;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction (SIGUSR1, &new_action, NULL);
}
