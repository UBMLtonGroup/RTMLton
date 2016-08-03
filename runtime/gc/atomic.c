/* Copyright (C) 1999-2005 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* s->atomicState is modified in other places, eg switch-thread.c */

void incAtomicBy (GC_state s, uint32_t v) {
        int lockop, unlockop;
        lockop = pthread_mutex_lock(&lock);
        assert(lockop == 0);
        s->atomicState += v;
        unlockop = pthread_mutex_unlock(&lock);
        assert(unlockop == 0);
}

void decAtomicBy (GC_state s, uint32_t v) {
        int lockop, unlockop;
        lockop = pthread_mutex_lock(&lock);
        assert(lockop == 0);
        s->atomicState -= v;
        unlockop = pthread_mutex_unlock(&lock);
        assert(unlockop == 0);
}

void setAtomic (GC_state s, uint32_t v) {
        int lockop, unlockop;
        lockop = pthread_mutex_lock(&lock);
        assert(lockop == 0);
        s->atomicState = v;
        unlockop = pthread_mutex_unlock(&lock);
        assert(unlockop == 0);
}

void incAtomic (GC_state s) {
        int lockop, unlockop;
        lockop = pthread_mutex_lock(&lock);
        assert(lockop == 0);
        s->atomicState++;
        unlockop = pthread_mutex_unlock(&lock);
        assert(unlockop == 0);
}

void decAtomic (GC_state s) {
        int lockop, unlockop;
        lockop = pthread_mutex_lock(&lock);
        assert(lockop == 0);
        s->atomicState--;
        unlockop = pthread_mutex_unlock(&lock);
        assert(unlockop == 0);
}

void beginAtomic (GC_state s) {
	int lockop, unlockop;

	lockop = pthread_mutex_lock(&lock);
	assert(lockop == 0);
	s->atomicState++;
	unlockop = pthread_mutex_unlock(&lock);
	assert(unlockop == 0);

	if (0 == s->limit)
		s->limit = s->limitPlusSlop - GC_HEAP_LIMIT_SLOP;
}

void endAtomic (GC_state s) {
	int lockop, unlockop;

	lockop = pthread_mutex_lock(&lock);
	assert(lockop == 0);
	s->atomicState--;
	unlockop = pthread_mutex_unlock(&lock);
	assert(unlockop == 0);

	if (0 == s->atomicState
			and s->signalsInfo.signalIsPending)
		s->limit = 0;
}
