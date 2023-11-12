# xv6


I have implemented PBS, where Priority is assigned on the following assumptions:

1. Lesser the dynamic priority value, lower the priority.
2. If two processes have the same dynamic priority, the process which has been scheduled lesser number of times will have higher priority.
3. If two processes have the same dynamic priority and have been scheduled the same number of times, the process which has been created earlier will have higher priority.

Each tick, in scheduler, I check search for the process with the most priority(least dynamic priority).
Having found that process, beofre trap is called, I increment the number of times its scheduled to 1 and reset its running time to 0.

Thus it is observed whenever procdump is called, then the process whose rtime is 0 is the process which was scheduled and is running. 

Formula for dynamic priority is as follows:

RBI = max((3 * RTime - STime - WTime) / (RTime + WTime + STime + 1) * 50, 0)

where R is the number of times the process has been scheduled, Time is the total time for which the process has been running since it was last scheduled, STime is the total time for which the process has been sleeping, WTime is the total time for which the process has been waiting in queue to be scheduled.

As can be observed,  initially when a process is allocated values in its struct its rbi is set to 25, however I observed that any process waits for atleast some ticks, causing its RBI to always choose the max between a negative number and 0. Therefore RBI is always 0 as RTime is set to 0 as soon as a process is scheduled the wait time and sleep time exceeds 3 * RTIme . After a longer period RTime definitely becomes 0 as wait time is always than 3*RTime.
Thus the dynamic priority of the process is always the static priority of the process.

If all processes are initially given same static prioirty, they all have same dynamic priority. So the process which is created first will always be scheduled first (assuming no processes have been scheduled yet), will run and then the scheduling will be equivalent to round robin scheduling.
However if all processes are initially given different static priorities using set_priority system call, then the process with the highest static priority will be scheduled first and will run till it is done or till it is blocked. Then the process with the next highest static priority will be scheduled and so on. So it will be equivalent to FCFS.
