/*
 * Copyright (c) 1996-1999 The University of Utah and the Flux Group.
 * 
 * This file is part of the OSKit Linux Glue Libraries, which are free
 * software, also known as "open source;" you can redistribute them and/or
 * modify them under the terms of the GNU General Public License (GPL),
 * version 2, as published by the Free Software Foundation (FSF).
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */
/*
 * Linux scheduler support.
 *
 * NOTE: These routines must carefully preserve the
 * caller's interrupt flag.
 */

#include <linux/sched.h>
#include <asm/atomic.h>
#include "osenv.h"

/*
 * Read the discussion in the linux source. Should the spinlock be under SMP?
 */
spinlock_t semaphore_wake_lock = SPIN_LOCK_UNLOCKED;

void
__down(struct semaphore *sem)
{
	struct task_struct *cur = current;
	struct wait_queue wait = { cur, NULL };
	unsigned long flags;

	save_flags(flags);
	cli();

	__add_wait_queue(&sem->wait, &wait);

	while (1) {
		spin_lock(&semaphore_wake_lock);
		if (sem->waking > 0) {
			sem->waking--;
			spin_unlock(&semaphore_wake_lock);
			break;
		}
		spin_unlock(&semaphore_wake_lock);

		osenv_sleep_init(&cur->sleeprec);
		osenv_sleep(&cur->sleeprec);
	}

	__remove_wait_queue(&sem->wait, &wait);

	current = cur;
        restore_flags(flags);
}


void
__up(struct semaphore *sem)
{
	unsigned long flags;

	save_flags(flags);
	cli();
	spin_lock(&semaphore_wake_lock);
	if (atomic_read(&sem->count) <= 0)
		sem->waking++;
	spin_unlock(&semaphore_wake_lock);
        restore_flags(flags);

	wake_up(&sem->wait);
}


static void
__sleep_on(struct wait_queue **q, int interruptible)
{
	struct task_struct *cur = current;
	struct wait_queue wait = { cur, NULL };

	if (!q)
		return;

	osenv_sleep_init(&cur->sleeprec);
	add_wait_queue(q, &wait);

	osenv_sleep(&cur->sleeprec);
	current = cur;

	remove_wait_queue(q, &wait);

	current = cur;
}


void
sleep_on(struct wait_queue **q)
{
	__sleep_on(q, 0);
}


void
interruptible_sleep_on(struct wait_queue **q)
{
	__sleep_on(q, 1);
}


long
interruptible_sleep_on_timeout(struct wait_queue **q, long timeo)
{
	long residual;

	/* XXX implement timeout */
	residual = timeo;

	__sleep_on(q, 1);

	return residual;
}


void
__wake_up(struct wait_queue **q, unsigned int mode)
{
	struct wait_queue *next;
	struct wait_queue *head;

	if (!q || !(next = *q))
		return;
	head = WAIT_QUEUE_HEAD(q);
	while (next != head) {
		struct task_struct *p = next->task;
		next = next->next;
		if (p != NULL) {
			osenv_wakeup(&p->sleeprec, OSENV_SLEEP_WAKEUP);
		}
		if (!next)
			panic("wait_queue is bad");
	}
}


void
__wait_on_buffer(struct buffer_head *bh)
{
	struct task_struct *cur = current;
	struct wait_queue wait = { cur, NULL };
	unsigned flags;

	flags = linux_save_flags_cli();

	__add_wait_queue(&bh->b_wait, &wait);

	while (buffer_locked(bh)) {
		osenv_sleep_init(&cur->sleeprec);
		osenv_sleep(&cur->sleeprec);
	}
	current = cur;

	__remove_wait_queue(&bh->b_wait, &wait);

	linux_restore_flags(flags);

	current = cur;
}


void
unlock_buffer(struct buffer_head *bh)
{
	clear_bit (BH_Lock, &bh->b_state);
	wake_up(&bh->b_wait);
}


/*
 * schedule() should never be called in the oskit since we have
 * no resource contention.
 */
void
schedule()
{
	panic("SCHEDULE CALLED!\n");
}
