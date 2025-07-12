#include <heap.h>

#include<iostream>
#include<vector>

using namespace std;

/*@brief calculate left child*/
size_t heap_left (size_t i)   { return i * 2 + 1; }
/*@brief calculate right child*/
size_t heap_right ( size_t i) { return i * 2 + 2; }
/*@brief calculate index of parent*/
size_t heap_parent( size_t i ){ return (i+1)/2-1; }

/*
@brief Swapping node with parent or grandparent, when value of node is lesser. The smaller is value is pushed upwards 
*/
void HeapUP(HeapItem *a, size_t pos){
    HeapItem t = a[pos];
    while(pos > 0 && a[heap_parent(pos)].val > t.val){
        a[pos] = a[heap_parent(pos)];
        pos = heap_parent(pos);
    }
    a[pos] = t;
}
/*@brief Swapping node with children or grandchildren, when value of node is greater. The bigger is value is pushed downwards*/
void HeapDOWN(HeapItem *a, size_t pos, size_t len){
    HeapItem t = a[pos];
    while(true){
        size_t left = heap_left(pos), right = heap_right(pos);
        size_t min_pos = pos; 
        uint64_t min_val = t.val;

        if(left < len && a[left].val < t.val){
            min_pos = pos;
            min_val = a[left].val;
        }
        if(right < len && a[right].val < t.val){
            min_pos = pos;
        }

        if(min_pos == pos){
            break;
        }

        a[pos] = a[min_pos];
        pos = min_pos;
    }
    a[pos] = t;
}
/*@brief If value of parent node becomes greater than child node, it violates a invariant for min-heap.
Time Complexity : O(logN)
@param a  heap item where invariant might be violated
@param pos  position/index of node responsible for violation
@param len length/size of heap*/
void HeapUpdate(HeapItem *a, size_t  pos , size_t len){
    if(pos > 0 && a[pos].val < a[heap_parent(pos)].val){
        HeapUP(a, pos);
    } else {
        HeapDOWN(a, pos, len);
    }
}

/*@brief  Delete a node from Heap. Swap with last item and then delete last item. Time Complexity : O(1)*/
void HeapDelete(vector<HeapItem> &a, size_t pos){
    a[pos] = a.back();
    a.pop_back();
    if(pos < a.size()){
        HeapUpdate(a.data(), pos, a.size());
    }
}
