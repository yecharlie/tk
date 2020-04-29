#include "tk.h"
#include <vector>
#include <string>

#undef main
extern int _user_main_routine(int argc, char* argv[]);

namespace tk {

class main_task : public task
{
	private:
		int argc;
		std::vector<char*> argv;
	public:
		main_task(int ac, char* av[]);
	private:
		void routine() {_user_main_routine(argc, argv.data());}
};

main_task::main_task(int ac, char* av[]):task("main"), argc(ac), argv(av, av+ac)
{
	thistask = this;
	s_time = 0;
	s_state = sched::RUNNING;
	new_tasks_cnt = 0;

	eat();
}

} // namespace tk

int main(int argc, char* argv[])
{
	/* strange! main_task run without being launched by start().
	 * rather, it forms a base for the start() and other fucilities of tk system.*/
	tk::main_task(argc, argv);
}
