"""
FIT3143 Assignment 1 Proof of Concept
Topic: Physical and logical clocks
Lab Group: 6
Team No: 7
Members: Chan Kah Hee, Wong Jun Kang
"""
from mpi4py import MPI

class SK_Vector_Clock:
    """An efficient implementation of vector clocks using Singhal-Kshemkalyani's technique."""
    def __init__(self, comm: MPI.Intracomm):
        self.comm = comm
        self.size = comm.Get_size()
        self.rank = comm.Get_rank()
        self.timestamp = [0 for _ in range(self.size)]
        self.last_update = [0 for _ in range(self.size)]
        self.last_sent = [0 for _ in range(self.size)]
    
    def increment(self) -> None:
        """Tick for the clock"""
        self.timestamp[self.rank] += 1
        self.update_last_update(self.rank)

    def __str__(self) -> str:
        return (
            f"Node {self.rank}\t"
            f"LAMPORT_TIME \t= {self.timestamp}\n\t\t\t\t"
            f"LAST_UPDATE \t= {self.last_update}\n\t\t\t\t"
            f"LAST_SENT \t= {self.last_sent}\n"
        )

    def update_last_update(self, rank: int):
        """Update the Last Update vector"""
        self.last_update[rank] = self.timestamp[self.rank]

    def update_last_sent(self, rank: int):
        """Update the Last Sent vector"""
        self.last_sent[rank] = self.timestamp[self.rank]

    def update_timestamp(self, rank: int, timestamp: int) -> bool:
        """Update the timestamp if the given timestamp is greater than the current timestamp"""
        cond = timestamp > self.timestamp[rank]
        self.timestamp[rank] = timestamp if cond else self.timestamp[rank]
        return cond

    def event(self) -> None:
        """Event occurs"""
        self.increment()
        print(f"Event happened in {str(self)}", flush=True)

    def send_message(self, dest: int) -> None:
        """Send a message"""
        self.increment()

        # only last_sent[dest] < last_update[i] for all i, needs to be sent 
        timestamps = {(rank, timestamp) for rank, timestamp in enumerate(self.timestamp) 
                        if self.last_sent[dest] < self.last_update[rank]}
        self.comm.send(timestamps, dest=dest)

        self.update_last_sent(dest)
        print(f"Message sent from {str(self)}", flush=True)


    def recv_message(self, source: int) -> None:
        """Receive a message"""
        timestamps = self.comm.recv(source=source)
        self.increment()

        # for each timestamp in the set of timestamps, update current timestamp
        # and Last Update vector if necessary.
        for rank, timestamp in timestamps:
            if self.update_timestamp(rank, timestamp):
                self.update_last_update(rank)

        print(f"Message received at {str(self)}", flush=True)
        

def process_0(comm: MPI.Intracomm):
    clock = SK_Vector_Clock(comm)
    clock.event()
    clock.send_message(dest=1)
    clock.event()
    clock.recv_message(source=1)

def process_1(comm: MPI.Intracomm):
    clock = SK_Vector_Clock(comm)
    clock.recv_message(source=0)
    clock.send_message(dest=0)
    clock.event()

def main():
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()

    if rank == 0:
        process_0(comm)
    elif rank == 1:
        process_1(comm)

if __name__ == "__main__":
    main()

