#include "tk.h"

#include <cstdlib> // rand()
#include <vector>
#include <sstream>
#include <fstream>

class customer;
class producer;

const int MIN_REQ_SEEDS = 300;


struct seeds : public tk::object 
{
	int count;
	int pending() {return count == 0;}
	void put(int n) {if ((count += n) == n) alert();}
	int take() {
		if (!count) return 0;
		else {count--; return 1;}
	}
};


class producer : public tk::task
{
friend class logger;
	private:
		static int allprd;
		int nprd;
		seeds* sp;
		void routine();
	public:
		static std::vector<producer*> pvec;
		producer(const char* n, seeds* ss) : sp(ss), task(n)  {pvec.push_back(this);}
		~producer() {if (pvec.empty()) delete sp;}
};

int producer::allprd = 0;
std::vector<producer*> producer::pvec;

class customer : public tk::task
{
friend class logger;
	private:
		int ncus;
		seeds* sp;
		int modest(); 
		void acknowledge() {alert();}
		void routine();
	public:
		static std::vector<customer*> cvec;
		customer(const char* n, int interv, seeds* ss) : task(n), interval(interv),sp(ss) {cvec.push_back(this);}
		~customer() {sp->forget(this);}

		int interval;
};
std::vector<customer*> customer::cvec;


class logger : public tk::task
{
		std::list<std::string>*  data;
		seeds* sp;
		static std::string title(std::vector<producer*>& pvec, std::vector<customer*>& vec);
		void routine();
	public:
		logger(seeds* ss, int sz) : task("logger", sz), data(new std::list<std::string>), sp(ss){}
		void flush(const char*);
};

std::string logger::title(std::vector<producer*>& pvec, std::vector<customer*>& cvec)
{
	producer* pp;
	customer* cp;
	std::ostringstream oss;
	oss << "Time" << ",";
	for (int i = 0; i < pvec.size() && (pp=pvec[i]); i++) oss << pp->t_name << ",";
	for (int j = 0; j < cvec.size() && (cp=cvec[j]); j++) oss << cp->t_name << ",";
	oss << "Seeds";
	return oss.str();
}

void logger::flush(const char* n) 
{
	if (data) {
		std::ofstream out(n);
		auto it = data->begin();
		for (;it != data->end(); it++)
			out << *it << std::endl;
		delete data;
		data = 0;
	}
}


void logger::routine()
{
	std::vector<producer*>& pvec = producer::pvec;
	std::vector<customer*>& cvec = customer::cvec;
	std::ostringstream oss;
	data->push_back(title(pvec, cvec));

	producer* pp;
	customer* cp;
	while (true) {
		std::ostringstream oss;
		oss << getclock() << ",";
		for (int i = 0; i < pvec.size() && (pp=pvec[i]); i++) oss << pp->nprd << ",";
		for (int j = 0; j < cvec.size() && (cp=cvec[j]); j++) oss << cp->ncus << ",";
		oss << sp->count;
		data->push_back(oss.str());

		sleep();
	}
}


int customer::modest()
{
	customer* cp;

	for (int i = 0; i<cvec.size() && ( cp = cvec[i] ); i++)
		if (cp != this 
			&& cp
			&& cp->rdstate() == tk::sched::RUNNING
			&& cp->rdtime() == rdtime()
			&& interval < cp->interval) {
			// let cp reminder this later
			sleep(cp);
			return 1;
		}
	return 0;
}


void customer::routine()
{
	while (true) {
		do {
			wait(sp);
		} while (modest());

		ncus += sp->take();
		acknowledge();
		std::printf("%3d | %s consumes %d seed at %ld\n", sp->count, t_name, 1, getclock());

		delay(interval);
	}
}


void producer::routine()
{
	while (allprd < MIN_REQ_SEEDS) {
		// assume it takes the producer #interval time  to produce #cnt amount of seeds 
		int interval = std::rand() % 4 + 2; // [2, 5]

		// cnt is propotional with interval
		int cnt;
	        if (interval <= 2)
			cnt = 1;
		else if (interval <= 4)
			cnt = 2;
		else
			cnt = 3;
		delay(interval);

		sp->put(cnt);
		nprd += cnt;
		allprd += cnt;
		std::printf("%3d | %s produces %d seeds at %ld\n", sp->count, t_name, cnt, getclock());
	}
}


void exit() 
{
	// save log
	logger* lg =  ( logger* ) tk::sched::clock_task;
	lg->flush("./test_task_leisure_time.out");

	// the folowing operations are optional

	customer* cp;
	std::vector<customer*>& cvec = customer::cvec;
	while (!cvec.empty()) {
		cp = cvec.back();
		cp->cancel(0); // IDLE->TERMINATED, required before one could delete it
		cvec.pop_back();
		delete cp;
	}

	producer* pp;
	std::vector<producer*>& pvec = producer::pvec;
	while (!pvec.empty()) {
		pp = pvec.back();
		pvec.pop_back();
		delete pp;
	}

	// delete logger task (current task) 
	lg->cancel(0);
	delete lg;
}


int main()
{
	seeds* sd = new seeds();

	// the order in which the lg (along with other tasks) is created doesnot matter.
	// explicitly allocate 15000 bytes for lg by task(name, stack_size). The default stack size for every tak is 10000.
	logger* lg = new logger(sd, 15000); 

	// clock task will be invoked at every new clock tick before any other tasks could execute
	tk::sched::clock_task = lg;

	// invoke logger after all task finished
	// to do clean job, optional
	tk::sched::clock_task_exit = true;

	producer* pd = new producer("Tool", sd);
	customer* cs = new customer("XiaoMu", 2, sd);
	customer* cs2 = new customer("XiaoLan", 3, sd);

	// start all created task in a sequence
	currenttask->start();

	// write to disk & clear objects on exiting 
	tk::sched::exit_fct = exit;
}
