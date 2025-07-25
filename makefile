CC = gcc
CFLAGS = -Wall -Wextra -I.
LDFLAGS = -lpthread -lrt

OBJS =  main.o producer.o consumer.o
DEPS = factory.h

LOGGER_OBJS = Prodlogger.o conslogger.c

main : $(OBJS)
	$(CC) -o main $(OBJS) $(LDFLAGS)

prod_log : Prodlogger.c
	$(CC) Prodlogger.c -o Prodlogger $(LDFLAGS)

cons_log : conslogger.c
	$(CC) conslogger.c -o conslogger $(LDFLAGS)

%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o main prod_log cons_log


shmclean:
	-@sem_unlink /prod_sem_log_ready
	-@sem_unlink /prod_sem_log_written
	-@shm_unlink /prod_log_shm
	-@sem_unlink /cons_sem_log_ready
	-@sem_unlink /cons_sem_log_written
	-@shm_unlink /cons_log_shm