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

    static bool compare (request lhs, request rhs) {
        return lhs.type < rhs.type;
    }
};

priority_queue<request, vector<request>, decltype(&request::compare)> q(&request::compare);
int capacity, n;

atomic_bool threadExit{ false };
mutex m;

// обработка ctrl+c
auto handler = [](int sig) { threadExit = true; };

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
            m.unlock();
            continue;
        }

        request r = q.top();
        if (r.group != group) {
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

    // создаем потоки приборов
    int number = 1;
    for (int i = 0; i < n; i++) {

        int size;
        cout << "Enter group No. " << i + 1 << " size: " << endl;
        cin >> size;

        for(int j = 0; j < size; j++) {

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