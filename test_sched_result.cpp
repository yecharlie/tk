#include "tk.h"
#include <cstdio>


/* TEST: sched.result() & task.sleep() & task.delay() */


class simple : public task
{
	public:
		simple():task("simple", 5000) {}
	private:
		void routine();
};


void simple::routine()
{
	std::printf("simple: the first time entering\n");
	delay(1);
	std::printf("simple: the second time entering\n");
}

int main()
{

	std::printf("main: start\n");
	simple s;
	currenttask->start();

	//waiting for s
	s.result();

	std::printf("main: end\n");
}
