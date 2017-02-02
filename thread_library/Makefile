#Default Objects and Flags
TEST_CPU = test_cpu.cpp
TEST_MUTEX = test_mutex.cpp
TEST_MUTEX2 = test_mutex2.cpp
TEST_MUTEX3 = test_mutex3.cpp
TEST_THREAD = test_thread.cpp
TEST_JOIN = test_join.cpp
TEST_JOIN2 = test_join2.cpp
TEST_JOIN3 = test_join3.cpp
TEST_JOIN4 = test_join4.cpp
TEST_JOIN5 = test_join5.cpp
TEST_P1 = test_p1.cpp
TEST_CV = test_cv.cpp
TEST_CV2 = test_cv2.cpp
TEST_CV3 = test_cv3.cpp
TRY_THREAD_INT = try_thread_int.cpp
TRY_MUTEX_INT = try_mutex_int.cpp
TRY_CV_INT = try_cv_int.cpp
TRY_CV_INT_2 = try_cv_int_2.cpp
TRY_JOIN = try_join.cpp
TRY_JOIN2 = try_join2.cpp
TRY_MUTEX = try_mutex.cpp
TRY_CV = try_cv.cpp
TRY_MEM = try_mem.cpp
DONG1 = dong1.cpp
DONG2 = dong2.cpp
DONG3 = dong3.cpp
DONG4 = dong4.cpp
DONG5 = dong5.cpp
DONG6 = dong6.cpp
DONG7 = dong7.cpp
DONG8 = dong8.cpp
DONG9 = dong9.cpp
DONG10 = dong10.cpp
DONG11 = dong11.cpp
DONG12 = dong12.cpp

CC = g++
CXXFILES = global.cpp cpu.cpp mutex.cpp thread.cpp cv.cpp

FLAGS = -g -Wall  -std=c++11
LIBS = libcpu.a -ldl -pthread

all: test_cpu test_mutex test_mutex2 test_mutex3 test_thread test_join test_join2 test_join3 test_join4 test_join5 test_cv test_cv2 test_cv3 test_p1 try_join try_join2 try_mutex try_thread_int try_mutex_int try_cv_int try_cv_int_2 try_cv try_mem dong1 dong2 dong3 dong4 dong5 dong6 dong7 dong8 dong9 dong10 dong11 dong12

try_thread_int:
	$(CC) $(CXXFILES) $(TRY_THREAD_INT)  $(LIBS) $(FLAGS)  -o try_thread_int

try_mutex_int:
	$(CC) $(CXXFILES) $(TRY_MUTEX_INT)  $(LIBS) $(FLAGS)  -o try_mutex_int

try_cv_int:
	$(CC) $(CXXFILES) $(TRY_CV_INT)  $(LIBS) $(FLAGS)  -o try_cv_int

try_cv_int_2:
	$(CC) $(CXXFILES) $(TRY_CV_INT_2)  $(LIBS) $(FLAGS)  -o try_cv_int_2

try_join:
	$(CC) $(CXXFILES) $(TRY_JOIN)  $(LIBS) $(FLAGS)  -o try_join

try_join2:
	$(CC) $(CXXFILES) $(TRY_JOIN2)  $(LIBS) $(FLAGS)  -o try_join2

try_mutex:
	$(CC) $(CXXFILES) $(TRY_MUTEX)  $(LIBS) $(FLAGS)  -o try_mutex

try_cv:
	$(CC) $(CXXFILES) $(TRY_CV)  $(LIBS) $(FLAGS)  -o try_cv

try_mem:
	$(CC) $(CXXFILES) $(TRY_MEM)  $(LIBS) $(FLAGS)  -o try_mem

test_thread:
	$(CC) $(CXXFILES) $(TEST_THREAD)  $(LIBS) $(FLAGS)  -o test_thread

test_cpu:
	$(CC) $(CXXFILES) $(TEST_CPU)  $(LIBS) $(FLAGS)  -o test_cpu

test_mutex:
	$(CC) $(CXXFILES) $(TEST_MUTEX)  $(LIBS) $(FLAGS)  -o test_mutex

test_mutex2:
	$(CC) $(CXXFILES) $(TEST_MUTEX2)  $(LIBS) $(FLAGS)  -o test_mutex2

test_mutex3:
	$(CC) $(CXXFILES) $(TEST_MUTEX3)  $(LIBS) $(FLAGS)  -o test_mutex3

test_join:
	$(CC) $(CXXFILES) $(TEST_JOIN)  $(LIBS) $(FLAGS)  -o test_join	

test_join2:
	$(CC) $(CXXFILES) $(TEST_JOIN2)  $(LIBS) $(FLAGS)  -o test_join2	

test_join3:
	$(CC) $(CXXFILES) $(TEST_JOIN3)  $(LIBS) $(FLAGS)  -o test_join3

test_join4:
	$(CC) $(CXXFILES) $(TEST_JOIN4)  $(LIBS) $(FLAGS)  -o test_join4

test_join5:
	$(CC) $(CXXFILES) $(TEST_JOIN5)  $(LIBS) $(FLAGS)  -o test_join5

test_p1:
	$(CC) $(CXXFILES) $(TEST_P1)  $(LIBS) $(FLAGS)  -o test_p1

test_cv:
	$(CC) $(CXXFILES) $(TEST_CV)  $(LIBS) $(FLAGS)  -o test_cv

test_cv2:
	$(CC) $(CXXFILES) $(TEST_CV2)  $(LIBS) $(FLAGS)  -o test_cv2

test_cv3:
	$(CC) $(CXXFILES) $(TEST_CV3)  $(LIBS) $(FLAGS)  -o test_cv3

dong1:
	$(CC) $(CXXFILES) $(DONG1)  $(LIBS) $(FLAGS)  -o dong1

dong2:
	$(CC) $(CXXFILES) $(DONG2)  $(LIBS) $(FLAGS)  -o dong2

dong3:
	$(CC) $(CXXFILES) $(DONG3)  $(LIBS) $(FLAGS)  -o dong3

dong4:
	$(CC) $(CXXFILES) $(DONG4)  $(LIBS) $(FLAGS)  -o dong4

dong5:
	$(CC) $(CXXFILES) $(DONG5)  $(LIBS) $(FLAGS)  -o dong5

dong6:
	$(CC) $(CXXFILES) $(DONG6)  $(LIBS) $(FLAGS)  -o dong6

dong7:
	$(CC) $(CXXFILES) $(DONG7)  $(LIBS) $(FLAGS)  -o dong7

dong8:
	$(CC) $(CXXFILES) $(DONG8)  $(LIBS) $(FLAGS)  -o dong8

dong9:
	$(CC) $(CXXFILES) $(DONG9)  $(LIBS) $(FLAGS)  -o dong9		

dong10:
	$(CC) $(CXXFILES) $(DONG10)  $(LIBS) $(FLAGS)  -o dong10

dong11:
	$(CC) $(CXXFILES) $(DONG11)  $(LIBS) $(FLAGS)  -o dong11

dong12:
	$(CC) $(CXXFILES) $(DONG12)  $(LIBS) $(FLAGS)  -o dong12

clean:
	rm -f *.o *~ test_cpu test_mutex test_mutex2 test_mutex3 test_int test_thread test_join test_p1 try_join try_join2 test_join2 test_join3 test_join4 test_join5 test_cv test_cv2 test_cv3 try_mutex try_thread_int try_mutex_int try_cv_int try_cv_int_2 try_cv try_mem dong1 dong2 dong3 dong4 dong5 dong6 dong7 dong8 dong9 dong10 dong11 dong12
