#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <tuple>
#include <semaphore.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

//SHARED MEMORY/SEMAPHORES
static int **messages;
static int *tosses;
static int *epoch;
static int *message_sem1;

static int *ANS;
static int *CURRENT;
static int *NUM;

int *messageSent;
int *processorsFinished;

struct Processor
{
    int processorNumber;
    int groupNumber;
    bool faulty;
    int favoredAnswer;
};

void semWait(int *semaphore){
    while(*semaphore > 0){}
    return;
}

void receive(int *semaphore){
    semWait(semaphore);
}

void endEpoch(int *semaphore){
    semWait(semaphore);
}


int* create_shared_memory(size_t size){
    return (int*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
}

void store_toss(int TOSS, Processor processor, int groupSize){
    tosses[(processor.processorNumber - 1) - ((processor.groupNumber - 1) * groupSize)] = TOSS;
}

void adversary_broadcast(Processor processor, int numProcessors){
    for(int j = 0; j < numProcessors; j++){
        messages[processor.processorNumber - 1][j] = 0;   
    }
    messageSent[processor.processorNumber - 1] = 1;
}

void adversary(Processor processor, int numProcessors, int groupSize, int faultsTolerated)
{
    while(*epoch > 0){
        //trying to delay agreement as long as possible
        adversary_broadcast(processor, numProcessors);
        receive(message_sem1);

        if(processor.groupNumber == *epoch % (numProcessors/groupSize)){
            store_toss(0, processor, groupSize);
            }

        adversary_broadcast(processor, numProcessors);
        receive(message_sem1);

        sleep(1);
        if(*processorsFinished == (numProcessors - faultsTolerated)){
            break;
        }

    }
}

int tossCoin()
{
    return rand() % 2;
}

void initialize_processors(std::vector<Processor> &processors, int numProcessors, int groupSize, int testScenario)
{

    int numGroups = numProcessors / groupSize;
    Processor processor_initializer;
    int currentProcessor = 1;
    int groupNumber = 1;
    int addedToGroup = 0;

    for (int i = 1; i <= numProcessors; i++){
        processor_initializer.faulty = false;

        if(testScenario == 1 || testScenario == 2){
            processor_initializer.favoredAnswer = 1;
        }
        else if(testScenario == 3 || testScenario == 4){
            processor_initializer.favoredAnswer = tossCoin();
        }

        processor_initializer.processorNumber = i;
        
        if(groupNumber > numGroups)
            processor_initializer.groupNumber = -1;
        else
            processor_initializer.groupNumber = groupNumber;

        addedToGroup++;

        if(addedToGroup == groupSize){
            addedToGroup = 0;
            groupNumber++;
        }
        processors.push_back(processor_initializer);
    }
}

int group_to_hijack(int epochNumber, int numProcessors, int groupSize){
    return epochNumber % (numProcessors / groupSize);
}

void initialize_faulty_processors(std::vector<Processor> &processors, int faultsTolerated, int groupSize, int numProcessors, int testScenario)
{

    if (testScenario == 1 || testScenario == 3){

        int processorToModify = 0;
        int looper = 0;

        for (int i = faultsTolerated; i > 0; i--){
            processors[processorToModify].faulty = true;
            processors[processorToModify].favoredAnswer = 0;

            if (processorToModify + groupSize <= numProcessors)
                processorToModify = processorToModify + groupSize;
            else{
                looper++;
                processorToModify = looper;
            }
        }
    }
    else if(testScenario == 2 || testScenario == 4){
        int faultyInGroup = (groupSize / 2) + 1;
        int faultsAddedtoGroup = 0;
        int faultsAdded = 0;
        int epoch_attack = 1;
        int groupToAttack = group_to_hijack(epoch_attack, numProcessors, groupSize);

        for(int i = 0; i < numProcessors; i++){
            if(processors[i].groupNumber == groupToAttack){
                processors[i].faulty = true;
                processors[i].favoredAnswer = 0;
                faultsAddedtoGroup++;
                faultsAdded++;
            }
            if(faultsAddedtoGroup == faultyInGroup){
                epoch_attack++;
                groupToAttack = group_to_hijack(epoch_attack, numProcessors, groupSize);
                faultsAddedtoGroup = 0;
            }

            if(faultsAdded == faultsTolerated)
                break;
        }
    }
}


void broadcast(int **shared_memory, int VALUE, Processor processor, int numProcessors){
    
    for(int j = 0; j < numProcessors; j++){
        shared_memory[processor.processorNumber - 1][j] = VALUE;
    }
    messageSent[processor.processorNumber - 1] = 1;   
}


int count_received_messages(Processor processor, int numProcessors, int faultsTolerated){

    int numHeads = 0;
    int numTails = 0;

    for(int i = 0; i < numProcessors; i++){
        if(messages[i][processor.processorNumber - 1] == 1){
            numHeads++;
        }
        else if(messages[i][processor.processorNumber - 1] == 0){
            numTails++;
        }
    }

    if(numHeads >= (numProcessors - faultsTolerated)){
        return 1;
    }
    else if(numTails >= (numProcessors - faultsTolerated)){
        return 0;
    }
    else
        return -1; //stands in for "?" in original algorithm

}

std::tuple<int, int> most_frequent_message(Processor processor, int numProcessors){

    int numHeads = 0;
    int numTails = 0;

    for(int i = 0; i < numProcessors; i++){
        if(messages[i][processor.processorNumber - 1] == 1)
            numHeads++;
        else if(messages[i][processor.processorNumber - 1] == 0)
            numTails++;
    }
    
    if(numHeads > numTails)
        return std::make_tuple(1, numHeads);
    else
        return std::make_tuple(0, numTails);

}

int majority_toss(int groupSize)
{
    int numHeads = 0;
    int numTails = 0;

    for(int i = 0; i < groupSize; i++){
        if(tosses[i] == 1)
            numHeads++;
        else
            numTails++;
    }
    if(numHeads > numTails)
        return 1;
    else
        return 0;
}

void process_lock(int numProcessors){

    int processesCleared = 0;

    while(processesCleared < numProcessors){
        processesCleared = 0;
        for(int i = 0; i < numProcessors; i++){
            processesCleared = messageSent[i] + processesCleared;
        }
    }

    processesCleared = 0;

    for(int i = 0; i < numProcessors; i++){
        messageSent[i] = 0;
    }

    *message_sem1 = *message_sem1 - numProcessors;
    sleep(1);

    return;
}

void epoch_handler(std::vector<Processor> processors, int numProcessors, int faultsTolerated){
    
    while(*epoch > 0){

        process_lock(numProcessors);
        //FINISH ROUND ONE BROADCAST
        *message_sem1 = *message_sem1 + numProcessors;

        
        process_lock(numProcessors);
        //END OF ROUND 2 BROADCAST
        *message_sem1 = *message_sem1 + numProcessors;

        std::cout << "--------------------------------------------" << std::endl;
        std::cout << "               EPOCH " << *epoch << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << "        ANS    " << "NUM    " << "CURRENT" << std::endl;
        for(int i = 0; i < numProcessors; i++){
            if(processors[i].faulty)
                std::cout << "P" << i + 1 << ":      F      F      F" << std::endl;
            else
                std::cout << "P" << i + 1 << ":      " << ANS[i] << "      " << NUM[i] << "      " << CURRENT[i] << std::endl;
        }

        *epoch = *epoch + 1;
        sleep(1);

        if(*processorsFinished == (numProcessors - faultsTolerated))
            break;
    }

}


void Byzantine_Agreement(Processor processor, int numProcessors, int faultsTolerated, int groupSize)
{
    int index = processor.processorNumber - 1;
    CURRENT[index] = processor.favoredAnswer;// CURRENT <- INPUT      The value that processpr P currently favors as the answer of the Byzantine agreement algorithm                                    
    int TOSS;
    std::tuple<int, int> ANS_NUM;
    int currentEpoch;

    while (*epoch > 0) // for e = 1 to INFINITY do
    { 
        //ROUND 1 

            broadcast(messages, CURRENT[index], processor, numProcessors); // broadcast(epoch, 1, CURRENT); 
            receive(message_sem1);
            
            CURRENT[index] = count_received_messages(processor, numProcessors, faultsTolerated); // receive(e, 1, *) messages;

            if(processor.groupNumber == (*epoch % (numProcessors/groupSize))){
                TOSS = tossCoin();
                store_toss(TOSS, processor, groupSize);
            }
            else
                TOSS = 0;
                
        //ROUND 2

            broadcast(messages, CURRENT[index], processor, numProcessors);
            receive(message_sem1);

            ANS_NUM = most_frequent_message(processor, numProcessors);
            ANS[index] = std::get<0>(ANS_NUM);
            NUM[index] = std::get<1>(ANS_NUM); //tie defaults to 0

            if(NUM[index] >= (numProcessors - faultsTolerated)){
                *processorsFinished = *processorsFinished + 1;
                return;
            }
            else if(NUM[index] >= (faultsTolerated + 1)){
                CURRENT[index] = ANS[index];
            }
            else if((faultsTolerated + 1) > NUM[index]){
                CURRENT[index] = majority_toss(groupSize);
            }

           sleep(1);
    } 
}



int main(int argc, char **argv)
{
    std::string numProcessorsArg = argv[1];
    std::string groupSizeArg = argv[2];
    int numProcessors = stoi(numProcessorsArg);                        // n in paper
    int groupSize = stoi(groupSizeArg);                             // g in paper
    int faultsTolerated = (numProcessors - 1) / 3; // t in paper

    message_sem1 = create_shared_memory(sizeof(*message_sem1));
    epoch = create_shared_memory(sizeof(*epoch));
    
    ANS = new int[numProcessors];
    CURRENT = new int[numProcessors];
    NUM = new int[numProcessors];

    NUM = create_shared_memory(sizeof(*NUM));
    ANS = create_shared_memory(sizeof(*ANS));
    CURRENT = create_shared_memory(sizeof(*CURRENT));
    
    processorsFinished = create_shared_memory(sizeof(*processorsFinished));

    messageSent = new int[numProcessors];
    messageSent = create_shared_memory(sizeof(*messageSent));

    messages = new int*[numProcessors];
    messages = (int **)mmap(NULL, sizeof **messages, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    
    tosses = new int[groupSize];
    tosses = (int *)mmap(NULL, sizeof *tosses, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    for (int i = 0; i < numProcessors; i++){
        messages[i] = new int[numProcessors];
        messages[i] = (int *)mmap(NULL, sizeof *messages[i], PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    }

    int testScenario;
    for(int t = 0; t < 4; t++){

        testScenario = t + 1;
        *processorsFinished = 0;

        for(int i = 0; i < numProcessors; i++){
            messageSent[i] = 0;
        }
        std::cout << "TEST SCENARIO " << testScenario << std::endl;

        *message_sem1 = numProcessors;
        *epoch = 1;

        std::vector<Processor> processors;
        initialize_processors(processors, numProcessors, groupSize, testScenario);
        initialize_faulty_processors(processors, faultsTolerated, groupSize, numProcessors, testScenario);

        srand(time(NULL));

        pid_t pid;

        for(int i = 0; i < numProcessors + 1; i++){
        pid = fork();
            if(pid == 0){
                if(i == 0){
                    epoch_handler(processors, numProcessors, faultsTolerated);
                }
                else{
                    if(!processors[i - 1].faulty)
                        Byzantine_Agreement(processors[i - 1], numProcessors, faultsTolerated, groupSize);
                    else
                        adversary(processors[i - 1], numProcessors, groupSize, faultsTolerated);
                }
                _exit(0);
            }
        }
        for(int i = 0; i < numProcessors + 1; i++)
            wait(0);

    sleep(5);
    }
    return 0;
}