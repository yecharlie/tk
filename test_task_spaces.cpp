#include "tk.h"
#include <cstdio>


/*
 * test_task_spaces.cpp
 * Simple example from "Tasks" section of Stroustrup/Shopiro coroutine memo,
 * Release Notes pp 13-4.
 * Shows task basics:  creation, termination, "returning" values, blocking.
 */

class spaces : public task
{
	private:
		const char*s;
		int count;
	public:
		spaces(const char* ss):s(ss), task("spaces", 5000), count() {}
		int rdcount() {return count;}
	private:
		void routine();
};

void spaces::routine()
{
	int i;
	char c;
	while (c = *s++)
		if (c == ' ') i++;
	count = i;
	std::printf("exec spaces routine\n");
}

int main()
{
	spaces sc("a line with four spaces");
	currenttask->start();

//	sc.result();
	int count = sc.rdcount();
	std::printf("count = %d\n", count);
}
