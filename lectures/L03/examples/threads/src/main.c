/*
 * SPDX-License-Identifier: MIT
 *
 * Who runs when: cooperative threads, preemption, and k_yield(), as a trace.
 */

#include <zephyr/kernel.h>

/* Print a line prefixed with the name and priority of the thread that prints it. */
static void say(const char *what)
{
	k_tid_t self = k_current_get();

	printk("%-6s (%3d)  %s\n", k_thread_name_get(self), k_thread_priority_get(self), what);
}

/*
 * One semaphore per thread. A thread waiting on its semaphore is blocked, so the scheduler ignores
 * it; giving the semaphore makes it ready. Which thread then runs is up to the scheduler, and that
 * is what the trace shows.
 */
static K_SEM_DEFINE(coop_a_sem, 0, 1);
static K_SEM_DEFINE(coop_b_sem, 0, 1);
static K_SEM_DEFINE(low_sem, 0, 1);
static K_SEM_DEFINE(high_sem, 0, 1);
static K_SEM_DEFINE(peer_sem, 0, 1);

/*
 * Act 1: two cooperative threads. A negative priority makes a thread cooperative: once it runs, no
 * other thread gets the CPU until it blocks, sleeps or yields, not even one of higher priority.
 */
static void coop_a_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&coop_a_sem, K_FOREVER);
	say("running");
	k_sem_give(&coop_b_sem);
	say("gave coop_b's semaphore: coop_b has a higher priority, but I keep the CPU");
	k_yield();
	say("back again, after coop_b returned");
}

static void coop_b_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&coop_b_sem, K_FOREVER);
	say("running, because coop_a yielded");
}

/*
 * Acts 2 and 3: preemptible threads. A preemptible thread loses the CPU the moment a thread of
 * higher priority becomes ready. A thread of the same priority has to wait until it is offered the
 * CPU, and k_yield() is how a thread offers it.
 */
static void low_entry(void *p1, void *p2, void *p3)
{
	/* Act 2: preemption. */
	k_sem_take(&low_sem, K_FOREVER);
	say("running");
	say("giving high's semaphore");
	k_sem_give(&high_sem);
	say("back again: high preempted me inside k_sem_give()");

	/* Act 3: k_yield(). */
	k_sem_take(&low_sem, K_FOREVER);
	say("running");
	k_sem_give(&peer_sem);
	say("gave peer's semaphore: peer has my priority, so it waits");
	k_yield();
	say("back again, after peer returned");
	k_yield();
	say("yield returned at once: main is ready, but has a lower priority");
}

static void high_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&high_sem, K_FOREVER);
	say("running, in the middle of low's k_sem_give()");
}

static void peer_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&peer_sem, K_FOREVER);
	say("running, because low yielded");
}

/*
 * The threads are created and started at boot, and each blocks on its semaphore at once. The name
 * of each thread is the name given here, and the stack size is what a small thread needs on a real
 * target: on native_sim, the real stack is the host thread's.
 */
K_THREAD_DEFINE(coop_a, 1024, coop_a_entry, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(coop_b, 1024, coop_b_entry, NULL, NULL, NULL, -2, 0, 0);
K_THREAD_DEFINE(low, 1024, low_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(high, 1024, high_entry, NULL, NULL, NULL, 2, 0, 0);
K_THREAD_DEFINE(peer, 1024, peer_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
	printk("-- Act 1: cooperative threads\n");
	say("waking coop_a");
	k_sem_give(&coop_a_sem);

	printk("-- Act 2: preemption\n");
	say("waking low");
	k_sem_give(&low_sem);

	printk("-- Act 3: k_yield()\n");
	say("waking low");
	k_sem_give(&low_sem);

	say("done: every other thread has returned");
	return 0;
}
