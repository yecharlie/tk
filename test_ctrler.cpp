#include <cstdio>
#include "tk.h"

int main()
{
	ctrler* c1 = new ctrler();
	c1->set_acquired_size(100);
	ctrler* c2 = new ctrler();
	c2->set_acquired_size(200);
	ctrler* c3 = new ctrler();
//	c3->set_acquired_size(300);
//	c1->insert_after(0);
	c2->insert_after(c1);
	c3->insert_after(c2);

	std::printf("ctrler_chain:\n");
	ctrler::ctrler_chain()->print(CHAIN);

	c1->merge();
	std::printf("ctrler_chain:\n");
	ctrler::ctrler_chain()->print(CHAIN);
}
