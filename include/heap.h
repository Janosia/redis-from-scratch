#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace std;

class HeapItem{
    public:
        uint64_t val = 0;
        size_t *ref = NULL;
};

void HeapUpdate(HeapItem *a, size_t pos, size_t len);
void HeapDelete(vector<HeapItem> &a, size_t pos);
void HeapUpsert(vector<HeapItem>&a, size_t pos, HeapItem  t);