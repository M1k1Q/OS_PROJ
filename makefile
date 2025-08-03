CC = gcc
CFLAGS = -Wall -Wextra -I.
LDFLAGS = -lpthread -lrt

OBJS =  main.o producer.o consumer.o MakeCar.o
DEPS = factory.h

LOGGER_OBJS = Prodlogger.o conslogger.c

main : $(OBJS)
	$(CC) -o main $(OBJS) $(LDFLAGS)

prod_log : Prodlogger.c	$(DEPS)
	$(CC) Prodlogger.c -o prodlog $(LDFLAGS)

cons_log : conslogger.c	$(DEPS)
	$(CC) conslogger.c -o conslog $(LDFLAGS)

cm_log : CarMakeLogger.c $(DEPS)
	$(CC) CarMakeLogger.c -o Cmlog $(LDFLAGS)


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o main prodlog conslog Cmlog


shmclean:
	-@sem_unlink /prod_sem_log_ready
	-@sem_unlink /prod_sem_log_written
	-@shm_unlink /prod_log_shm
	-@sem_unlink /cons_sem_log_ready
	-@sem_unlink /cons_sem_log_written
	-@shm_unlink /cons_log_shm
	-@sem_unlink /CarMake_sem_log_ready
	-@sem_unlink /CarMake_sem_log_written
	-@shm_unlink /CarMake_log_shm
	
