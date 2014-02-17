/*
 * PriorityQueue.h
 *
 *  Created on: Feb 14, 2014
 *      Author: Michael DuBois
 */

#ifndef PRIORITYQUEUE_H_
#define PRIORITYQUEUE_H_

#include <stdexcept>
using std::runtime_error;
#include <memory>
using std::allocator;
#include <utility>
using std::swap;
#include <limits>
using std::numeric_limits;

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
	static const size_t MAX_ID = numeric_limits<size_t>::max();
	static const bool DEBUG = false;
};

/**
 * A Priority Queue implementation.
 */
// TODO could add alloc to template like STL containers. Awk w/ 3 arrays tho.
template<class T>
class PriorityQueue : PriorityQueueBase {
public:

	//--------------------------------------------------------------------------
	// INSTANTIATION / COPY SEMANTICS
	//--------------------------------------------------------------------------

	/// Constructor
	// TODO handle parameters == 0
	PriorityQueue(size_t initialCapacity=DEFAULT_INITIAL_CAPACITY,
				  size_t stepSize=DEFAULT_STEP_SIZE)
		: mItemsAllocator(),
		  mPrioritiesAllocator(),
		  mIdsAllocator(),
		  mInitialCapacity(initialCapacity),
		  mStepSize(stepSize),
		  mStepSize2x(2*mStepSize),
		  mCapacity(mInitialCapacity),
		  mSize(0),
		  mNumResizes(0)
	{
		// If the stepSize is zero, or the first resize would overflow size_t
		if(stepSize == 0 || stepSize > (MAX_ID - initialCapacity)) {
			throw out_of_range("Your `stepSize` is stupid.");
		}

		allocateArrays();

		if(PriorityQueueBase::DEBUG) {
			cout << "PriorityQueue created with capacity " << initialCapacity
				 << " and stepSize " << stepSize << endl;
		}
	}

	/// Copy Constructor
	PriorityQueue(PriorityQueue& src)
		: mItemsAllocator(),
		  mPrioritiesAllocator(),
		  mIdsAllocator(),
		  mInitialCapacity(src.mInitialCapacity),
		  mCapacity(src.mCapacity),
		  mSize(src.mSize),
		  mStepSize(src.mStepSize),
		  mStepSize2x(src.mStepSize2x),
		  mNumResizes(src.mNumResizes)
	{
		allocateArrays();

		// Copy values
		// arrays are related, so we can do it more efficiently than std::copy
		for(size_t i=0; i < mSize; i++) {
			mItems[i] = mItemsAllocator.construct(src.mItems[i]);
			mPriorities[i] = mPrioritiesAllocator.construct(src.mPriorities[i]);
			mIds[i] = mIdsAllocator.construct(src.mIds[i]);
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
		swap(first.mItemsAllocator, second.mItemsAllocator);
		swap(first.mPrioritiesAllocator, second.mPrioritiesAllocator);
		swap(first.mIdsAllocator, second.mIdsAllocator);
		swap(first.mCapacity, second.mCapacity);
	}

	/// Destructor
	virtual ~PriorityQueue() {
		destroyAllNodes();
		deallocateArrays();
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
			resize(mCapacity + mStepSize);
		}

		// Consolidate ids if necessary (rare occurrence)
		checkIdOverflow();

		// Insert the item at the end and swim it up to it's place
		size_t i = mSize;
		createNode(i, item, score, mNextId);
		mNextId++;
		mSize++;
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
		if(!empty()) {
			// Swap the root with the last element
			swapNodes(0, mSize - 1);
			destroyNode(mSize-1);
			mSize--;

			// Check if we need to resize
			checkCapacity();

			// And sink the new root to it's proper place.
			sink(0);

			// We do all this to avoid the alternative -- a ~n remove that shifts
			// all the elements in the array up.
		}
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
		destroyAllNodes();
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
	// Two arrays is more efficient than an array of node container objects.
	T* mItems;
	allocator<T> mItemsAllocator;
	int* mPriorities;
	allocator<int> mPrioritiesAllocator;
	size_t* mIds;
	allocator<size_t> mIdsAllocator;

	size_t mInitialCapacity;
	size_t mStepSize;
	size_t mStepSize2x; // cache this for performance
	size_t mCapacity;
	size_t mSize;
	size_t mNextId;
	int mNumResizes;


	void allocateArrays() {
		mItems = mItemsAllocator.allocate(mCapacity);
		mPriorities = mPrioritiesAllocator.allocate(mCapacity);
		mIds = mIdsAllocator.allocate(mCapacity);
	}

	void deallocateArrays() {
		mItemsAllocator.deallocate(mItems, mCapacity);
		mPrioritiesAllocator.deallocate(mPriorities, mCapacity);
		mIdsAllocator.deallocate(mIds, mCapacity);
	}

	void destroyAllNodes() {
		for(size_t i = 0; i < mSize; i++) {
			destroyNode(i);
		}
	}

	/**
	 * Destroys all objects associated with node `i`
	 */
	void destroyNode(size_t i) {
		mItemsAllocator.destroy(mItems+i);
		mPrioritiesAllocator.destroy(mPriorities+i);
		mIdsAllocator.destroy(mIds+i);
	}

	void createNode(size_t i, T item, int priority, size_t id) {
		mItemsAllocator.construct(mItems+i, item);
		mPrioritiesAllocator.construct(mPriorities+i, priority);
		mIdsAllocator.construct(mIds+i, id);
	}

	/**
	 * Swaps node `a` with node `b` in both the items and priorities array.
	 */
	void swapNodes(size_t a, size_t b) {
		swap(mItems[a], mItems[b]);
		swap(mPriorities[a], mPriorities[b]);
		swap(mIds[a], mIds[b]);
	}

	/**
	 * Set the size.
	 */
	void checkCapacity() {
		// remember size_t is unsigned
		// If we've shrunk enough, resize down to free memory
		if(mCapacity >= mStepSize2x && mSize < (mCapacity - mStepSize2x)) {

			//cout << mSize << " is less than " << (mCapacity - mStepSize2x) << endl;

			// Compute ideal newCapacity
			size_t newCapacity = mCapacity - mStepSize;
			//cout << "idealCapacity: " << newCapacity << endl;

			if(newCapacity >= mInitialCapacity) {
				resize(newCapacity);
			}
		}
	}

	/**
	 * Since `nextId` increases over the lifetime of the queue,
	 * irrespective of the current number of elements, it's possible for
	 * `nextId` to overflow. However, there will never be more than the
	 * max value of size_t elements in the queue, so we can consolidate
	 * ids in the event of an impending overflow.
	 */
	void checkIdOverflow() {
		// If the next increment of `nextId` would overflow
		// and we can consolidate
		if(mNextId == MAX_ID && mSize < MAX_ID) {
			consolidateIds();
		}
	}

	/**
	 * Consolidates the ids for all nodes into the range [0, mSize).
	 */
	void consolidateIds() {
		// TODO efficiently sort pointers to items in mIds in an aux array
		// then iterate through aux and assign 0 <= i < mSize
		throw runtime_error(
				string("PriorityQueue::consolidateIds() has been left") +
				string(" as an exercise to the reader.") );
	}

	void printContents() {
		cout << "Array contents: " << endl;
				for (size_t i = 0; i < mSize; i++)
				    cout << "\t #" << i << ": "
				    	 << *mItems[i] << " @" << mItems[i]
				    	 << "/"
				    	 << mPriorities[i] << endl;
	}

	/**
	 * Resizes the backing array if necessary.
	 */
	void resize(size_t newCapacity) {

		if(PriorityQueueBase::DEBUG) {
			cout << "RESIZING from " << mCapacity << " to " << newCapacity
					<< " with " << mSize << " items." << endl;
		}

		// Allocate new arrays
		T* newItems = mItemsAllocator.allocate(newCapacity);
		int* newPriorities = mPrioritiesAllocator.allocate(newCapacity);
		size_t* newIds = mIdsAllocator.allocate(newCapacity);

		// Copy values to new array and destroy original values.
		for(size_t i=0; i < mSize; i++) {
			mItemsAllocator.construct(newItems+i,mItems[i]);
			mPrioritiesAllocator.construct(newPriorities+i,mPriorities[i]);
			mIdsAllocator.construct(newIds+i, mIds[i]);
			destroyNode(i);
		}

		// Deallocate old arrays
		deallocateArrays();

		// update pointers to new arrays
		mItems = newItems;
		mPriorities = newPriorities;
		mIds = newIds;
		mCapacity = newCapacity;

		mNumResizes++;
	}

	/**
	 * Propagates a node **downward** to its proper place to reheapify the heap.
	 */
	void sink(size_t i) {
		bool heapified = false;
		while(!heapified) { // While node `i` is out of place

			// Assume node is correctly placed
			size_t destIdx = i;

			// Get child indexes
			size_t leftIdx = leftIdxOf(i);
			size_t rightIdx = rightIdxOf(i);

			// If the right child has greater priority
			if(greaterPriority(rightIdx, leftIdx)) {
				destIdx = rightIdx;
			} else if(greaterPriority(leftIdx, rightIdx)) {
				// if the left child has greater priority
				destIdx = leftIdx;
			}

			// If `i` has greatest priority (or `i` is a leaf)
			// (Remember leaf nodes return their own index for child indices)
			if(destIdx == i) {
				heapified = true;
			} else {
				swapNodes(destIdx, i);
				i = destIdx;
			}
		}
	}

	/**
	 * Propagates a node **upward** to its proper place to reheapify the heap.
	 */
	void swim(size_t i) {
		size_t parentIdx = parentIdxOf(i);

		// While `i` is not the root and `i`'s parent has lower priority
		while(parentIdx != i && greaterPriority(i, parentIdx)) {
			// Swap value at `i` with value at parent
			swapNodes(parentIdx, i);
			i = parentIdx;
			parentIdx = parentIdxOf(i);
		}
	}

	/**
	 * Returns true if `a` has greater priority than `b`
	 * TODO a max function could be more useful/efficient
	 */
	bool greaterPriority(size_t a, size_t b) {
		bool greaterPriority = mPriorities[a] > mPriorities[b];
		bool equalPriorityAndOlder =
				mPriorities[a] == mPriorities[b] && mIds[a] < mIds[b];
		return greaterPriority || equalPriorityAndOlder;
	}

	/**
	 * Returns the index of the parent of the node at `i` or `i` if no parent.
	 */
	size_t parentIdxOf(size_t i) const {
		// Remember, size_t is unsigned
		return (i > 0) ? (i - 1) / 2 : i;
	}

	/**
	 * Returns the index of the left child of the node at `i` or `i` if
	 * node `i` has no children.
	 */
	size_t leftIdxOf(size_t i) const {
		size_t idx = 2*i+1;
		return (idx < mSize) ? idx : i;
	}

	/*
	 * Returns the index of the right child of the node at `i` or `i` if
	 * node `i` has no children.
	 */
	size_t rightIdxOf(size_t i) const {
		size_t idx = 2*i+2;
		return (idx < mSize) ? idx : i;
	}
};


#endif /* PRIORITYQUEUE_H_ */
