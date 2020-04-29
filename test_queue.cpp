#include <cstdio>
#include "tk.h"


/* modified example from public.c, <task> Release 3.0.3
 * trivial task example:
        Make a set of tasks which pass an object round between themselves.
	Each task gets an object from the head of one queue, and puts
	the object on the tail of another queue.
	Main creates each task, then puts the object on a queue.
	Each task quits after getting the object MAX_CYCLES times.
*/

using namespace tk;


const int NTASKS = 1;
const int MAX_CYCLES = 2;


struct pc : task {				// derive a class from task
	qtail<object*>* qt;
	qhead<object*>* qh;
	pc( const char* n, qtail<object*>* t, qhead<object*>* h) :task(n, 5000), qt(t), qh(h) {}	
	void routine();
};						// be used directly


void pc::routine()
{
	std::printf("new pc(%s)\n",t_name);

        for (int i = 0; i < MAX_CYCLES; i++) {
                object* p = qh->get();
                printf("task %s\n", t_name);
                qt->put(p);
        }
	printf("task %s: done.\n", t_name);
}


main()
{
        qhead<object*>* hh = new qhead<object*>;
        qtail<object*>* t = hh->tail();		// hh and t refer to same queue.
        qhead<object*>* h;

	std::printf("main\n");

        for (int i=0; i<NTASKS; i++) {
                char* n = new char[2];	// make a one letter task name
                n[0] = 'a'+i;
                n[1] = 0;

                h = new qhead<object*>;
                new pc(n,t,h);
                printf("main()'s loop\n");
                t = h->tail();
        }


        new pc("last pc",t,hh);	// create another new task
	currenttask->start();  		// start all newly created tasks, which will get stock immediately
	std::printf("main: here we go\n");
        t->put(new object);		// put the object on a queue
	std::printf("main: exit\n");
}
