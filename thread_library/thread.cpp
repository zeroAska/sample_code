#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include "global.h"
#include "thread.h"
#include <atomic>
#include <cassert>
#include <cstdlib>

thread_id_t thread_count = 0;
thread_id_t global_id_count = 0;
ucontext_t schedule_context;
ucontext_t going_to_die_context;
using namespace std;

thread::thread(thread_startfunc_t func, void * arg){
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while ( guard.exchange(true) ) {}

	#ifdef DEBUG
		cout<<"[thread] constructor disable interrupt fin"<<endl;
	#endif

	total_thread_num ++;
	try {
		impl_ptr = new impl(READY, global_id_count, func, arg); 
	} catch (bad_alloc& e) {
		cout<<"[thread] memory alloc failed: "<<e.what()<<endl;
		assert_interrupts_disabled();
		guard.store(false);
		cpu::interrupt_enable();
		assert_interrupts_enabled();
		throw e;
	}

	impl_ptr->set_mother(this);

	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif
	#ifdef DEBUG
		cout<<"[thread] constructor going to interrupt"<<endl;
	#endif
	guard.store(false);
	cpu::interrupt_enable();
}

thread::~thread(){
	#ifdef DEBUG
		if (this->impl_ptr != NULL)
			cout <<"[~thread] " << this->impl_ptr->get_id() <<" call destructor" <<endl;
		else
			cout<<"[~thread] thread impl is null"<<endl;
	#endif

	if (this->impl_ptr != NULL) { 
	    impl_ptr->set_mother(NULL);
	    impl_ptr = NULL;
	}
}

void thread::join() {
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while(guard.exchange(true));
	#ifdef DEBUG
		cout<<"[thread::join] thread impl_ptr to join is "<<impl_ptr<<endl;
	#endif

	if (impl_ptr != NULL)
		impl_ptr->join();

	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif

	guard.store(false);
	cpu::interrupt_enable();	
}

void thread::yield(){
	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while ( guard.exchange(true) ) {}

	swapcontext(&((cpu::self())->impl_ptr->get_curr_thread_impl_ptr()->context), &schedule_context);
	
	#ifdef DEBUG
		assert_interrupts_disabled();
	#endif	

	guard.store(false);
	cpu::interrupt_enable();
}

void start_routine_helper(thread_startfunc_t func, void *arg){
	#ifdef DEBUG
		cout<<"enter start_routine_helper"<<endl;
		assert_interrupts_disabled();
	#endif

	cpu::interrupt_enable();
	guard.store(false);

	#ifdef DEBUG
		cout<<"enter func"<<endl;
	#endif

	func(arg);

	#ifdef DEBUG
		assert_interrupts_enabled();
	#endif

	cpu::interrupt_disable();
	while (guard.exchange(true));
	cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->exit(0);
}

thread::impl::impl(thread_status_t s, thread_id_t i,thread_startfunc_t func, void* arg){
	global_id_count = global_id_count + 1;
	status = s;
	id = i;

	getcontext(&context);
	char *stack = new char[STACK_SIZE];
	context.uc_stack.ss_sp = stack;
	context.uc_stack.ss_size = STACK_SIZE;
	context.uc_stack.ss_flags = 0;
	context.uc_link = &schedule_context;
	makecontext(&context,(void (*)())start_routine_helper, 2, func, arg);
	ready_q.push_back(this);
}

void thread::impl::set_mother(thread * t) {
	this->mother = t;
}

thread * thread::impl::get_mother() {
	return mother;
}

thread::impl::~impl(){
	#ifdef DEBUG
		cout<<"[thread::impl:~impl()] destructor free stack mem"<<endl;
	#endif
		
	delete [] (char *)(context.uc_stack.ss_sp);
	status = (thread_status_t)-1;
	if (get_mother() != NULL)
		get_mother()->impl_ptr = NULL;
	mother = NULL;

	#ifdef DEBUG
		print_status();
	#endif

	id = (thread_id_t)-1;
}

// assumes guard and interrupt will be re-enabled after yield
void thread::impl::internal_yield(thread_status_t s){
	#ifdef DEBUG
		cout<<"[thread::impl::internal_yield] thread id "<<cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id()<<": status = WAIT\n";
	#endif
	set_status(s);
	swapcontext(&context,&schedule_context);
}

thread_id_t thread::impl::get_id(){
	return id;
}

void thread::impl::set_id(thread_id_t new_id){
	id = new_id;
}

thread_status_t thread::impl::get_status(){
	return status;
}

void thread::impl::set_status(thread_status_t new_status){
	status = new_status;
}

void thread::impl::exit(void* retval){
	set_status(DIE);
	#ifdef DEBUG
		cout << id << "[thread::impl::exit] die" <<endl;
	#endif
}

thread::impl * thread::impl::pop_join_q(){
	if(!join_q.empty()){
		#ifdef DEBUG
			cout << id << "[thread::impl::pop_join_q] join_q not empty" <<endl;
		#endif

		thread::impl* temp = join_q.front();
		join_q.pop_front();
		return temp;
	}
	else{
		#ifdef DEBUG
			cout << "[thread::impl::join_q] thread " << id <<" join_q empty" <<endl;
		#endif
		return nullptr;
	}
}

// NOTE: scheduler assumes that interrupt are disabled, and guard are set
//       before entering the context
void schedule(void){
	//disable interrupt
	#ifdef DEBUG
		cout << "enter schedule function" << endl;
		fflush(stdout);
	#endif

	while(1){
		if (cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status() == DIE){
			#ifdef DEBUG
				cout << "[schedule]: thread "<<cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id()<<" finish and die" << endl;
			#endif

			total_thread_num --;

			// join(): when B DIE, taking threads in join_q to ready_q. 
			// 		Set B's impl_ptr to null
			thread::impl* temp = cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->pop_join_q();
			while( temp != NULL )   {
				#ifdef DEBUG
					cout << "[schedule]: thread "<<cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id()<<" finish and die" << endl;
					cout<<"[schedule] push join q to ready q: thread id is "<<temp->get_id()<<endl;
                                #endif
				ready_q.push_back(temp);
				temp->set_status(READY);
				temp = cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->pop_join_q();
				// TODO: send_ipi to one free cpu if existing
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
		if(total_thread_num==0 || ready_q.empty() ) break;
 	
    	assert(ready_q.empty() == false);
    	cpu::self()->impl_ptr->set_curr_thread_impl_ptr(ready_q.front());
		ready_q.pop_front();

		#ifdef DEBUG
			cout << "[scheduler] thread list" <<endl;
			for (auto it = ready_q.begin(); it != ready_q.end(); ++it){
				cout << (*it)->get_id() << "\t";
			}
			cout << endl;
			cout << "swap out of schedule function, curr thread id "<< cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id() << endl;
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
			cout << "swap into schedule function" << endl;
			fflush(stdout);
		#endif	
  	}

	#ifdef DEBUG
		cout << "leave schedule, whole program exit" << endl;
	#endif

	swapcontext(&schedule_context,&going_to_die_context);
}


void thread::impl::join() 
{
	// 0. if B has already died before being joined (impl_ptr==null), goto 2;
	// 		else, goto 1
	#ifdef DEBUG
		cout<<"[thread::impl::join] get_status of <thread B> "<<get_id()<<" is "<<get_status()<<endl;
		cout<<"[thread::impl::join] status of <thread calls B.join()> "<<cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_id()<<" is "<<cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status()<<endl;
	#endif

	if( get_status() != DIE) 
	{
		// 1. add A to B's join_q; set status of A to WAIT
		#ifdef DEBUG
			assert(cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->get_status()==READY);
		#endif

		join_q.push_back(cpu::self()->impl_ptr->get_curr_thread_impl_ptr());
		cpu::self()->impl_ptr->get_curr_thread_impl_ptr()->internal_yield(WAIT);
	}
	// 2. join ends
}

void thread::impl::print_status() {
	SET_COLOR(GREEN);

	cout << "[thread] thread ID: " << id << endl;
	cout << "[thread] status: " << status << endl;

	cout << "[thread] mother thread ptr: ";
	if(mother!=NULL && mother->impl_ptr!=NULL) {
		cout << mother->impl_ptr->get_id() << endl;
	} else {
		cout << "NULL ptr\n";
	}

	cout << "[thread] join_q:\n";
	for (auto& it: join_q){
		cout << (*it).get_id() << "\t";
	}
	cout << endl;
	CLEAR_COLOR;
}
