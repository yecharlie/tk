#include "tk.h"
#include <cstdio>
#include <cstdlib> // labs, size_t
#include <iostream>


namespace tk {

std::forward_list<task*> task::taskchain; 
int task::new_tasks_cnt = 0;

const char* state_string(sched::statetype s)
{
	switch (s) {
	case sched::IDLE:		return "IDLE";
	case sched::TERMINATED:	return "TERMINATED";
	case sched::RUNNING:		return "RUNNING";
	default:		return 0;
	}
}


task::task(const char* name, std::size_t stacksize) : t_name(name), asked_size(stacksize), cb(), t_alert()
{
	new_tasks_cnt++;
	taskchain.push_front(this);
}


void task::start()
{
	if (this != thistask) task_error(E_ILLSTART, this);
	if (!new_tasks_cnt) return;

	thistask->insert(0, this);
	auto it = taskchain.begin();
	for (;new_tasks_cnt > 0;++it, new_tasks_cnt--)
		(*it)->insert(0, this);

	schedule();
}


task::~task()
{
	//It means that newly created tasks that have not been started are not allowed to be deleted also.
	if (s_state != TERMINATED) task_error(E_TASKDEL, this);

	taskchain.remove(this);
	if (this == thistask) {
		if (new_tasks_cnt > 0) task_error(E_NSTART, this);
		thistask = 0;
		end(0, ctrler::OBSOLETE);
		schedule();
	}
}


void task::eat()
/*
 * eat() will be called only once at the beginning by the main task
 */
{
	// set explicit this pointer & force to be synchronized with thistask
	static task*& th = object::thistask;
	static std::size_t d, acqs;
	ctrler c;
	while (true) {
		//allocate space
		while (!(th->cb && th->cb->state == ctrler::USED)) {
			if (!th->cb) {
				th->cb = &c;
				setjmp_tagged(th->cb->jmpb, J_TORUN);
//				std::printf("%s res=%d jmpb=%x jcode=%d\n", res==0?"setjmp":"longjmp", res, &th->cb->jmpb, J_TORUN);
			}

			if ((acqs = th->cb->acquired_size ) >= th->asked_size &&
					acqs < th->asked_size + DFSS)
				th->cb->state = ctrler::USED;
			else if (acqs < th->asked_size )
				// acqs > 0 is always true
				object::task_error(E_STACKSIZE, th);
			else if ((d = std::labs((char*)&c - (char*)th->cb)) < th->asked_size)
				eat();
			else if (d >= th->asked_size + DFSS)
				object::task_error(E_STACKALLOC, th);
			else {
				// derive c from cb
				c.acquired_size = acqs - d; 
			       	th->cb->acquired_size = d;

				th->cb->tk = th;

				// link c after cb : so that we could be quicker to search a block of fres space which may have been allocated brfore.
				c.insert_after(th->cb);

				if (!setjmp_tagged(c.jmpb, J_TOALLOC)) {
					std::longjmp(th->cb->jmpb, J_TORUN);
				}
			}
		}

		// do my routine
		th->routine();

		if (!setjmp_tagged(th->cb->jmpb, J_NEWTURN)) {

			// nommally terminated
			th->end(0, ctrler::FREE);

			th->schedule();
		}
	} // while(true)
}


void task::restore(task* running)
{
//	std::printf("%s.restore()\n",t_name);
	int res = 0;
	// when task running was terminated, stack check was already performed in end()
	if (running && running->cb && !running->cb->contact()) task_error(E_STACKOVERFLOW, running);

	if (running && running->s_state != sched::TERMINATED) res = setjmp_tagged(running->cb->jmpb, J_SWITCH);
	if (!res) {
		if (cb) std::longjmp(cb->jmpb, J_SWITCH);
		else {
			ctrler* cc = ctrler::ctrler_chain();

			// first fit
			for (; cc; cc=cc->next) {
				if (cc->state == ctrler::FREE && cc->acquired_size >= asked_size)
					/* "polymorphism" of longjmp under setjmp_tagged
					 * J_TOALLOC -> this is a ctrler has never been used
					 * J_NEWTURN -> this is a ctrler has just accomplished a task
					 * */
					std::longjmp(cc->jmpb, J_TOALLOC | J_NEWTURN);
			}
			task_error(E_NOSPACE, this);
		}
	}
}


void task::resume()
{
	task* running = thistask;
	thistask = this;
	restore(running);
}


void task::cancel(int res)
{
	if (s_state != TERMINATED)
		if (this == thistask)
			// guarantee that this task cannot be resumed
			sched::cancel(res);
		else
			// also free the space
			end(res, ctrler::OBSOLETE);
}


void task::end(int res, ctrler::statetype c_stat)
{
	if (s_state != TERMINATED) {
		// set s_state to TERMINATED
		sched::cancel(res);
	}

	if (cb) {
		cb->state = c_stat;
		cb->tk = 0;
		if (!cb->contact()) {
			//lost contact with the sentinel of next ctrler:
			// so we cannot merge
			task_error(E_STACKOVERFLOW, this);
		}

		cb->merge();
		if (cb->prev) cb->prev->merge();
		cb = 0;
	}
}


void task::sleep(object* t)
{
	if (t) t->remember(this);
	if (s_state == RUNNING) remove();
	if (this == thistask) schedule();
}


void task::delay(long d)
{
	insert(d, this);
	if (thistask == this) schedule();
}


void task::wait(object* ob)
{
	if (ob == (object*) this) task_error(E_WAIT, this);
	t_alert = ob;
	while (ob->pending())
		sleep(ob);
}


int task::waitlist(object* a...)
{
	return waitvec(&a);
}


int task::waitvec(object** v)
{
	object* ob;
	bool all_pending = true;
	int i;
	while (all_pending) {
		for (i = 0; ( ob = v[i]  )
			&& ( all_pending = all_pending && ob->pending() );
		       	i ++) ob->remember(this);
		if (all_pending) {
			// wait self
			if (i == 1 && v[0]==(object*)this) task_error(E_WAIT, this);
			sleep();
		}
	}

	t_alert = v[i];
	for (int j = 0;ob = v[j]; j++) ob->forget(this);
	return i;
}

void task::print(int n)
{
	if (n&CHAIN) {
		// print task list
		std::printf("\ttask chain:\n");
		auto it = taskchain.begin();
		for (;it != taskchain.end(); ++ it)
			(*it)->print(n&~CHAIN);

		//print run chain, briefly
		sched::print(CHAIN);
	}else{
		const char* ss= state_string(s_state);
		const char* ns = (t_name) ? t_name : "";
		std::printf("task %s ", ns);
		if (this == thistask)
			std::printf("(is thistask, %s)", ss);
		else if (ss)
			std::printf("(%s) ", ss);
		else
			std::printf("(state=%d CORRUPTED) ", s_state);
		std::printf("\n");
	}
}
}
