#include <cstdio>

#include "tk.h"


/* modified example from splice.c, <task> Release 3.0.3
 * Tasking program to test fix to bug in qhead::splice that occurred
 * when a splice occurred to a queue on which another task was blocked.
 *
 * Main sets up 2 queues:  Q1h-Q1t and Q2h-Q2t.  Task T A gets from
 * Q1h and puts to Q2t.  It attempts to put QMAX+1 objects on the Q2t,
 * and blocks.  Task TT B puts QMAX objects on Q1t, then splices the
 * 2 queues, forming one with Q1h as the head and Q2t as the tail.
 * Task TT B gets MAX+1 objects from Q1h, so as to alert Task A that
 * its Q2t->put can complete, and then completes.  Task A runs again,
 * and gets the rest of the objects off the queue and quits.
 *
 * EXPECTED OUTPUT:
 * Output should show Objects 1-11 being put and gotten from the queue
 * (they won't necessarily be in sequence, but each put should have a matching
 * get) and main, B, and A should all announce that they're done.
 */

const int QMAX = 5;

class O {	// sequentially numbered objects
	static int Ocount;
	int id;
public:
			O()		{ id = ++Ocount; }
	int		get_id()	{ return id; }
	static int	get_Ocount()	{ return Ocount; }
};

int O::Ocount;

class T : public task {
public:
	qtail<O*>* t;
	qhead<O*>* h;
        T(const char* n, qtail<O*>* t, qhead<O*>* h) : task(n, 5000), t(t), h(h) {}
	void routine();
};

void T::routine()
{
	O *Op;
	std::printf("new T(%s)\n", t_name);
	for (int i = 0; i <= QMAX; i++) {
				// put more Os than Qmax to make T block
		Op = new O;
		t->put(Op);
		std::printf("T %s:  put O %d\n", t_name, Op->get_id());
	}
	while ( h->rdcount() > 0 ) {
		Op = (O*) h->get();
		std::printf("T %s:  got O %d\n", t_name, Op->get_id());
	}

	std::printf("T %s: done.\n", t_name);
}

class TT : public task {
public:
	qtail<O*>* t;
	qhead<O*>* h;
        TT(const char* n, qtail<O*>* t, qhead<O*>* h) : task(n, 5000), t(t), h(h) {}
	void routine();
};

void TT::routine()
{
	const char* n = t_name;
	std::printf("new TT(%s)\n",n);
	O *Op;
	for (int i = 0; i < QMAX; i++) {	// don't overflow the queue
		Op = new O;
		t->put(Op);
		std::printf("TT %s:  put O %d\n", n, Op->get_id());
	}
	qhead<O*> *h2 = t->head();	// h is about to be deleted in the splice
	h->splice(t);	// only one queue now (h is deleted)

	h2->print(VERBOSE);
	while(h2->rdcount() >= QMAX) {
		Op = (O*) h2->get();	
		std::printf("TT %s:  got O %d\n", n, Op->get_id());
	}

	std::printf("TT %s: done.\n", n);
}

main()
{
	qhead<O*> *Q1h = new qhead<O*>(WMODE, QMAX);
	qtail<O*> *Q1t = Q1h->tail();
	qhead<O*> *Q2h = new qhead<O*>(WMODE, QMAX);
	qtail<O*> *Q2t = Q2h->tail();

	// qhead with non-pointer T could not call print() if it was not also supplied parameter P to define how to print T.    
//	qhead<object> *Q3h; // ok
//	Q3h->print(VERBOSE); // compailation error

	std::printf("Q1h=%x, Q1t=%x\nQ2h=%x,Q2t=%x\n",Q1h, Q1t, Q2h, Q2t);
	T *TA = new T("A", Q2t, Q1h);
	TT *TB = new TT("B", Q1t, Q2h);

	currenttask->start();
	std::printf("main: exit\n");
}
