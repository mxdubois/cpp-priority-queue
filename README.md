cpp-priority-queue
==========

A toy implementation of a priority queue in C++. See "About the PriorityQueue" below for details.

Dependencies
------------
The project depends on C++11. GCC 4.8+ is recommended.

Build the sportsball example
------------------------------
I've included the makefile Eclipse generated for me. It is in the `Default` 
folder. From that directory, you should be able to build it with:

    make -f makefile

Running it
----------
You'll need to point the program to one of the datafiles in the `data`
directory. For example:

    sportsball ../data/sportsball1.txt [initialSize] [stepSize]

The `initialSize` is the initial size of the backing array. The `stepSize`
is the increment by which the array length is increased when it is filled.

About the PriorityQueue
-----------------------
A dynamically-resized priority queue implementation.

This priority queue is backed by a triad of heap-sorted arrays for
optimal performance.

It's recommended that one wrap objects in a smart pointer
(like `std::shared_ptr`) before inserting them into the queue.

Removal from the queue is a 3-step process (as with `std::priority_queue`)

    if(!myPriorityQueue.empty()) { // check that queue is not empty
        T myObject = myPriorityQueue.top(); // Copy top
        myPriorityQueue.pop(); // Remove and destroy
    }

This implementation uses `std::allocator` because

1. The STL containers use it
2. It seems to be the best way to destroy an arbitrary object without
       explicitly calling the objects destructor.

If you're working on this implementation, be careful to use the allocator
correctly. That is, use `allocator.construct(arrayPtr, obj)` (not assignment)
to place items into empty slots in the array. Then be sure to call
`allocator.destroy(arrayPtr+i)` on items before deallocating the array.
