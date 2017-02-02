#include "mutex.h"
#include "cpu.h"
#include "thread.h"
#include "global.h"
#include <cassert>
#include <list>
#include <iostream>
#include <atomic>
#include <string>
#include <cstdlib>

using std::cout;
using std::runtime_error;
using std::string;
using std::endl;

mutex::mutex() {
	try {
		impl_ptr = new impl;
	}
	catch (std::bad_alloc& e) {
		#ifdef DEBUG
			cout<<"[mutex] memory alloc failed: "<<e.what()<<endl;
		#endif
		throw e;
	}		
}

mutex::impl::impl() {
	status = FREE;
	curr_holder = NULL;	
}

mutex::impl::~impl() {
	for (auto && item : block_q) {
		delete item;
	}
	#ifdef DEBUG
		SET_COLOR(GREEN);
		print_status();
		CLEAR_COLOR;
	#endif
}

void mutex::lock()
{
	impl_ptr->lock();
}

void mutex::unlock()
{
	impl_ptr->unlock();
}

mutex::~mutex() {delete impl_ptr;}

void mutex::impl::lock()
{
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while (guard.exchange(true)) {}

	internal_lock();

	#ifdef DEBUG
		cout<<"[mutex::impl::lock] mutex locked, thread id is "<<curr_holder->get_id()<<"\n";
		assert_interrupts_disabled();
	#endif

	guard.store(false);
	cpu::interrupt_enable();
}

void mutex::impl::unlock()
{
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while (guard.exchange(true)) {}

	internal_unlock();

	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif
	
	guard.store(false);
	cpu::interrupt_enable();
}

void mutex::impl::internal_lock() {
	if (status == FREE)
	{
		status = BUSY;
		curr_holder = cpu::self()->impl_ptr->get_curr_thread_impl_ptr();
	} else
	{
		#ifdef DEBUG
			assert(cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status()==READY);
		#endif
		// add thread to block_queue
		block_q.push_back(cpu::self()->impl_ptr->get_curr_thread_impl_ptr());
		// switch to next ready thread
		cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->internal_yield(WAIT); 
	}
}

void mutex::impl::internal_unlock() {
	assert_interrupts_disabled();
	if( status == FREE || curr_holder != cpu::self()->impl_ptr->get_curr_thread_impl_ptr() ) {
		guard.store(false);
		cpu::interrupt_enable();
		throw runtime_error("[mutex::impl::internal_unlock] try to unlock a mutex that it never holds\n");		
	}

	#ifdef DEBUG
		cout<<"[mutex::impl::internal_unlock] mutex unlock, thread id is "<<curr_holder->get_id()<<", block_q.empty() is "<< block_q.empty()<<endl;
		SET_COLOR(LIGHT_PURPLE);
		print_status();
		CLEAR_COLOR;
		assert(status==BUSY);
		assert(curr_holder==cpu::self()->impl_ptr->get_curr_thread_impl_ptr());
		assert(cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status()==READY);
	#endif
	
	status = FREE;
	if( !block_q.empty() )
	{	
		#ifdef DEBUG
			assert(block_q.front()->get_status()==WAIT);
		#endif
			
		// move waiting thread to ready_q
		block_q.front()->set_status(READY);

		#ifdef DEBUG
			assert(block_q.front()->get_status()==READY);
		#endif

		ready_q.push_back(block_q.front());
		curr_holder = block_q.front();
		block_q.pop_front();

		#ifdef DEBUG
			SET_COLOR(RED);
			cout << "[mutex::impl::internal_unlock] after unlock: ";
			ready_q_print();
			CLEAR_COLOR;
		#endif
		
		status = BUSY;
	} else {
		curr_holder = NULL;
	}
}

void mutex::impl::print_status(){
	cout << "[mutex] status: " << status << endl;
	cout << "[mutex] block_q:\n";
	for (auto& it: block_q){
		cout << (*it).get_id() << "\t";
	}
	cout << endl;
	cout << "[mutex] current holder: ";
	if(curr_holder!=NULL) {
		cout << curr_holder->get_id() << endl;
	} else {
		cout << "NULL ptr" << endl;
	}
}
