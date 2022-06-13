#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <signal.h>
#include <mutex>
#include <atomic>
#include <vector>

#define DEVICE_MAX_SLEEP_TIME 1000
#define GEN_MAX_SLEEP_TIME 500

using namespace std;

struct request{
	int group, type;
};

queue<request> q;
int capacity, n;

atomic_bool threadExit{ false };
mutex m;

// обработка ctrl+c
void handler(int sig) {
    threadExit = true;
}

void GenerateRequest() {
    srand(time(0));
    while (true) {
        m.lock();

        if (threadExit) break;

        if (q.size() == capacity) {
            m.unlock();
            continue;
        }

        q.push(request {
            rand() % n,
            rand() % 3
        });

        m.unlock();

        this_thread::sleep_for(
            chrono::milliseconds(1 + rand() % GEN_MAX_SLEEP_TIME)
            );
    }

    cout << "Terminating generator thread" << endl;
    m.unlock();
}

void ProcessRequest(int number, int group) {
    srand(time(0));
    while (true)
    {
        m.lock();

        if (threadExit) break;

        if (q.empty()) {
            //cout << "Device No. " << number << " is free" << endl;
            m.unlock();
            continue;
        }

        request r = q.front();
        if (r.group != group) {
            //cout << "Device No. " << number << " is free" << endl;
            m.unlock();
            continue;
        }

        q.pop();

        int sleepTime = 250 + rand() % DEVICE_MAX_SLEEP_TIME;

        cout << "Device No. " << number << "(group " << group + 1 << ")"
            << " \tis working with request of group " 
            << r.group + 1 << " and type " << r.type + 1
            << ".\tAwakening after " << sleepTime << "ms."
            << " Queue size: " << q.size() << endl;
        
        m.unlock();
        this_thread::sleep_for(
            chrono::milliseconds(sleepTime)
            );
    }

    cout << "Terminating device thread number " << number << endl;
    m.unlock();
}

int main() {
    signal(SIGINT, handler);
    srand(time(0));

    cout << "Enter queue capacity and 'n' of groups: ";
    cin >> capacity >> n;

    vector<thread> devices;
    vector<int> groupSize;

    // вводим размеры групп
    for (int i = 0; i < n; i++) {
        int size;

        cout << "Enter group No. " << i + 1 << " size: ";
        cin >> size;

        groupSize.push_back(size);
    }

    // создаем потоки приборов
    int number = 1;
    for (int i = 0; i < n; i++) {
        for(int j = 0; j < groupSize[i]; j++) {

            devices.push_back(
                thread(ProcessRequest, number++, i)
                );

        }
    }

    // создаем генератор
    devices.push_back(
        thread (GenerateRequest)
        );

    // дожидаемся конца выполнения потоков
    for(auto &t: devices) t.join();

    cout << "main thread out";
}