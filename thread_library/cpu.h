/*
 * cpu.h -- interface to the simulated CPU
 *
 * This interface is meant to be used by the thread library.
 * The only part that is used by application programs is cpu::boot()
 *
 * This class is implemented almost entirely by the infrastructure; the
 * only part implemented by the thread library is cpu::init().
 */
#ifndef _CPU_H
#define _CPU_H

#include "thread.h"

typedef void (*interrupt_handler_t) (void);

static const unsigned int MAX_CPUS=16;         // maximum number of CPUs

class cpu {
public:
    /*
     * cpu::boot() starts all CPUs in the system.  The number of CPUs is
     * specified by num_cpus.
     * One CPU will call cpu::init(func, arg); the other CPUs will call
     * cpu::init(nullptr, nullptr).
     *
     * On success, cpu::boot() does not return.
     *
     * async, sync, random_seed configure each CPU's generation of timer
     * interrupts (which are only delivered if interrupts are enabled).
     * Timer interrupts in turn cause the current thread to be preempted.
     *
     * The sync and async parameters can generate several patterns of interrupts:
     *
     *     1. async = true: generate interrupts asynchronously every 1 ms using
     *        SIGALRM.  These are non-deterministic.  
     *
     *     2. sync = true: generate synchronous, pseudo-random interrupts on each
     *        CPU.  You can generate different (but repeatable) interrupt
     *        patterns by changing random_seed.
     *
     * An application will be deterministic if num_cpus=1 && async=false.
     */
    static void boot(unsigned int num_cpus, thread_startfunc_t func, void *arg,
                     bool async, bool sync, unsigned int random_seed);

    /*
     * interrupt_disable() disables interrupts on the executing CPU.  Any
     * interrupt that arrives while interrupts are disabled will be saved
     * and delivered when interrupts are re-enabled.
     *
     * interrupt_enable() and interrupt_enable_suspend() enable interrupts
     * on the executing CPU.
     *
     * interrupt_enable_suspend() atomically enables interrupts and suspends
     * the executing CPU until it receives an interrupt from another CPU.
     * The CPU will ignore timer interrupts while suspended.
     *
     * These functions should only be called by the thread library (not by
     * an application).  They are static member functions because they always
     * operate on the executing CPU.
     *
     * Each CPU starts with interrupts disabled.
     */
    static void interrupt_disable();
    static void interrupt_enable();
    static void interrupt_enable_suspend();

    /*
     * interrupt_send() sends an inter-processor interrupt to the specified CPU.
     * E.g., c_ptr->interrupt_send() sends an IPI to the CPU pointed to by c_ptr.
     */
    void interrupt_send();

    /*
     * The interrupt vector table specifies the functions that will be called
     * for each type of interrupt.  There are two interrupt types: TIMER and
     * IPI.
     */
    static const unsigned int TIMER = 0;
    static const unsigned int IPI = 1;
    interrupt_handler_t interrupt_vector_table[IPI+1];

    static cpu *self();                 // returns pointer to the cpu this thread
                                        // is running on

    class impl;                         // defined by the thread library
    impl *impl_ptr;                     // used by the thread library

    /*
     * Disable the default copy constructor and copy assignment operator.
     */
    cpu(const cpu&) = delete;
    cpu& operator=(const cpu&) = delete;
    cpu(cpu&&) = delete;
    cpu& operator=(cpu&&) = delete;

private:
    /*
     * cpu::init() initializes a CPU.  It is provided by the thread library
     * and called by the infrastructure.  After a CPU is initialized, it
     * should run user threads as they become available.  If func is not
     * nullptr, cpu::init() also creates a user thread that executes func(arg).
     *
     * On success, cpu::init() should not return to the caller.
     */
    void init(thread_startfunc_t, void *);

    cpu();                              // defined and used by infrastructure
    static void boot_helper(void *);    // defined and used by infrastructure
};

/*
 * The infrastructure provides an atomic guard variable, which thread
 * libraries should use to provide mutual exclusion on multiprocessors.
 * The switch invariant for multiprocessors specifies that this guard variable
 * must be true when calling swapcontext.  guard is initialized to false.
 */
#include <atomic>
extern std::atomic<bool> guard;

/*
 * assert_interrupts_disabled() and assert_interrupts_enabled() can be used
 * as error checks inside the thread library.  They will assert (i.e. abort
 * the program and dump core) if the condition they test for is not met.
 */
#ifdef NDEBUG

#define assert_interrupts_disabled()
#define assert_interrupts_enabled()

#else // NDEBUG

#define assert_interrupts_disabled()                                    \
                assert_interrupts_private(__FILE__, __LINE__, true)
#define assert_interrupts_enabled()                                     \
                assert_interrupts_private(__FILE__, __LINE__, false)

/*
 * assert_interrupts_private() is a private function for the interrupt
 * functions.  Your thread library should not call it directly.
 */
extern void assert_interrupts_private(const char *, int, bool);

#endif // NDEBUG

#endif /* _CPU_H */
