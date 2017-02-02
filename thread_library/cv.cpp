#include "cv.h"
#include "cpu.h"
#include "global.h"
#include <list>
#include <iostream>
#include <atomic>
#include <cstdlib>
#include <cassert>

using std::cout;
using std::runtime_error;
using std::endl;

cv::cv() {
	try {
		impl * cv_i = new impl;
		this->impl_ptr = (impl *) cv_i;
	}
	catch (std::bad_alloc& e) {
		#ifdef DEBUG
			cout<<"[cv] memory alloc failed: "<<e.what()<<endl;
		#endif
		throw e;
	}
}

cv::~cv() {
	delete (impl *)(this->impl_ptr);
	this->impl_ptr = NULL;
}

void cv::wait(mutex & m){
	((impl *)(this->impl_ptr))->wait(m);
}

void cv::signal(){
	((impl *)(this->impl_ptr))->signal();
}

void cv::broadcast(){
	((impl *)(this->impl_ptr))->broadcast();
}

cv::impl::impl(){}

cv::impl::~impl(){
	#ifdef DEBUG
		SET_COLOR(GREEN);
		print_status();
		CLEAR_COLOR;
	#endif
}

void cv::impl::wait(mutex & mu){
	#ifdef DEBUG
		std::cout<<"[cv::impl::wait] mutex unlocked"<<std::endl;
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while( guard.exchange(true) ){}

	mu.impl_ptr->internal_unlock();  // releasing the lock

	#ifdef DEBUG
		assert(cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status()==READY);
	#endif

	// put current thread to wait_q of this cv
	this->wait_q.push_back(cpu::self()->impl_ptr->get_curr_thread_impl_ptr());
	cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->internal_yield(WAIT);

	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif

	mu.impl_ptr->internal_lock();

	guard.store(false);
	cpu::interrupt_enable();
}

void cv::impl::signal(){
	
	// put the first one on the wait_q to the global ready_q;
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif
	
	cpu::interrupt_disable();
	while( guard.exchange(true) ){}
	
	#ifdef DEBUG
		std::cout<<"[cv::impl::signal] wait_q.empty() is " << this->wait_q.empty() <<std::endl;
	#endif		

	if (this->wait_q.empty() == false) {
		#ifdef DEBUG
			assert(this->wait_q.front()->get_status()==WAIT);
		#endif
		
		this->wait_q.front()->set_status(READY);

		#ifdef DEBUG
			assert(this->wait_q.front()->get_status()==READY);
		#endif

		ready_q.push_back(this->wait_q.front());
		this->wait_q.pop_front();		
	}

	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif
	
	guard.store(false);
	cpu::interrupt_enable();
}

void cv::impl::broadcast(){
	// put every element on the wait_q to the global ready_q;
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while( guard.exchange(true) ){}

	while (this->wait_q.empty() == false) {
		#ifdef DEBUG
			assert(this->wait_q.front()->get_status()==WAIT);
		#endif

		this->wait_q.front()->set_status(READY);

		#ifdef DEBUG
			assert(this->wait_q.front()->get_status()==READY);
		#endif
		
		ready_q.push_back(this->wait_q.front());
		this->wait_q.pop_front();
	}

	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif
	guard.store(false);
	cpu::interrupt_enable();
}

void cv::impl::print_status(){
	cout << "[cv] wait_q:\n";
	for (auto& it: wait_q){
		cout << (*it).get_id() << "\t";
	}
	cout << endl;
}