/*
 * PriorityQueue.h
 *
 *  Created on: Feb 14, 2014
 *      Author: Michael DuBois
 */

#ifndef PRIORITYQUEUE_H_
#define PRIORITYQUEUE_H_

#include <utility>
using std::swap;

// NOTES:
// - I've implemented the class in the header to simplify submission.
// - I've implemented a subset of the interface defined for the
//   std::priority_queue. This seemed more appropriate then directly
//   following the Java-style interface you listed in the hwk description.
// - I'm still getting used to pointer/reference/value and error handling.
//   Any tips or general rules you recommend would be most welcome.
/**
 * A base class for static members that don't depend on the template param
 */
struct PriorityQueueBase {
	// STATIC VARS
	static const size_t DEFAULT_INITIAL_CAPACITY = 30;
	static const size_t DEFAULT_STEP_SIZE = 10;
};

/**
 * A Priority Queue implementation.
 */
template<class T>
class PriorityQueue : PriorityQueueBase {
public:

	//--------------------------------------------------------------------------
	// INSTANTIATION / COPY SEMANTICS // TODO
	//--------------------------------------------------------------------------

	/// Constructor
	PriorityQueue(size_t initialCapacity=DEFAULT_INITIAL_CAPACITY,
				  size_t stepSize=DEFAULT_STEP_SIZE)
		: mInitialCapacity(initialCapacity),
		  mStepSize(stepSize),
		  mStepSize2x(2*mStepSize),
		  mCapacity(mInitialCapacity),
		  mSize(0),
		  mNumResizes(0),
		  mItemsAllocator(),
		  mPrioritiesAllocator()
	{
		cout << "PriorityQueue created with capacity " << initialCapacity
				<< " and stepSize " << stepSize << endl;

		allocateArrays();

		//mItems = new T[mCapacity];
		//mPriorities = new int[mCapacity];
	}

	/// Copy Constructor
	PriorityQueue(PriorityQueue& src)
		: mInitialCapacity(src.mInitialCapacity),
		  mCapacity(src.mCapacity),
		  mStepSize(src.mStepSize),
		  mStepSize2x(src.mStepSize2x),
		  mItemsAllocator(),
		  mPrioritiesAllocator()
	{
		allocateArrays();

		// Copy values
		// arrays are related, so we can do it more efficiently than std::copy
		for(size_t i=0; i < mSize; i++) {
			mItems[i] = src.mItems[i];
			mPriorities[i] = src.mPriorities[i];
		}
	}

	/**
	 * Copy assignment operator.
	 *
	 * Takes `rhs` by value, forcing a copy, then swaps.
	 * Adapted from http://stackoverflow.com/a/3279550/1599617
	 */
	PriorityQueue operator=(PriorityQueue rhs) {
		 swap(*this, rhs);
		 return *this;
	}

	/*  Move Constructor.
	 *
	 * Creates default object, then swaps `src` into it, leaving `src` barren.
	 * Adapted from http://stackoverflow.com/a/3279550/1599617
	 */
	PriorityQueue(PriorityQueue&& src)
		: PriorityQueue()
	{
		swap(*this, src);
	}

	/**
	 * Move assignment operator.
	 */
	PriorityQueue operator=(PriorityQueue&& rhs) {
		swap(*this, rhs);
		return *this;
	}

	/**
	 * Swaps two instances of this user-defined object.
	 *
	 * Adapted from  http://stackoverflow.com/a/3279550/1599617
	 */
	friend void swap(PriorityQueue& first, PriorityQueue& second) {
		using std::swap;
		// by swapping the members of two classes,
		// the two classes are effectively swapped
		swap(first.mSize, second.mSize);
		swap(first.mStepSize, second.mStepSize);
		swap(first.mStepSize2x, second.mStepSize2x);
		swap(first.mInitialCapacity, second.mInitialCapacity);
		swap(first.mItems, second.mItems);
		swap(first.mPriorities, second.mPriorities);
		swap(first.mCapacity, second.mCapacity);
	}

	/// Destructor
	virtual ~PriorityQueue() {
		//delete [] mItems;
		//delete [] mPriorities;
		for(size_t i = 0; i < mSize; i++) {
			mItemsAllocator.destroy(mItems + i);
			mPrioritiesAllocator.destroy(mPriorities + i);
		}
		mItemsAllocator.deallocate(mItems, mCapacity);
		mPrioritiesAllocator.deallocate(mPriorities, mCapacity);
	}

	//--------------------------------------------------------------------------
	// METHODS
	//--------------------------------------------------------------------------

	/**
	 * Inserts `item` of type `T` with priority `score`.
	 *
	 * If `getSize() + 1` exceeds `getCapacity()` at the time of insertion,
	 * a resize operation will occur, incurring memory allocation time.
	 */
	void insert(T item, int score) {
		// If we're full, resize up
		if(mSize == mCapacity) {
			resize(mSize + mStepSize);
		}

		// Insert the item at the end and swim it to the top
		size_t i = mSize;
		mItems[i] = item;
		mSize++; // increment size before swim for exception safety (?)
		swim(i);
	}

	/**
	 * Returns a constant reference to the element with the highest priority.
	 *
	 * Calling this function on an empty container causes undefined behavior.
	 */
	const T& top() const {
		return mItems[0];
	}

	/**
	 * Removes (and destroys) the element with the highest priority.
	 *
	 * You can access the element with `top()` before removing it to make
	 * a copy. Since the removed element is destroyed, it's best to use raw
	 * or smart pointers.
	 *
	 * If `getCapacity() - getSize() < 2*stepSize` at the time of removal,
	 * a resize operation will occur to shrink the backing data structure.
	 */
	void pop() {
		// Swap the root with the last element
		swapNodes(0, mSize - 1);
		mItemsAllocator.destroy(mItems + (mSize - 1));
		mPrioritiesAllocator.destroy(mPriorities + (mSize - 1));
		mSize--;

		checkSize();

		// And sink the new root to it's proper place.
		sink(0);

		// We do all this to avoid the alternative -- a ~n remove that shifts
		// all the elements in the array up.
	}

	/**
	 * Returns true if container is empty.
	 */
	bool empty() const {
		return (getSize() == 0);
	}

	/**
	 * Removes all items from the container.
	 */
	void clear() {
		mSize = 0;
		resize(mInitialCapacity);
	}

	/**
	 * Returns the number of elements in the container.
	 */
	const size_t getSize() const{
		return mSize;
	}

	/**
	 * Returns the capacity of the backing data structure.
	 */
	const size_t getCapacity() const{
		return mCapacity;
	}

	/**
	 * Returns the number of times the backing data structure has been resized.
	 */
	const int getNumResizes() const {
		return mNumResizes;
	}
private:
	T* mItems; // pointer to array of pointers
	int* mPriorities;
	/// Do not increment/decrement this! Use `setSize(newSize)`
	size_t mInitialCapacity;
	size_t mStepSize;
	size_t mStepSize2x; // cache this for performance
	size_t mCapacity;
	size_t mSize;
	int mNumResizes;
	std::allocator<T> mItemsAllocator;
	std::allocator<int> mPrioritiesAllocator;

	void allocateArrays() {
		//mItems = new T[mCapacity];
		//mPriorities = new int[mCapacity];
		mItems = mItemsAllocator.allocate(mCapacity);
		mPriorities = mPrioritiesAllocator.allocate(mCapacity);
	}

	/**
	 * Set the size.
	 */
	void checkSize() {
		// remember size_t is unsigned
		// If we've shrunk enough, resize down to free memory
		if(mCapacity > mStepSize2x && mSize < (mCapacity - mStepSize2x)) {
			cout << mSize << " is less than " << (mCapacity - mStepSize2x) << endl;

			// Compute ideal newCapacity
			size_t newCapacity = mCapacity - mStepSize;
			cout << "idealCapacity: " << newCapacity << endl;

			// But only resize if newCapacity is greater than initial capacity
			if(newCapacity >= mInitialCapacity) {
				resize(newCapacity);
			}
		}
	}

	/**
	 * Resizes the backing array if necessary.
	 */
	void resize(size_t newCapacity) {
		// Create new array and copy values
		T* newItems = mItemsAllocator.allocate(newCapacity);
		int* newPriorities = mPrioritiesAllocator.allocate(newCapacity);
		for(size_t i=0; i < mSize; i++) {
			newItems[i] = mItems[i];
			mItemsAllocator.destroy(mItems+i);
			newPriorities[i] = mPriorities[i];
			mPrioritiesAllocator.destroy(mPriorities+i);
		}

		// delete old array and swap new one in
		mItemsAllocator.deallocate(mItems, mCapacity);
		mPrioritiesAllocator.deallocate(mPriorities, mCapacity);
		//delete [] mItems;
		//delete[] mPriorities;

		mItems = newItems;
		mPriorities = newPriorities;

		// Update state
		mCapacity = newCapacity;
		mNumResizes++;
	}

	/**
	 * Moves a node **downward** to its proper place to reheapify the heap.
	 */
	void sink(size_t i) {
		bool heapified = false;
		while(!heapified) {
			// Check left, then right child for higher priorities
			if(mPriorities[leftIdxOf(i)] > mPriorities[i]) {
				swapNodes(leftIdxOf(i), i);
			} else if(mPriorities[rightIdxOf(i)] > mPriorities[i]) {
				swapNodes(rightIdxOf(i), i);
			} else {
				heapified = true;
			}
		}
	}

	/**
	 * Moves a node **upward** to its proper place to reheapify the heap.
	 */
	void swim(size_t i) {
		size_t parentIdx = parentIdxOf(i);

		// While not root and parent has lower priority
		while(parentIdx >= 0 && mPriorities[parentIdx] < mPriorities[i]) {
			// Swap value at `i` with parent
			swapNodes(parentIdx, i);

			// Increment
			i++;
			parentIdx = parentIdxOf(i);
		}
	}

	/**
	 * Swaps node `a` with node `b` in both the items and priorities array.
	 */
	void swapNodes(size_t a, size_t b) {
		swap(mItems[a], mItems[b]);
		swap(mPriorities[a], mPriorities[b]);
	}

	/**
	 * Returns the index of the parent of the node at `i`
	 */
	size_t parentIdxOf(size_t i) const {
		return (i - 1) / 2;
	}

	/**
	 * Returns the index of the left child of the node at `i`.
	 */
	size_t leftIdxOf(size_t i) const {
		return 2*i+1;
	}

	/*
	 * Returns the index of the right child of the node at `i`.
	 */
	size_t rightIdxOf(size_t i) const {
		return 2*i+2;
	}
};


#endif /* PRIORITYQUEUE_H_ */
