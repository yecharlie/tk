#ifndef TK_COROUTINE_H_
#define TK_COROUTINE_H_


#include<forward_list>
#include <list>
#include<queue>
#include <csetjmp>
#include <cstddef> // size_t
#include <limits>
#include <cstdio>


namespace tk {

class object;
class sched;
class timer;
class task;
class ctrler;
class main_task;

template<typename T>
struct loc_printer;
template<typename T, typename P=loc_printer<T>>
class qhead;
template<typename T, typename P=loc_printer<T>>
class qtail;

/* print flags, used as arguments to class print functions */
const int CHAIN		= 1;
const int VERBOSE	= 2;
const int STACK		= 4; // not implemented yet

/* Pointer of Function with Void parameters*/
typedef void (*PFV) ();

/* error code */
#define task_error_messages			 \
macro_start                                      \
macro(0, E_ERROR, "")                            \
macro(1, E_OLINE, "object::delete(): has chain") \
macro(2, E_SCHOBJ, "sched object used directly (not as base)" ) \
macro(3, E_RESTERM, "sched::insert(): cannot schedule terminated sched")	\
macro(4, E_RESRUN, "sched::schedule(): running")				\
macro(5, E_NEGTIME, "sched::schedule(): clock<0")			\
macro(6, E_SETCLOCK, "sched::setclock(): clock!=0")			\
macro(7, E_RESULT, "task::result(): thistask->result()")			\
macro(8, E_SCHTIME, "sched::schedule(): runchain corrupted: bad time")	\
macro(9, E_TASKDEL, "task::~task(): not terminated")			\
macro(10, E_ILLSTART, "task::start(): not by thistask") 		\
macro(11, E_NSTART, "sched::schedule(): new created task(s) not started") 		\
macro(12, E_CLINE, "ctrler::insert_after(): has been inserted earlier") 		\
macro(13, E_LONGJMP, "task::setjmp_limited(): unmatched Jcode") 		\
macro(14, E_STACKSIZE, "task::eat(): insufficient stack size") 		\
macro(15, E_STACKALLOC, "task::eat(): cannot allocate stack prpperly") 		\
macro(16, E_STACKOVERFLOW, "task::restore(): the stack has corrupted.") 		\
macro(17, E_NOSPACE, "task::restore(): no free stack space") 		\
macro(18, E_QDEL, "queue::~queue(): not empty")				\
macro(19, E_PUTFULL, "qtail::put(): full")					\
macro(20, E_GETEMPTY, "qhead::get(): empty")	\
macro(21, E_BACKFULL,"qhead::putback(): full")				\
macro(22, E_TIMERDEL,"timer::~timer(): not terminated")			\
macro(23, E_WAIT,"task::wait() | waitvec() | waitlist() : wait for self")				\
macro(24,E_CLOCKIDLE,"sched::schedule(): clock_task not idle")	\
macro_end(E_CLOCKIDLE)
#define macro_start
#define macro(num, name, string) const int name = num;
#define macro_end(last_name) const int MAXERR = last_name;
task_error_messages
#undef macro_start
#undef macro
#undef macro_end

/* encapsulate our main function*/
#define main(...) _user_main_routine(int argc, char* argv[])

/* DeFault Stack Size*/
const int DFSS = 10000;

/* setjmp at different circumstance with unique jcode,
 * where later longjmp with the corresponding jcode will arrive,
 * only for better readability */
#define setjmp_tagged(jmpb, jcode) setjmp(jmpb)

/* jump code */
const int J_SETJMP = 0;
const int J_TORUN = 1;
const int J_TOALLOC = 2;
const int J_NEWTURN = 4;
const int J_SWITCH = 8;

class object
{
friend class sched;
friend class task;
friend class main_task;
	private:
		std::forward_list<task*> o_list;
		static task* thistask;
	public:
		virtual ~object();

		void remember(task*);
		void forget(task*);
		void alert();
		virtual int pending();
		virtual void print(int);
		static task* this_task() {return thistask;}
		static int task_error(int, object*);
};

//user interface of current task
#define currenttask tk::object::this_task()

class sched : public object
{
friend class object;
friend class timer;
friend class task;
friend class main_task;
friend void _print_error(int n);
	public:
		enum statetype {IDLE=1, RUNNING=2, TERMINATED=4};
//		enum insertmode {PREEMPT=1, WAIT=2};
		struct schain;
	private:
		long s_time;
//		struct scmp;

//		bool s_onchain; // flog on the run chain

		static long clock;
//		static std::priority_queue<sched*, std::vector<sched*>, scmp> runchain;
		static schain runchain;
		statetype s_state;
	protected:
		sched() : s_time(0), s_state(IDLE) {}
	public:
		static void setclock(long);
		static long getclock() {return clock;}
//		static std::priority_queue<sched*, std::vector<sched*>, scmp> run_chain(); 
		static	task*	clock_task;	// awoken at each clock tick
		static bool clock_task_exit; // if applied, execute clock task before exit_fct
		static std::list<sched*> run_chain(); 
		static PFV exit_fct; // user-supplied exit function

		void restore(task*, int=0);
		long rdtime() {return s_time;}
		statetype rdstate() {return s_state;}
		int pending() {return s_state != TERMINATED;}
		void print(int);
		void cancel(int);
		virtual void setwho(object*);
		void result();
	private:
		void schedule();
		virtual void resume();
//		void insert(long, object*, insertmode=WAIT);
		void insert(long, object*);
		void remove();
};

//struct sched::scmp {
//	bool operator()(sched*& s1, sched*& s2) {return s1->rdtime() < s2->rdtime();}
//};
//
//struct sched::schain : public std::priority_queue<sched*, std::vector<sched*>, sched::scmp>
//{
//	bool remove(const sched*);
//	bool find(const sched*);
//	std::list<sched*> tolist();
//};

struct sched::schain : public std::list<sched*>
{
//	void push(sched* const, insertmode);
	void push(sched* const);
	void pop() {pop_front();}
	sched* top() const {return front();}
};

class timer : public sched {
		void resume();
	public:
		timer(long);
		~timer();
		void reset(long);
		void setwho(object*) {} // do nothing
		void print(int);
};

class ctrler 
{// control block
friend class task;
	public:
		enum statetype {FREE=1, USED=2, OBSOLETE=4};
	private:
		static const int SEN_RANGE = 25;
		static const int UNTOUCHED = 123123;
		static ctrler* ctrlerchain;
		typedef int SENTINEL[SEN_RANGE];
		typedef SENTINEL ctrler::*SP;
		static SP sp;

		SENTINEL sentinel;
		task* tk;
		std::jmp_buf jmpb;
		statetype state;
		std::size_t acquired_size;
		ctrler* prev, *next;
	public:
		ctrler() : state(FREE), tk(), prev(), next(), acquired_size(std::numeric_limits<std::size_t>::max()){}
//		void set_acquired_size(int sz) {acquired_size=sz;}
		void insert_after(ctrler*);
		void erase();	
		
		bool merge(); // merge with the right adjascent ctrler if allowed
		inline bool contact();
		void print(int);
		static ctrler* ctrler_chain() {return ctrlerchain;}
};


bool ctrler::contact()
{
	for (int i=0; i<SEN_RANGE; i++)
		if ((next->*sp)[i] != UNTOUCHED) {
			return false;
		}
	return true;
}


class task : public sched
{
friend class sched;
friend class main_task;
	private:
	       static std::forward_list<task*> taskchain;
	       static int new_tasks_cnt;
	       ctrler* cb;	
	       object*	t_alert;	/* object that inserted you */
	       std::size_t asked_size;
	protected:
	       task(const char* name=0, std::size_t stacksize=DFSS);
	public:
	       const char* t_name;
	       static std::forward_list<task*> task_chain() {return taskchain;} 

	       void start();
	       virtual ~task();
	       void wait(object*);
	       int waitlist(object* ...);
	       int waitvec(object**);
	       void delay(long);
	       void sleep(object* t = 0);
	       void cancel(int);
	       void setwho(object* o) {t_alert = o;}
	       void print(int);
	       object* who_alerted_me() {return t_alert;}
	private:
	       static void eat();
	       virtual void routine() = 0;
	       void end(int, ctrler::statetype);
	       void resume();
	       void restore(task*);
};

enum qmodetype { EMODE, WMODE, ZMODE };

template<typename T>
struct loc_printer<T*>
{
	static void print(T* t) {std::printf("@%x, ", t);}
};

template<typename T, typename P>
struct _queue
{// a queue with logical max capacity
	std::list<T>* q_list;
	int q_max;
	qhead<T, P>* q_head;
	qtail<T, P>* q_tail;
	_queue(int m):q_max(m),q_head(),q_tail() {q_list = new std::list<T>();}
	~_queue() {q_list->empty()?0:object::task_error(E_QDEL, 0); delete q_list;}
	void print(int);
};

template<typename T, typename P>
class qhead : private object
{
friend class qtail<T, P>;
	private:
		qmodetype qh_mode;
		_queue<T, P>* qh_queue;
	public:
		qhead(qmodetype = WMODE, int = 10000);
		~qhead();

		T get();
		int putback(T);

		int rdcount() {return ( qh_queue->q_list )->size();}
		int rdmax() {return qh_queue->q_max;}
		qmodetype rdmode() {return qh_mode;}

		qhead<T,P>* cut();
		void splice(qtail<T,P>*);

		qtail<T,P>* tail(); 

		void setmode(qmodetype m) {qh_mode=m;}
		void setmax(int m) {qh_queue->q_max = m;}
		int pending() {return rdcount() == 0;}

		void print(int);
};

//#include "qhead.hxx"
template<typename T, typename P>
qhead<T,P>::qhead(qmodetype mode, int max)
{
	if (max > 0) {
		qh_queue = new _queue<T,P>(max);
		qh_queue->q_head = this;
	}
	qh_mode = mode;
}

template<typename T, typename P>
qhead<T,P>::~qhead()
{
	_queue<T,P>* q = qh_queue;
	if (q->q_tail)
		q->q_head = 0;
	else
		delete q;
}


template<typename T, typename P>
T qhead<T,P>::get()
{
	for (;;) {
		_queue<T,P>* q = qh_queue;
		if (rdcount()) {
			T r = q->q_list->front();
			q->q_list->pop_front();
//			std::printf("get(): size=%d, q_max=%d, q_tail=%x\n", q->q_list->size(), q->q_max, q->q_tail);
			if (q->q_list->size() == q->q_max - 1 && q->q_tail) q->q_tail->alert(); 
			return r;
		}

		switch (qh_mode) {
			case WMODE:
				this_task()->sleep(this);
				break;
			case EMODE:
				task_error(E_GETEMPTY, this);
				break;
			case ZMODE:
				return 0;
		}
	}

}


template<typename T, typename P>
int qhead<T, P>::putback(T p)
{
	for (;;) {
		_queue<T,P>* q = qh_queue;
		if (rdcount() < q->q_max) {
			 q->q_list->push_front(p);
			 return 1;
		}

		switch (qh_mode) {
			case WMODE:
			case EMODE:
				task_error(E_BACKFULL, this);
			case ZMODE:
				return 0;
		}
	}
}


template<typename T, typename P>
qtail<T,P>* qhead<T, P>::tail()
{
	_queue<T,P>* q = qh_queue;
	qtail<T,P>* t = q->q_tail;
	if (!t) {
		t = new qtail<T,P>(qh_mode, 0); // put 0 to avoid reacllocate internal _queue
		q->q_tail = t;
		t->qt_queue = q;
	}
	return t;
}


/* make room for a filter upstream from this qhead */
/* result:  (this)qhead<-->newq    (new)qhead<-->oldq ?<-->qtail? */
template<typename T, typename P>
qhead<T,P>* qhead<T,P>::cut()
{
	_queue<T,P>* oldq = qh_queue;
	qhead* h = new qhead(qh_mode, oldq->q_max);
	_queue<T,P>* newq = h->qh_queue;

	oldq->q_head = h;
	h->qh_queue = oldq;

	qh_queue = newq;
	newq->q_head = this;

	return h;
}

// debug
//class O : public object {	// sequentially numbered objects
//	static int Ocount;
//	int id;
//public:
//			O()		{ id = ++Ocount; }
//	int		get_id()	{ return id; }
//	static int	get_Ocount()	{ return Ocount; }
//};

/* this qhead is supposed to be upstream to the qtail t
   add the contents of this's queue to t's queue
   destroy this, t, and this's queue
   alert the spliced qhead and qtail if a significant state change happened
*/

template<typename T, typename P>
void qhead<T,P>::splice(qtail<T,P>* t)
{
	_queue<T,P>* qt = t->qt_queue;
	_queue<T,P>* qh = qh_queue;

	int qtcount = qt->q_list->size();
	int qhcount = qh->q_list->size();
	int halert = (!qtcount && qhcount); //qt becomes non-empty. Note that after splice() qt and qh are merged to one entity.
	int talert = (qh->q_max <= qhcount && qhcount+qtcount < qt->q_max); // qh becomes non-full

//	std::printf("splice1(): ");
//	auto it = qt->q_list->begin();
//	for (;it != qt->q_list->end(); it++) std::printf("\t O %d",((O*) (*it) )->get_id());
//	std::printf("\n");
	if (qhcount) {
		qt->q_list->splice(qt->q_list->end(), *( qh->q_list ));
	}

//	std::printf("splice2(): ");
//	it = qt->q_list->begin();
//	for (;it != qt->q_list->end(); it++) std::printf("\t O %d",((O*) (*it) )->get_id());
//	std::printf("\n");

	qh->q_tail->qt_queue = qt;
	delete t;
	qt->q_tail = qh->q_tail;

	qh->q_tail = 0;
	delete this;

	if (halert) qt->q_head->alert();
	if (talert) qt->q_tail->alert();
}


template<typename T, typename P>
void qhead<T,P>::print(int n)
{
	int m = qh_queue->q_max;
	int c = qh_queue->q_list->size();
	_queue<T,P>* q = qh_queue;

	printf("qhead mode=%d, max=%d, count=%d, tail=%x\n",
		qh_mode, n, c, q->q_tail);

	if (n&VERBOSE) {
		int m = n & ~(CHAIN|VERBOSE);
		if (q->q_tail) {
			printf("\ttail of queue:\n");
			q->q_tail->print(m);
		} else printf("\tno tail\n");
		q->print(m);
	}

}

template<typename T, typename P>
void _queue<T, P>::print(int n)
{
	if (q_list->empty()) return;

	std::printf("\tobject on queue:\n[");
	auto it = q_list->begin();
	for (;it!=q_list->end(); it++)
		P::print(*it);
	std::printf("]\n");
}
template<typename T, typename P>
class qtail : public object
{
friend class qhead<T,P>;
	private:
		qmodetype qt_mode;
		_queue<T, P>* qt_queue;
	public:
		qtail(qmodetype = WMODE, int = 10000);
		~qtail();

		int put(T);

		int rdspace() {return qt_queue->q_max - ( qt_queue->q_list )->size();}
		int rdmax() {return qt_queue->q_max;}
		qmodetype rdmode() {return qt_mode;}

		qtail<T,P>* cut();
		void splice(qhead<T,P>*);

		qhead<T,P>* head();

		void setmode(qmodetype m) {qt_mode = m;}
		void setmax(int m) {qt_queue->q_max = m;}
		int pending() {return rdspace() == 0;}

		void print(int);
};

//#include "qtail.hxx"
template<typename T, typename P>
qtail<T,P>::qtail(qmodetype mode, int max)
{
	if (max > 0) {
		qt_queue = new _queue<T,P>(max);
		qt_queue->q_tail = this;
	}
	qt_mode = mode;
}

template<typename T, typename P>
qtail<T,P>::~qtail()
{
	_queue<T,P>* q = qt_queue;

	if (q->q_head)
		q->q_tail = 0;
	else
		delete q;
}


template<typename T, typename P>
qhead<T,P>* qtail<T,P>::head()
{
	_queue<T,P>* q = qt_queue;
	qhead<T,P>* h = q->q_head;

	if (!h) {
		h = new qhead<T,P>(qt_mode, 0);
		q->q_head = h;
		h->qh_queue = q;
	};

	return h;
}

template<typename T, typename P>
int qtail<T,P>::put(T p)
{
	for (;;) {
		_queue<T,P>* q = qt_queue;
		if (rdspace()) {
			 q->q_list->push_back(p);
			 if (q->q_list->size() == 1 && q->q_head) q->q_head->alert(); 
			 return 1;
		}

		switch (qt_mode) {
			case WMODE:
				this_task()->sleep(this);
				break;
			case EMODE:
				task_error(E_PUTFULL, this);
				break;
			case ZMODE:
				return 0;
		}
	}
}

template<typename T, typename P>
qtail<T,P>* qtail<T,P>::cut()
{
	_queue<T,P>* oldq = qt_queue;
	qtail<T,P>* t = new qtail(qt_mode, oldq->q_max);
	_queue<T,P>* newq = t->qt_queue;

	t->qt_queue = oldq;
	oldq->q_tail = t;

	newq->q_tail = this;
	qt_queue = newq;

	return t;
}

template<typename T, typename P>
void qtail<T,P>::splice(qhead<T,P>* h)
{
	h->splice(this);
}

template<typename T, typename P>
void qtail<T,P>::print(int n)
{
	int m = qt_queue->q_max;
	int c = qt_queue->q_list->size();
	qhead<T,P>* h = qt_queue->q_head;

	std::printf("qtail mode=%d, max=%d, space=%d. head=%x\n", qt_mode, m, m-c, h);

	if (n&VERBOSE) {
		int m = n & ~(CHAIN|VERBOSE);
		if (h) {
			std::printf("\thead of queue:\n");
			h->print(m);
		} else std::printf("\tno head\n");
		qt_queue->print(m);
	}
}

}
#endif // _TK_H_
