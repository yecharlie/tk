#include <algorithm>
#include <cstdio>
#include "tk.h"


namespace tk {

long sched::clock = 0;
sched::schain sched::runchain;
task* sched::clock_task = 0;
bool sched::clock_task_exit = false;
PFV sched::exit_fct = 0;

void sched::schain::push(sched* const s)
{
	auto it = begin();
	for (;it != end() && (*it)->rdtime() < s->rdtime(); it++);
	insert(it, s);
}


void sched::setclock(long t)
{
	if (clock) object::task_error(E_SETCLOCK, (object*)0);
	clock = t;
}

std::list<sched*> sched::run_chain()
{
//	return runchain.tolist();
//	return std::list<sched*> (runchain);
	return runchain;
}

void sched::cancel(int res)
{
	if (s_state == RUNNING) remove();
	s_state = TERMINATED;
	s_time = res;
	alert();
}

void sched::result()
{
	if (this == (sched*)this_task()) task_error(E_RESULT, this);
	while (s_state != TERMINATED) {
		this_task()->sleep(this);
	}
}

void sched::schedule()
{
//	std::printf("\tschedule()\n");
//	sched::print(CHAIN);
//	std::printf("\tctrler chain:\n");
//	if (ctrler::ctrler_chain()) ctrler::ctrler_chain()->print(CHAIN);

	if (task::new_tasks_cnt) task_error(E_NSTART, this);

	sched* p = 0;
	long tt;
	if (!runchain.empty()) {
		p = runchain.top();
		runchain.pop();
	} else if (clock_task_exit && clock_task) {
		if (clock_task->s_state != IDLE)
			task_error(E_CLOCKIDLE, this);
		p = clock_task;
		clock_task->s_time = clock; // to skip the if(tt != clock) block below
		clock_task_exit = false;
	}else {// exit
		if (exit_fct) { // do not enter exit_fct twice
			PFV ef = exit_fct;
			exit_fct = 0;
			(*ef)();
		} 		
		exit(0);
	}

	tt = p->s_time;
	if (tt != clock) {
		if (tt < clock) task_error(E_SCHTIME, this);
		clock = tt;
		if (clock_task) {
			if (clock_task->s_state != IDLE)
				task_error(E_CLOCKIDLE, this);
			/* clock_task preferred */
			runchain.push(p);
			p = clock_task;
		}
	}
	if (p != this)
		p->resume();
}

void sched::insert(long d, object* who)
{
	switch(s_state) {
		case TERMINATED:
			task_error(E_RESTERM, this);
			break;
		case IDLE:
			break;
		case RUNNING:
			if (this != (class sched *)this_task()) task_error(E_RESRUN, this);
	}

	if (d<0) task_error(E_NEGTIME, this);

	if (std::find(runchain.begin(), runchain.end(), this) != runchain.end()) task_error(E_RESRUN, this);

	s_time = clock + d;
	s_state = RUNNING;
//	s_onchain = true;
	setwho(who);
	runchain.push(this);
}

void sched::remove()
{
	runchain.remove(this);
	s_state = IDLE;
}

void sched::print(int n)
{
	if (n&CHAIN) {
		std::printf("\trun chain:\n");
		std::list<sched*> l = run_chain();
		auto it = l.begin();
		for (;it != l.end(); ++it)
			(*it)->print(n & ~CHAIN);
	}else {
		std::printf("\n");
	}
}

/* sched::resume() is a virtual function.  Because sched is not intended
 * to be used directly, but only as a base class, this should never be called.
 * Must define resume for each class derived from sched.
 */
void
sched::resume()
{
	task_error(E_SCHOBJ, this);
}

/* sched::setwho() is a virtual function.  Because sched is not intended
 * to be used directly, but only as a base class, this should never be called.
 * Must define setwho for each class derived from sched.
 */
void
sched::setwho(object*)
{
	task_error(E_SCHOBJ, this);
}

}
