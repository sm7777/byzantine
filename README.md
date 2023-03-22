# Randomized Byzantine Agreement

This program implements the Randomized Byzantine Agreement Algorithm as described in:

Chor, Benny and Brian A. Coan. “A Simple and Efficient Randomized Byzantine Agreement Algorithm.” IEEE Transactions on Software Engineering SE-11 (1985): 531-539.

## Running the Program

The program takes two arguments n and g

n is the number of processors and g is the size of each group

To run the program after compiling:
```
./byzantine n g
```

Code Overview

The code runs through four simulations:
1.	Test Scenario 1 sets the good processors to a uniform value and distributes faulty processors evenly throughout the groups.
2.	Test Scenario 2 sets the good processors to a uniform value but distributes the faulty processors in a methodology that attempts to delay the Byzantine Agreement algorithm from reaching consensus.
3.	Test Scenario 3 sets the good processors to a random value and distributes the faulty processors evenly throughout the groups.
4.	Test Scenario 4 sets the good processors to a random value and distributes the faulty processors in a methodology that attempts to delay the Byzantine Agreement algorithm from reaching consensus.

A structure which contains critical information for each processor is created. It houses whether the process is faulty, its number, its group number, and its initial answer to the Byzantine problem.

The processors are initialized through the use of a function called initialize_processors

initialize_processors sets the process number, group, faulty state, and group. Each process is added to a group based on the parameters outlined in the paper. Groups are numbered from 1 to n/g. Any leftover process is said to belong to no group, a placeholder value of -1 is given to these processors.

initialize_faulty_processors is used to pick which processors are deemed faulty for the scenario. 

In Test Scenario 1 and 3, the faulty processors are evenly distributed throughout the processor groups and their initial value is set to 0. 

In Scenarios 2 and 4 the goal is to maximize the disruption of the Byzantine Algorithm. Having a majority of faulty processors in the group which will perform the coin toss was chosen as the maximum disruptive mechanism. Having a majority of the processors in the group that flips the coin ensured the faulty processors control at least the first outcome. 

After the processors are initialized the program forks n + 1 times. A child process for each processor is created, and an additional process is created which helps managed process synchronization.

The epoch_handler is the function that helps manage process synchronization as well as keep track of the current epoch.

The good processors will continue to the Byzantine_Agreement algorithm and the fault processors will go to the adversary function.

Process synchronization and the data being passed between processes will be stored in shared memory. A number of shared memory variables are created:

messages – a two dimensional array which each process populates when broadcasting, they read from it when receiving

tosses – an array that stores the tosses of the group which performs the toss in each epoch

epoch – an integer that increments with each epoch

message_sem1 – a semaphore that synchronizes processes and makes sure all processes have executed sections of the code

ANS – an array that stores each process’s answer to the byzantine agreement

CURRENT – an array that stores each process’s CURRENT value

NUM – an array that stores each process’s count of the most occurring answer

messageSent – a flag that keeps track is the processes have broadcast their message

processorFinished – keeps track of home many processes have completed their execution

The Byzantine Agreement and Adversary

The Byzantine_Agreement function executes the Byzantine agreement algorithm as outlined in the paper. For every good processor, it starts with initializing CURRENT to the processor’s favored answer. The favored answer is broadcast. Broadcast is achieved by storing the value in an n x n matrix where the columns are the sent message of a process and the rows are the received messages. 
As each process broadcasts, the faulty processes are executing in the adversary function. The adversary function is also broadcasting into the same shared memory space but the message is pre-determined and never changes. As processes execute in the two functions, the epoch_handler is using a process_lock function which makes sure all n processes execute before allowing them to proceed. Once they have all broadcast their first message, they are released.
Each process now examines the shared memory space and looks at their respective messages received. They count the answers and if a majority value is present it becomes the new CURRENT for that process. If a significant majority is not found, -1 is given to the process in place of the “?” characters in the paper. 
Next, a coin flip is performed if the processor is in the group designate by the e % n/g. The coin flip is a done using the rand() function in C++ seeded by srand(). The toss result is stored in an array in shared memory of the size of a group.
A second broadcast is now performed by both the Byzantine algorithm and the adversary, once again storing the CURRENT value in the two dimensional messages array. The processes are synchronized in the epoch_handler and released once all messages are received. 
Another function most_frequent_message is used to determine which message is the most frequent and how many occurrences of it there were. These values are subsequently stored a ANS and NUM. 
The rest of the Byzantine Algorithm executes nearly identically to how it is outlined in the pseudocode. When the TOSS needs to be used, a function majority_toss is used to determine which toss outcome was the majority. When an agreement has been reached for the process it adds itself to the variable processorsFinished and returns.
The adversary and epoch_handler also end their execution once all the good processors have finished.

The program then loops back in the parent process and starts simulation of the next scenario until all four have been executed.

Results

The results of each scenario were achieved usually within 1-2 epochs of execution. In the case when the good processors had random inputs and the adversary was chosen to hinder performance, it took longer than any other scenario. The randomness in good processor initial value seemed to play the most significant role in performance. The consensus was reached fairly quickly in the other more straightforward scenarios.
The scenario in the paper (Table 1) looked to take longer than my simulation. The description of how they implemented the adversary seemed perhaps a bit more complex than the implementation achieved in this assignment and could be the source of the discrepancy.
