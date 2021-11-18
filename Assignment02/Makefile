OBJS = main.o sensor_node.o base_station.o generate_height.o circular_queue.o report.o
CC = mpicc
CFLAGS = -Wall -c
LFLAGS = -Wall
LIBS = -lm -lpthread

WSN: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@ $(LIBS)

# compile every c files into object files
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@  

# default run
run:
	mpirun -np 10 -oversubscribe ./WSN 3 3 6000 5
	
clean:
	rm -f *.o WSN log.txt *.out
