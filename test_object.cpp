#include <cstdio>
#include "tk.h"

void _print_list(std::list<sched*>& l)
{
	std::printf("sched list, size=%d\n", l.size());

	auto it = l.begin();
	for (;it != l.end(); it++) (*it)->print(0);
}

struct ttask : public task
{
	ttask(const char* n) : task(n) {};
};


int main()
{
	object o1;
	task* p1 = new ttask("t1");
	task* p2 = new ttask("t2");
	o1.remember(p1);
	o1.remember(p2);
	o1.print(VERBOSE);
	o1.forget(p1);
	o1.print(VERBOSE);
	o1.alert();
	o1.print(VERBOSE);
	std::list<sched*> l = sched::run_chain();
	_print_list(l);
}
