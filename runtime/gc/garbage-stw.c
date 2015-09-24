#include <signal.h>

static GC_state stashed;

void resume_threads(GC_state s)
{
	for(int i = 0 ; i < MAXPRI ; i++) {
		if (i == 1) continue;
		s->threadPaused[i] = 0;
	}
}

void quiesce_threads(GC_state s)
{
	for(int i = 0; i < MAXPRI ; i++) {
		if (i == 1) continue; // skip GC thread
		fprintf(stderr, "%d] Telling thread %d to pause.\n", PTHREAD_NUM, i);
		pthread_kill(s->realtimeThreads[i], SIGUSR1);
	}

	bool fp;
	do {
		fp = TRUE;
		for(int i = 0 ; i < MAXPRI ; i++) {
			if (s->threadPaused[i] == 0) fp = FALSE;
		}
	} while (fp == FALSE);

	fprintf(stderr, "%d] all threads are paused\n", PTHREAD_NUM);
}

static void handle_sigusr1(int signum)
{
	fprintf(stderr, "%d] caught signal(%d). pausing.\n", PTHREAD_NUM, signum);
	stashed->threadPaused[PTHREAD_NUM] = 1;
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
