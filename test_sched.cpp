#include <cstdio>
#include "tk.h"

struct ttask : public task
{
	ttask(const char* n) : task(n) {};
};

void _print_list(sched::schain& l)
{
	std::printf("sched list, size=%d\n", l.size());

	auto it = l.begin();
	for (;it != l.end(); it++) (*it)->print(0);
}

int main()
{
	// test schain
	task* p1 = new ttask("t1");
	task* p2 = new ttask("t2");
	task* p3 = new ttask("t3");
	sched::schain sc;
	sc.push(p1);
	sc.push(p2);
	sc.push(p3);
	_print_list(sc);
	sc.remove((sched*)p2);
	_print_list(sc);

}
