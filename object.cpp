#include <iterator>
#include <cstdio>

#include "tk.h" 

namespace tk {
task* object::thistask = 0;

object::~object()
{
	std::forward_list<task*> :: iterator it;
	for (it = o_list.begin(); it != o_list.end(); ++it)
		if ((*it)->s_state != sched::TERMINATED) {
			task_error(E_OLINE, this);
			break;
		}
}

void object::remember(task* p)
{
	if (p->s_state != sched::TERMINATED)
		o_list.push_front(p);
}

void object::forget(task* p)
{
	//remove all
	o_list.remove(p);
}

void object::alert()
{
	std::forward_list<task*> :: iterator it;
	for(it = o_list.begin(); it != o_list.end(); ++it)
		if((*it)->s_state == sched::IDLE)
			(*it)->insert(0, this);
	//flush
	o_list.clear();
}

/*
 * virtual int object::pending() returns 1 if the object should be waited for.
 * By default say yes and assume that alert() will be called somehow.
 */
int object::pending()
{
	return 1;
}

void object::print(int n)
{
	if(n&VERBOSE) {
		std::printf("object: this=%x\n", this);
		if(o_list.empty())
			std::printf("\tNo tasks remembered\n");
		std::forward_list<task*>::iterator it;
		for(it = o_list.begin(); it != o_list.end(); ++it) {
			std::printf("\tNext task in this %x remember chain is:\n", this);
			(*it)->print(n & ~CHAIN & ~VERBOSE);
		}
	} else {
		std::printf("\n");
	}

}

#define macro_start static const char* error_name[] = {
#define macro(num, name, string) string,
#define macro_end(last_name) };
task_error_messages
#undef macro_start
#undef macro
#undef macro_end

void _print_error(int n)
{
	int i = (n<1 || n>MAXERR) ? 0 : n;
	std::printf("\n\n**** task_error(%d) %s\n",n, error_name[i]);

	if (object::this_task()) {
		std::printf("thistask: ");
		object::this_task()->print(VERBOSE|STACK);
	}
	if (!sched::run_chain().empty()) {
		sched::runchain.top()->sched::print(CHAIN);
	}
}

int object::task_error(int n, object* th)
{
	_print_error(n);
	exit(n);
}
}
