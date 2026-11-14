#include <YuEvents/YuPoll.h>

#include <utility>

// *************************************************************************************************

void
YuTimerHeap::add(YuTimerNode* node) {
    node->heapIndex = timers.size();
    timers.push_back(node);
    siftUp(node->heapIndex);
}

void
YuTimerHeap::remove(YuTimerNode* node) {
    size_t index = node->heapIndex;
    if (index == SIZE_MAX) {
        return;
    }

    size_t last = timers.size() - 1;
    if (index != last) {
        swap(index, last);
    }
    timers.pop_back();
    node->heapIndex = SIZE_MAX;
    if (index < timers.size()) {
        siftDown(index);
        siftUp(index);
    }
}

void
YuTimerHeap::siftUp(size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (timers[index]->deadlineNanoseconds >= timers[parent]->deadlineNanoseconds) {
            break;
        }
        swap(index, parent);
        index = parent;
    }
}

void
YuTimerHeap::siftDown(size_t index) {
    size_t count = timers.size();
    while (true) {
        size_t left = index * 2 + 1;
        size_t right = left + 1;
        size_t smallest = index;
        if (left < count &&
                timers[left]->deadlineNanoseconds < timers[smallest]->deadlineNanoseconds) {
            smallest = left;
        }
        if (right < count &&
                timers[right]->deadlineNanoseconds < timers[smallest]->deadlineNanoseconds) {
            smallest = right;
        }
        if (smallest == index) {
            break;
        }
        swap(index, smallest);
        index = smallest;
    }
}

void
YuTimerHeap::swap(size_t a, size_t b) {
    std::swap(timers[a], timers[b]);
    timers[a]->heapIndex = a;
    timers[b]->heapIndex = b;
}
