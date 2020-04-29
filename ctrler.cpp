#include <iostream>
#include <iomanip> // setw
#include "tk.h"


namespace tk {

const int ctrler::SEN_RANGE;
const int ctrler::UNTOUCHED;
ctrler* ctrler::ctrlerchain = 0;
ctrler::SP ctrler::sp = &ctrler::sentinel;


void ctrler::insert_after(ctrler* cb)
{
	if (next || prev || ctrlerchain == this)
		object::task_error(E_CLINE, 0);

	//setup sentinel
	for (int i=0; i<SEN_RANGE; i++) sentinel[i]=UNTOUCHED;

	if (!cb){ // insert this at the front of ctrler chain
		if (ctrlerchain) ctrlerchain->prev=this;
		next = ctrlerchain;

		ctrlerchain = this;
	} else {
		next = cb->next;
		prev = cb;

		if(cb->next) cb->next->prev = this;
		cb->next = this;

		if (!ctrlerchain) { // cb hasn't been in the ctrlerchain
			//setup sentinel
			for (int i=0; i<SEN_RANGE; i++) cb->sentinel[i]=UNTOUCHED;

			ctrlerchain = cb;
		} 
	}
}


void ctrler::erase()
{
	if (prev) prev->next = next;
	if (next) next->prev = prev;

	if (this == ctrlerchain) ctrlerchain = next;

	next = prev = 0;
}


bool ctrler::merge()
{
	if (!next || next->state == USED)
	       	return false;
	else if (next->state == FREE && state ==OBSOLETE )
	       	return false;
	else if (state == USED)
	       	return false;
	else {
		acquired_size += next->acquired_size;
		next->erase();
		return true;
	}
}


const char* state_string(ctrler::statetype s)
{
	switch (s) {
	case ctrler::FREE:		return "FREE";
	case ctrler::USED:	return "USED";
	case ctrler::OBSOLETE:		return "OBSOLETE";
	}
}


void ctrler::print(int n)
{
	const char* s = state_string(state);
	const char* na = state == USED ? tk->t_name : "";
	std::cout<<"ctrler "<<s<<" "
		<<&(this->jmpb)<<" "
		<<acquired_size<<" "
		<<na	<<std::endl;

	if (n & CHAIN) {
		if (next) next->print(n);
	}
}

} // namespace tk
