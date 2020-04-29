#include "tk.h"
#include <cstdio>

class simple : public task
{
	public:
		simple():task("simple", 5000) {}
	private:
		void routine();
};

void simple::routine()
{
	std::printf("delete before\n");
	cancel(0);
	delete this;
	std::printf("delete after (not printed)\n");
}

int main()
{
	std::printf("main begin\n");
	simple s;
	currenttask->start();
	std::printf("main end\n");
}
