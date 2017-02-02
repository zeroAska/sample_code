#ifndef _GLOBAL_H
#define _GLOBAL_H
#include <ucontext.h>
#include <map>
#include <list>
#include "thread.h"
// #define DEBUG
// macro
// mutex, CPU
#define BUSY false
#define FREE true

// thread
#define READY  0
#define WAIT  1
#define RUN   2
#define DIE 3

typedef int thread_id_t;
typedef int thread_status_t;
extern int total_thread_num;
extern thread_id_t global_id_count;
extern ucontext_t schedule_context;
extern ucontext_t going_to_die_context;
extern std::list<thread::impl *> ready_q;
extern std::atomic<bool> guard;
extern std::map<cpu*, bool> cpu_status;

void schedule(void);
void ready_q_print();
void send_ipi();

// TODO: destructor: set non-ptr value to -1; set ptr to NULL; 
//		set upper-level ptr to NULL (maybe add a mother* to each data structure?)

class thread::impl{
public:
	ucontext_t context;

	impl(thread_status_t, thread_id_t, thread_startfunc_t, void*);
	~impl();
	thread::impl * pop_join_q();
	void join();
	
	thread_id_t get_id();
	void set_id(thread_id_t);
	thread_status_t get_status();
	void set_status(thread_status_t);
	void internal_yield(thread_status_t s);
	void exit(void* retval);
	thread * get_mother();
	void set_mother(thread * t);
	void print_status();
private:
	thread_id_t id;
	thread * mother;
	thread_status_t status;
	std::list <thread::impl*> join_q;
};

class cpu::impl {
public:
	impl(thread_startfunc_t f, void * arg,thread** first_thread);
	~impl();
	thread::impl * get_curr_thread_impl_ptr();
	void set_curr_thread_impl_ptr(thread::impl* t);
private:
	thread::impl * curr_thread_impl_ptr;
};

class mutex::impl {
public:
	impl();
	~impl();
	void lock();
	void unlock();  
	void internal_lock();
	void internal_unlock();
	void print_status();
private:
	bool status;
	std::list <thread::impl*> block_q;
	thread::impl * curr_holder;
};

class cv::impl
{
public:
	impl();
	~impl();

	void wait(mutex & m);
	void signal();
	void broadcast();
	void print_status();
private:
	std::list<thread::impl*> wait_q;
};

void enable_int_guard_with_assert();
void disable_int_guard_with_assert();

// colorful console
#define NONE "\033[m"
#define DARK_RED "\033[0;32;31m"
#define RED "\033[1;31m"			// !
#define DARK_GREEN "\033[0;32;32m"
#define GREEN "\033[1;32m"			// !
#define DARK_BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"
#define CYAN "\033[0;36m"
#define BLUE "\033[1;36m"			// !  blue
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"			// !
#define LIGHT_GRAY "\033[0;37m"
#define WHITE "\033[1;37m"

#define SET_COLOR(x) printf(x);
#define CLEAR_COLOR SET_COLOR(NONE)

#endif
