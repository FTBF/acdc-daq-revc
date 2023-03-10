#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

template<typename T>
class BlockingQueue : public queue<T>
{
public:
    BlockingQueue() : queue<T>()
    {
    }

    void push(T&& data)
    {
        std::unique_lock<std::mutex> lock(mut_);
        
        queue<T>::push(data);
        block_.notify_all();
    }

    template <class... Args> 
    void emplace(Args&&... args)
    {
        std::unique_lock<std::mutex> lock(mut_);
        
        queue<T>::emplace(args...);
        block_.notify_all();
    }

    const T& front()
    {
        std::unique_lock<std::mutex> lock(mut_);
            
        if(queue<T>::size() <= 0)
        {
            block_.wait(lock);
        }
        
        return queue<T>::front();
    }

    void pop()
    {
        std::unique_lock<std::mutex> lock(mut_);

        if(queue<T>::size() <= 0)
        {
            block_.wait(lock);
        }

        //T data = std::move(queue<T>::front());
        queue<T>::pop();
        //return std::move(data);
    }

private:
    std::mutex mut_;
    std::condition_variable block_;
};
