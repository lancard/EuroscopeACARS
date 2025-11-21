#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
class ConcurrentQueue
{
private:
    queue<T> q;
    mutex mtx;

public:
    void Enqueue(T item)
    {
        lock_guard<mutex> lock(mtx);
        q.push(move(item));
    }

    T Dequeue()
    {
        lock_guard<mutex> lock(mtx);
        if (q.empty())
            return nullptr;
        string item = move(q.front());
        q.pop();
        return item;
    }
};