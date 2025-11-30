#pragma once
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

class SimpleRandGen
{
public:
    SimpleRandGen() {
        srand(time(NULL));
    }
    unsigned long long get(int min, int max) {
        unsigned long long state = rand();
        return state % (max - min + 1) + min;
    }
};