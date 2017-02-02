#include "global.h"
#include "thread.h"
#include <list>
#include <iostream>
#include <map>
#include <cassert>

using std::list;
using std::map;
using std::cout;
using std::endl;

list<thread::impl*> ready_q;
int total_thread_num = 0;
map<cpu*, bool> cpu_status;

// NOTE: scheduler assumes that interrupt are disabled, and guard are set
//       before entering the context
void schedule(void){
	#ifdef DEBUG
		cout << "enter schedule function" << endl;
		fflush(stdout);
		assert_interrupts_disabled();
	#endif

	while(1){
		if (cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status() == DIE){
			#ifdef DEBUG
				cout << "[schedule] thread "<<cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id()<<" finish and die" << endl;
			#endif

			total_thread_num --;

			// join(): when B DIE, taking threads in join_q to ready_q. 
			// 		Set B's impl_ptr to null
			thread::impl* temp = cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->pop_join_q();
			while( temp != NULL )   {
				#ifdef DEBUG
					cout<<"[schedule] thread "<<cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id()<<" finish and die" << endl;
					cout<<"[schedule] push join_q to ready_q: thread id is "<<temp->get_id()<<endl;
					SET_COLOR(GREEN);
					cout<<"[schedule] thread id "<< temp->get_id()<<" status: "<<temp->get_status()<<endl;
					cout<<"[schedule] join_q with first element: "<<temp->get_id()<<endl;
					cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->print_status();
					CLEAR_COLOR;
                #endif

				#ifdef DEBUG
					assert(temp->get_status()==WAIT);
				#endif

				temp->set_status(READY);

				#ifdef DEBUG
					assert(temp->get_status()==READY);
				#endif

				ready_q.push_back(temp);
				temp = cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->pop_join_q();
			}
			temp = cpu::self()->impl_ptr->get_curr_thread_impl_ptr();
			delete temp;
			cpu::self()->impl_ptr->set_curr_thread_impl_ptr(nullptr);

			//if(total_thread_num==0 || ready_q.empty() ) break;
    	}
    	else if(cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status() == WAIT){
				cpu::self()->impl_ptr->set_curr_thread_impl_ptr(nullptr);
      			//some operations for join thread
    	}
    	else {//still runable
      		ready_q.push_back(cpu::self()->impl_ptr->get_curr_thread_impl_ptr());
			cpu::self()->impl_ptr->set_curr_thread_impl_ptr(nullptr);
    	}

		// TODO: for multi-cpu case, if now we still have runnable threads in the ready_q, and other cpus are sleeping, we can call cpu::interrupt_send to wake up other cpu.
		// The way to check other cpu's status is to have a new global variable: map<cpu *,int> cpus.  then we can check the status one by one, and assign threads to them
		// TODO: if there are no other on the ready_q, call cpu::interrupt_enable_suspend
		if( total_thread_num==0 || ready_q.empty() ) break;
 	
    	assert(ready_q.empty() == false);
    	cpu::self()->impl_ptr->set_curr_thread_impl_ptr(ready_q.front());
		ready_q.pop_front();

		#ifdef DEBUG
			cout << "[schedule] thread list" <<endl;
			for (auto it = ready_q.begin(); it != ready_q.end(); ++it){
				cout << (*it)->get_id() << "\t";
			}
			cout << endl;
			cout << "[schedule] swap out of schedule function, curr thread id "<< cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id() << endl;
			fflush(stdout);
		#endif

		// TO CHECK: before swap:
		// need to release all the interrupts and guards here
			// Scene 1: when the next thread is a newly made one, if we swapcontext to it, it will  automatically enable interrupt and disable guard in start_func_routine(). So we need to do this here
			// Scene 2: when the next thread A is an old one, and A experienced yield/interrupt  before. Then after it go back executing, it will automatically  with enabling interrupt here
			// Scene 3: when the next thread A is an old one, and A use lock/cv.wait(using internal_yield) thus sleep, after swap, it will auto release the interrupt/guard. In this case, we don't need to release the interrrupt here.
		
		swapcontext(&schedule_context,&(cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->context));

		// TO CHECK: after switching back
		// case 1: the thread function finishes 
		// case 2: the thread function calls yield/interrupt again
		// case 3: internal yield
		#ifdef DEBUG
			cout << "[schedule] swap into schedule function" << endl;
			fflush(stdout);
		#endif	
  	}

	#ifdef DEBUG
		cout << "[schedule] leave schedule, whole program exit" << endl;
	#endif

	swapcontext(&schedule_context,&going_to_die_context);
}

void ready_q_print() {
	std::cout<<"ready q is ";
	for (auto && item : ready_q) {
		std::cout<<item->get_id()<< " ";
	}
	std::cout<<"\n";
}

void disable_int_enable_guard_with_assert() {
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif
	cpu::interrupt_disable();
	while ( guard.exchange(true)) {}
}

void enable_int_disable_guard_with_assert() {
	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif	
	guard = false;
	cpu::interrupt_enable();
}

void send_ipi() {
	for(auto& it: cpu_status) {
		if( FREE == it.second ) {
			it.second = BUSY;
			it.first->interrupt_send();
			break;
		}
	}
}
