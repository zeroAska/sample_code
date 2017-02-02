#include "thread.h"
#include "global.h"
#include <cassert>
#include <list>
#include <ucontext.h>
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <atomic>

using namespace std;

static void timer_int_handler(void){
	#ifdef DEBUG
		SET_COLOR(BLUE);
		cout << "[timer_int_handler] start\n";
		CLEAR_COLOR;
	#endif

	cpu::interrupt_disable();
	while(guard.exchange(true)) {}

	if (ready_q.empty() == false ) {
		assert(cpu::self()->impl_ptr != NULL); // during cpu::init()
		thread::impl * curr = cpu::self()->impl_ptr->get_curr_thread_impl_ptr();
		
		#ifdef DEBUG
			cout<<"[timer_int_handler] ready q not empty, curr is "<<curr<<endl;
			//assert(curr != NULL);
		#endif
		
		if (curr == NULL) {	
			// This will happen after creating the first thread, but the interrupt happens before assigning the curr_thread
		} else {
			curr->internal_yield(READY);
		}
	}
	
	#ifdef DEBUG
		SET_COLOR(BLUE);
		cout<<"[timer_int_handler] ends"<<endl;
		CLEAR_COLOR;
	#endif

	guard.store(false);
	cpu::interrupt_enable();
}

static void ipi_int_handler(void){
	#ifdef DEBUG
		cout<<"[IPI_int_handler] starts"<<endl;
	#endif

	cpu::interrupt_disable();
	while (guard.exchange(true)) {}

	if (ready_q.empty() == false) {
		assert(cpu::self()->impl_ptr != NULL);
		thread::impl * curr = cpu::self()->impl_ptr->get_curr_thread_impl_ptr();
		// #ifdef DEBUG
		// 	assert(curr != NULL);
		// #endif
		// if (curr == NULL) {
		// 	//TODO: multi cpu wake up
		// }
		// else {
		// 	curr->internal_yield(READY);
		// }
		assert(curr==NULL);
		if(curr != NULL) {
			// TODO: shouldn't be the case?
		} else { // wake up

		}

		guard.store(false);
		cpu::interrupt_enable();
	} else {
		guard.store(false);
		cpu::interrupt_enable_suspend();
	}

	#ifdef DEBUG
		cout<<"[IPI_int_handler] ends"<<endl;
	#endif
}

void cpu::init(thread_startfunc_t f, void * arg) {
	interrupt_vector_table[TIMER] = &timer_int_handler;
	interrupt_vector_table[IPI] = &ipi_int_handler;
	
	thread* first_thread = NULL;

	// TODO: this line is buggy. If interrupt happens inside, cpu::self()->impl_ptr will segfault
	try {
		impl_ptr = new impl(f, arg, &first_thread);		
	} catch (std::bad_alloc& e) {
		assert_interrupts_disabled();
		guard.store(false);
		cpu::interrupt_enable();	
		assert_interrupts_enabled();	
		throw e;
	}

	if(f != NULL){
		#ifdef DEBUG
			cout<<"[cpu::init] swap to the first thread"<<endl;
		#endif
		assert(first_thread != NULL);
		assert(first_thread->impl_ptr != NULL);
		   
		swapcontext(&going_to_die_context, &(first_thread->impl_ptr->context));
	}
	// TODO: move this line, since first_thread will not exist for empty f
	delete first_thread;
	//interrupt_disable();
	#ifdef DEBUG
		cout<<"[cpu::init] remainning thread num is "<<total_thread_num<<endl;
	#endif
	interrupt_enable_suspend();
}

cpu::impl::impl(thread_startfunc_t func, void * arg, thread** first_thread) {
<<<<<<< HEAD
	if(func != NULL)
		init(func, arg,first_thread);
}

void cpu::impl::init(thread_startfunc_t func, void* arg, thread** first_thread) {
  // Note: make context for scheduler, and it should only run once
	getcontext(&schedule_context);
	try {
		schedule_context.uc_stack.ss_sp = new char[STACK_SIZE];		
	} catch (std::bad_alloc& e) {
		#ifdef DEBUG
			cout<<"cpu impl memory alloc failed: "<<e.what()<<endl;
		#endif
		assert_interrupts_disabled();
		guard.store(false);
		cpu::interrupt_enable();
		throw e;
	}

	schedule_context.uc_stack.ss_size = STACK_SIZE;
	schedule_context.uc_link = &going_to_die_context;
	makecontext(&schedule_context,schedule,0);

	// NOTE: when interrupt happens in this function, cpu::self()->impl_ptr will be null, and interrupt handler will segfault
	// TODO: useful in ipi for multicpu
	cpu::self()->impl_ptr = this;

    #ifdef DEBUG
		cout << "cpu first thread create" << endl; 
		assert_interrupts_disabled();
	#endif
=======
	if(func != NULL) {
		getcontext(&schedule_context);
		char *stack = new char[STACK_SIZE];
		schedule_context.uc_stack.ss_sp = stack;		
		schedule_context.uc_stack.ss_size = STACK_SIZE;
		schedule_context.uc_stack.ss_flags = 0;
		schedule_context.uc_link = &going_to_die_context;
		makecontext(&schedule_context,schedule,0);

		// NOTE: when interrupt happens in this function, cpu::self()->impl_ptr will be null, and interrupt handler will segfault
		cpu::self()->impl_ptr = this;

	    #ifdef DEBUG
			cout << "[cpu::impl::init] cpu first thread created" << endl;
			assert_interrupts_disabled();
		#endif
>>>>>>> f54db2bb4621b8e63f57ae157233f845b18c7a40

		cpu::interrupt_enable();
		*first_thread = new thread(func, arg);	

		#ifdef DEBUG
			assert_interrupts_enabled();
			cout << "[cpu::impl::init] first thread " << (*first_thread)->impl_ptr->get_id()<<" create finish" <<endl;		
		#endif

<<<<<<< HEAD
	cpu::interrupt_disable();
	while (guard.exchange(true));
	// TODO: this line is useful in ipi for multi cpu. first thread is popped from ready q in ipi
	curr_thread_impl_ptr = (*first_thread)->impl_ptr;
	ready_q.pop_front();
	#ifdef DEBUG
		cout<<"[cpu] pop the first thread onto the ready queue"<<endl;
	#endif
=======
		cpu::interrupt_disable();
		while (guard.exchange(true));
		curr_thread_impl_ptr = (*first_thread)->impl_ptr;
		ready_q.pop_front();
		#ifdef DEBUG
			cout<<"[cpu::impl::init] pop the first thread onto the ready queue"<<endl;
		#endif
	}		
>>>>>>> f54db2bb4621b8e63f57ae157233f845b18c7a40
}

thread::impl * cpu::impl::get_curr_thread_impl_ptr(){
	return curr_thread_impl_ptr  == NULL ? NULL : curr_thread_impl_ptr;
}

void cpu::impl::set_curr_thread_impl_ptr(thread::impl* t){
	curr_thread_impl_ptr = t;
}
