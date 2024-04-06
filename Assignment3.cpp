#include <iostream>
#include <thread>
#include <queue>
#include <cmath>
#include <chrono>
#include <fstream>
#include <mutex>
#include <vector>
#include <random>
#include <deque>
#include <algorithm>
#include <chrono>

const int presents_total = 500000; //Test value
std::deque<int> remaining(presents_total);

enum op {
    ADD,
    REMOVE,
    SEARCH
};

std::mutex linkedlock;
std::mutex giftarraylock;
std::mutex sizelock;
std::random_device rd;
std::mt19937 r(rd());

class Gift {
public:
    int giftno;
    Gift *prev;
    Gift *next;
    bool in_list;

    Gift(int no):
        giftno(no),
        prev(nullptr),
        next(nullptr),
        in_list(false){}
};

std::vector<Gift> in_linked;

class LinkedList {
public:
    int size;

    LinkedList():
        head(new Gift(-1)),
        size(0){};

    //Finds the first id in the list less than giftno,
    //counting down from giftno. Inserts itself after it.
    void insert_sorted(int giftno) {
        Gift& start = in_linked[giftno];
        Gift* go = head;
        while (go->next != nullptr) {
            if (go->next->giftno > giftno) {
                break;
            }
            go = go->next;
        }
        start.prev = go;
        start.prev->next = &start;
        start.next = go->next;
        if (start.next != nullptr) {
            start.next->prev = &start;
        }
    }

    //Returns the giftno at the nth link in the list
    int get_nth_gift(int n)
    {
        int res = -1;
        Gift* go = head->next;
        for (int i = 0; i <= n; i++) {
            if (go == nullptr) {
                break;
            }
            res = go->giftno;
            go = go->next;
        }
        return res;
    }

    bool operate(op operation, int giftno)
    {
        std::lock_guard<std::mutex> lock(linkedlock);
        if (operation == ADD)
        {
            std::cout << "Adding id" << giftno << " to the list!" << std::endl;
            Gift& gift = in_linked[giftno];
            if (gift.in_list) {
                std::cout << "Failure: Gift #" << giftno << " already in list!" << std::endl;
                return false; //This gift is already in the list
            }
            gift.in_list = true;
            insert_sorted(giftno);
            std::cout << "Added id" << giftno << " to the list!" << std::endl;
            return true; //Successful
        }
        else if (operation == REMOVE)
        {
            std::cout << "Removing gift at node " << giftno << " from the list!" << std::endl;
            //In this context, giftno is a rand dist from 0-size, inclusive
            int no = get_nth_gift(giftno);
            if (no < 0) {
                std::cout << "Failure: Weird Case!" << std::endl;
                return false; //Weird case
            }
            Gift& gift = in_linked[no];
            if (!gift.in_list) {
                std::cout << "Failure: Gift #" << no << " not in list!" << std::endl;
                return false; //This gift isn't even in the list
            }
            gift.prev->next = gift.next;
            if (gift.next != nullptr) {
                gift.next->prev = gift.prev;
            }
            gift.in_list = false; //I'll delete all these from memory when problem1 is done
            std::cout << "Removed gift #" << no << " from the list!" << std::endl;
            return true; //Successful
        }
        else
        {
            Gift& gift = in_linked[giftno];
            std::cout << "Searching for  " << giftno << " in the list!" << std::endl;
            return gift.in_list;
        }
    }

    int getSize()
    {
        std::lock_guard<std::mutex> lock(sizelock);
        return size;
    }

    void addSize()
    {
        std::lock_guard<std::mutex> lock(sizelock);
        size++;
    }

    void subSize()
    {
        std::lock_guard<std::mutex> lock(sizelock);
        size--;
    }
private:
    Gift* head;
};

int next_remaining() {
    std::lock_guard<std::mutex> lock(giftarraylock);
    int front = remaining.front();
    remaining.pop_front();
    return front;
}

int check_remaining()
{
    std::lock_guard<std::mutex> lock(giftarraylock);
    return remaining.size();
}

void problem1ThreadBehavior(int threadId, LinkedList* ll)
{
    std::mt19937 mt(rd());
    while (check_remaining() || ll->getSize())
    {
        int choice = 0;
        std::uniform_int_distribution<int> dist(0, 2);
        choice = dist(mt);
        bool res;
        if (choice == 0) {
            res = ll->operate(ADD, next_remaining());
            if (res) {
                ll->addSize();
            }
            else {
                //bad
            }
        }
        else if (choice == 1) {
            std::uniform_int_distribution<int> disto(0, ll->getSize());
            res = ll->operate(REMOVE, disto(mt));
            if (res) {
                ll->subSize();
            }
            else {
                //bad
            }
        }
        else {
            std::uniform_int_distribution<int> disto(0, presents_total-1);
            if (ll->operate(SEARCH, disto(mt))) {
                //good
            }
            else {
                //bad
            }
        }
    }

}

void problem1()
{
    for (int i = 0; i < presents_total; i++) {
        remaining.push_back(i);
        in_linked.emplace_back(i);
    }
    LinkedList* ll = new LinkedList();
    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(remaining), std::end(remaining), rng);
    std::cout << "Starting problem 1!" << std::endl;
    std::cout << "To be honest, problem 1 just stalls forever. I ran out of time to debug it." << std::endl;
    /*
    std::vector<std::thread> threads;
    for (int i = 1; i <= 4; i++) {
        threads.emplace_back(problem1ThreadBehavior, i, ll);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for(int j = 0; j < in_linked.size(); j++) {
        delete &in_linked[j];
    }

    delete ll;
    */
}

class Report {
public:
    int** threadTemps;
    std::priority_queue<int> hottest;
    std::priority_queue<int, std::vector<int>, std::greater<int>> coldest;

    Report() {
        threadTemps = new int*[8];
    }

    //Takes the arrays of temp data and the max/min heaps from each thread
    void addQueueAndRankings(int id, int* temps, std::priority_queue<int>* ranks)
    {
        threadTemps[id] = temps;
        //Sort hottest temps
        for (int i = 0; i < 5; i++) {
            hottest.push(ranks->top());
            ranks->pop();
        }
        //Sort coldest temps
        for (int j = 5; j < 60; j++) {
            ranks->pop();
            if (j >= 55) {
                coldest.push(ranks->top());
            }
        }
    }

    void generateReport()
    {
        std::cout << "Hottest temperatures:" << std::endl;
        for (int i = 0; i < 5; i++) {
            std::cout << hottest.top() << "F" << std::endl;
            hottest.pop();
        }
        std::cout << "Coldest temperatures:" << std::endl;
        for (int j = 0; j < 5; j++) {
            std::cout << coldest.top() << "F" << std::endl;
            coldest.pop();
        }

        //For each ten minute interval in the data, get the largest and smallest temperature among each thread's
        //entry for that minute, and compare it against the respective largest and smallest temperature 10 minutes later.
        //The greatest absolute difference between the two is saved and compared against the running lead.
        int start = 0;
        int stop = 9;
        int biggestStart = 0;
        int biggestStop = 0;
        int biggestDiff = 0;
        while (stop < 60) {
            int smallest1 = threadTemps[0][start];
            int largest1 = smallest1;
            int smallest2 = threadTemps[0][stop];
            int largest2 = smallest2;

            for(int i = 0; i < 8; i++) {
                int x = threadTemps[i][start];
                int y = threadTemps[i][stop];
                if (x < smallest1) smallest1 = x;
                if (x > largest1) largest1 = x;
                if (y < smallest2) smallest2 = y;
                if (y > largest2) largest2 = y;
            }
            int res1 = std::abs(largest2 - smallest1);
            int res2 = std::abs(largest1 - smallest2);
            if (res1 > biggestDiff || res2 > biggestDiff)
            {
                biggestStart = start;
                biggestStop = stop;
                res1 > res2 ? biggestDiff = res1 : biggestDiff = res2;
            }
            start++;
            stop++;
        }
        std::cout << "Largest 10-minute interval difference:" << std::endl;
        std::cout << "Start minute: " << biggestStart+1 << ", Stop minute: " << biggestStop+2 << ", Difference: " << biggestDiff << std::endl;
    }
};

//Each thread tracks its minutes independently. At the end of the hour, it forwards its data to the report.
void problem2ThreadBehavior(int id, Report* report)
{
    std::priority_queue<int>* rankings = new std::priority_queue<int>;
    int* records = new int[60];
    int minute = 0;
    std::random_device rand;
    std::mt19937 randng(rand() + id + std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> rng(-100, 70);
    while (minute < 60) {
        int temp = rng(randng);
        records[minute] = temp;
        rankings->push(temp);
        minute++;
    }
    report->addQueueAndRankings(id, records, rankings);
}

//Instantiate threads and report
void problem2()
{
    std::cout << "Starting problem 2!" << std::endl;
    std::vector<std::thread> threads;
    Report* report = new Report();
    for (int i = 0; i < 8; i++) {
        threads.emplace_back(problem2ThreadBehavior, i, report);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    report->generateReport();
}

int main()
{
    problem1();
    problem2();
    return 0;
}
