#include <signal.h>

void resume_threads(GC_state s)
{

}

void quiesce_threads(GC_state s)
{
	for(int i = 0; i < MAXPRI ; i++) {
		if (i == 1) continue; // skip GC thread
		fprintf("Telling thread %d to pause.\n", i);
		pthread_kill(s->realtimeThreads[i], SIGUSR1);
	}

	bool fp;
	do {
		fp = TRUE;
		for(int i = 0 ; i < MAXPRI ; i++) {
			if (s->threadPaused[i] == 0) fp = FALSE;
		}
	} while (fp == FALSE);

	fprintf(stderr, "all threads are paused\n");
}

static void handle_sigusr1(int signum)
{
	fprintf(stderr, "%d] caught signal %d\n", PTHREAD_NUM, signum);
}

void install_signal_handler()
{
	struct sigaction new_action;
	fprintf(stderr, "installing SIGUSR1 handler\n");
	new_action.sa_handler = handle_sigusr1;
	sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	sigaction (SIGUSR1, &new_action, NULL);
}
