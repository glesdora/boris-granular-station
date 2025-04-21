#ifndef DOUBLEBUFFER_H
#define DOUBLEBUFFER_H

#include <vector>
#include <atomic>

class DoubleBuffer {
public:
    DoubleBuffer(int size) : bufferA(size), bufferB(size), isBufferAActive(true), pointerArray{ bufferA.data() } {}

    float* const* getActiveBufferPointerArray() {
        pointerArray[0] = getActiveBuffer().data();
        return pointerArray;
    }

    std::vector<float>& getActiveBuffer() {
        return isBufferAActive ? bufferA : bufferB;
    }

    std::vector<float>& getInactiveBuffer() {
        return isBufferAActive ? bufferB : bufferA;
    }

    void swapBuffers() {
        isBufferAActive = !isBufferAActive;
    }

private:
    std::vector<float> bufferA;
    std::vector<float> bufferB;
    std::atomic<bool> isBufferAActive;
    float* pointerArray[1];
};

#endif // DOUBLEBUFFER_H