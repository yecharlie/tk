#include "tk.h"


namespace tk {

timer::timer(long d)
{
	s_state = IDLE;
	insert(d, this);
}


timer::~timer()
{
	if (s_state != TERMINATED) task_error(E_TIMERDEL, this);
}


void timer::reset(long d)
{
	remove();
	insert(d, this);
}


void timer::resume() // time is up; "delete" timer & schedule next task 
{
	s_state = TERMINATED;
	alert();
	schedule();
}


void timer::print(int n)
{
	std::printf("timer %ld == clock+%ld\n", s_time, s_time - getclock());
}

} // namespace tk
