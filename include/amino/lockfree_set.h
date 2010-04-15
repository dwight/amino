/*
 * (c) Copyright 2008, IBM Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  Change History:
 *
 *  yy-mm-dd  Developer  Defect     Description
 *  --------  ---------  ------     -----------
 *  08-08-04  Hui Rui     N/A        Initial implementation
 */

#ifndef AMINO_LOCKFREE_SET_H
#define AMINO_LOCKFREE_SET_H

#include "ordered_list.h"
using namespace std;

namespace amino {
/**
 * @brief The node type of the lock-free set, which stores the element and its hash key in it.
 *
 * @author Hui Rui
 *
 * @tparam KeyType
 * 				Type of element stored in the set
 */
template<typename KeyType> class SetNode {
public:
	/*the element*/
	KeyType element;
	/*the hash key of the element */
	unsigned int key;

	/**
	 * @brief Constructor. construct a SetNode
	 */
	SetNode(const KeyType& elem, unsigned int k) :
		element(elem), key(k) {
	}

	/**
	 * @brief Constructor. construct a SetNode only with the hash key. The explicit keyword implies it is
	 * forbidden converting a int object to a SetNode object.
	 */
	explicit SetNode(unsigned int k) :
		key(k) {
		element = KeyType();
	}

	SetNode() {
	}

	const SetNode& operator=(const SetNode& node) {
		key = node.key;
		element = node.element;
        return node;
	}

	/* @brief Judge the result by both the element and the value of the hash key*/
	bool operator==(const SetNode<KeyType>& rightHand) {
		return this->key == rightHand.key && this->element == rightHand.element;
	}

	/* @brief Judge the result by both the element and the value of the hash key*/
	bool operator>=(const SetNode<KeyType>& rightHand) {
		if (this->key == rightHand.key)
			return this->element >= rightHand.element;
		return this->key >= rightHand.key;
	}
};

/**
 * @brief This is an implementation of lock-free hash set data structure, based on algorithm described in two
 * paper "Split-Ordered Lists - Lock-free Resizable Hash Tables" by Ori Shalev Tel Aviv University and
 * Nir Shavit Tel-Aviv University and Sun Microsystems Laboratories "High Performance Dynamic Lock-Free
 * Hash Tables and List-Based Set" by Maged M. Michael.
 *
 * The internal data structure is a single linked LockFreeOrderedList. Elements are sorted by "binary reversal"
 * of hash key of elements. Additionally, an array of dummy nodes is stored for allowing quick access to elements
 * in the middle of lock-free ordered list. Elements are wrapped by SetNode before stored into set.
 *
 * @author Hui Rui
 *
 * @tparam KeyType
 * 					Type of elements stored in the set
 */
template<typename KeyType> class Set {
private:
	/*default size of main array*/
	static const int DEFAULT_ARRAY_SIZE = 512;

	/*default size of segment*/
	static const int DEFAULT_SEGMENT_SIZE = 64;

	/*default minimal size of segment*/
	static const int MINIMAL_SEGMENT_SIZE = 8;

    /*default load factor*/
#define SET_MAX_LOAD 0.75

	/*A ordered list used for storing the SetNode*/
	OrderedList<SetNode<KeyType> > *oList;

	/*The main Array stores the address of some segments. and the segments store the address of the node in ordered list*/
	atomic<atomic<NodeType<SetNode<KeyType> > *> *> *mainArray;

	typedef NodeType<SetNode<KeyType> >* node_ptr;
	typedef atomic<atomic<node_ptr> *> array_t;

	/* The total count of elements( regular node)*/
	atomic<int> _count;

	/* The segment size */
	int segmentSize;

	/* The load factor */
	float loadFactor;

	/* The current max number of dummy nodes */
	atomic<int> _size;

	/* prevent the use of copy constructor and copy assignment*/
	Set(const Set&) {
	}
	Set& operator=(const Set&) {
	}
public:
	/**
	 * @brief Create a new set with explicitly specified expected size and load factor.
	 * The number of dummy nodes has a maximum of expectedSize This isn't a limitation
	 * of actual elements stored in set. But the average search number will increase
	 * if number of elements is bigger than expectedSize
	 *
	 * @param expectedSetSize The expected size of set
	 * @param expectedLoadFactor Average load factor. Number of dummy nodes will expand 2X if
	 * 							 the actual load factor is higher than this parameter.
	 */
	explicit Set(int expectedSetSize = DEFAULT_SEGMENT_SIZE
			* DEFAULT_ARRAY_SIZE, float expectedLoadFactor = SET_MAX_LOAD) {
		/*get the expected size of segment*/
		segmentSize = (getLargestValue(expectedSetSize / DEFAULT_ARRAY_SIZE))
				<< 1;
		if (segmentSize < MINIMAL_SEGMENT_SIZE)
			segmentSize = MINIMAL_SEGMENT_SIZE;

		loadFactor = expectedLoadFactor;

		_size = 2; /*the default number of dummy nodes*/
		_count = 0; /*the default number of elements is 0*/

		oList = new OrderedList<SetNode<KeyType> > ; /* construct an empty ordered list*/

		/* new an array. and initialize it with NULL which indicates it is uninitialized by dummy node*/
		mainArray = new array_t[DEFAULT_ARRAY_SIZE];
		for (int i = 0; i < DEFAULT_ARRAY_SIZE; ++i) {
			mainArray[i] = NULL;
		}

		/* initialize the first bucket of the segment*/
		SetNode<KeyType> dummy(0);
		node_ptr address = NULL;
		if ((address = oList->add_returnAddress(dummy, &oList->head)) != NULL) {
			set_bucket(0, address);
		}
		assert(address != NULL);
		assert(get_bucket(0).load(memory_order_relaxed) != NULL);
	}

	/**
	 * @brief Destructor, supposed to be called when its associated data structure is to be deleted
	 */
	~Set() {
		for (int i = 0; i < DEFAULT_ARRAY_SIZE; ++i)
			delete[] mainArray[i].load(memory_order_relaxed);
		delete[] mainArray;
		delete oList;
	}

	/**
	 * @brief Judge if the set is empty
	 *
	 * @return If the ordered list not stores the regular node, return true. else return false.
	 */
	bool empty() {
		return _count.load(memory_order_relaxed) == 0;
	}

	/**
	 * @brief Get the size of set
	 *
	 * @return
	 * 		The size of set
	 */
	unsigned int size() {
		return _count.load(memory_order_relaxed);
	}

	/**
	 * @brief Add the specified element into the set
	 *
	 * @param element
	 * 			The element to be added
	 * @return
	 * 			If the element inserted successful return true, else return false
	 */
	bool insert(const KeyType& element) {
		/*get the element's hash code*/
		int key = hash_function(element);
		/*get the right bucket by modulo _size*/
		unsigned int bucket = key % _size.load(memory_order_relaxed);
		assert(binary_reverse(binary_reverse(bucket)) == bucket);
		/*if the bucket is not initialized, initialize it*/
		if (get_bucket(bucket).load(memory_order_relaxed) == NULL) {
			initialize_bucket(bucket);
		}

		assert(get_bucket(bucket).load(memory_order_relaxed) != NULL);
		/*construct a set node with the element itself and a special key*/
		SetNode<KeyType> node = SetNode<KeyType> (element, regularKey(key));
		/*now the bucket must be initialized. get the address stored in it*/

		atomic<node_ptr> start = get_bucket(bucket);

		/*insert the node by ordered list's add method. if not successful, return false */
		if (!oList->add(node, &start)) {
			return false;
		}

		int old_size = _size.load(memory_order_relaxed);
		/*update the number of elements*/
		++_count;
		/*if the load factor be exceeded and the current _size smaller than the DEFAULT_ARRAY_SIZE, double the _size*/
		if (_count.load(memory_order_relaxed) / old_size > loadFactor && old_size
				< DEFAULT_ARRAY_SIZE * segmentSize)
			_size.compare_swap(old_size, 2 * old_size);
		return true;
	}

	/**
	 * @brief Remove a specified element by value
	 *
	 * @param element
	 * 			The element to be removed
	 * @return
	 * 			If the value exists in the list, remove it and return true. else return false.
	 */
	bool remove(const KeyType& element) {
		/*get the element's hash code*/
		int key = hash_function(element);
		/*get the right bucket by modulo _size*/
		unsigned int bucket = key % _size.load(memory_order_relaxed);

		/*if the bucket is not initialized, initialize it*/
		if (get_bucket(bucket).load(memory_order_relaxed) == NULL)
			initialize_bucket(bucket);
		assert(get_bucket(bucket).load(memory_order_relaxed) != NULL);

		/*construct a set node with the element itself and a special key*/
		SetNode<KeyType> node = SetNode<KeyType> (element, regularKey(key));
		/*now the bucket must be initialized. get the address stored in it*/
		atomic<node_ptr> start = get_bucket(bucket);

		/*remove the node by ordered list's remove method. if not successful, return false */
		if (!oList->remove(node, &start))
			return false;
		/*update the number of elements*/
		--_count;
		return true;
	}

	/**
	 * @brief Judge if a specified element is in the set
	 *
	 * @param element
	 * 			The element to be searched
	 * @return
	 * 			If the element exists in the list, return true. else return false
	 */
	bool search(const KeyType& element) {
		/*get the element's hash code*/
		unsigned int key = hash_function(element);
		/*get the right bucket by modulo _size*/
		unsigned int bucket = key % _size.load(memory_order_relaxed);
		/*if the bucket is not initialized, initialize it*/
		if (get_bucket(bucket).load(memory_order_relaxed) == NULL)
			initialize_bucket(bucket);
		assert(get_bucket(bucket).load(memory_order_relaxed) != NULL);

		/*construct a set node with the element itself and a special key*/
		SetNode<KeyType> node(element, regularKey(key));
		/*now the bucket must be initialized. get the address stored in it*/
		atomic<node_ptr> start = get_bucket(bucket);

		/*search the node by ordered list's search method. if succeeds, return true.else, return false */
		return oList->search(node, &start);
	}

private:
	/**
	 * @brief Get n's leftmost one bit specified value, if n==0, the value is 0
	 *
	 * @param n
	 * 			The specified number
	 * @return
	 * 			Return n's leftmost one bit specified value, if n==0, return 0
	 */
	unsigned int getLargestValue(unsigned int n) {
		n |= (n >> 1);
		n |= (n >> 2);
		n |= (n >> 4);
		n |= (n >> 8);
		n |= (n >> 16);
		return n - (n >> 1);
	}

	/**
	 * @brief Get the value by reversing the bits order of n
	 *
	 * @param n
	 * 			The number be reversed
	 * @return
	 * 			The value obtained by reversing order of n
	 */
	unsigned int binary_reverse(unsigned int n) {
		n = ((n & 0x55555555) << 1) | ((n >> 1) & 0x55555555);
		n = ((n & 0x33333333) << 2) | ((n >> 2) & 0x33333333);
		n = ((n & 0x0f0f0f0f) << 4) | ((n >> 4) & 0x0f0f0f0f);
		n = (n << 24) | ((n & 0xff00) << 8) | ((n >> 8) & 0xff00) | (n >> 24);
		return n;
	}

	/**
	 * @brief Get the hash key's reverse value
	 *
	 * @param key
	 * 			A number
	 * @return
	 * 			The key's binary reversed value
	 */
	unsigned int dummyKey(unsigned int key) {
		return binary_reverse(key);
	}

	/**
	 * @brief Set the highest bit of n to be one. then get its reverse value
	 *
	 * @param key
	 * 			A number
	 * @return
	 * 			Reverse value of (key | 0x80000000)
	 */
	unsigned int regularKey(unsigned int key) {
		return binary_reverse(key | 0x80000000);
	}

	/**
	 * @brief Get the parent's position by the bucket
	 *
	 * @param bucket
	 * 			The specified position
	 * @return
	 * 			The parent's position by the bucket
	 */
	unsigned int get_parent(unsigned int bucket) {
		return bucket - getLargestValue(bucket);
	}

	/**
	 * @brief Direct the pointer in the segment cell of the index bucket. The value assigned
	 * is the address of a new dummy node containing the dummy key bucket.
	 *
	 * @param bucket
	 * 			The index bucket
	 */
	void initialize_bucket(unsigned int bucket) {
		unsigned int parent = get_parent(bucket);
		/* if the parent is not initialized, this function is called recursively to initialize it*/
		if (get_bucket(parent).load(memory_order_relaxed) == NULL) {
			initialize_bucket(parent);
		}
		assert(get_bucket(parent).load(memory_order_relaxed) != NULL);

		SetNode<KeyType> dummy = SetNode<KeyType> (dummyKey(bucket));

		assert(binary_reverse(dummyKey(bucket)) == bucket);
		/*choose parent to be as close as possible to bucket in the list, but still preceding it*/
		atomic<node_ptr> start = get_bucket(parent);

		node_ptr address = oList->add_returnAddress(dummy, &start);
		/*insert the dummy node and get its address*/

		assert(oList->search(dummy, &start));

		set_bucket(bucket, address);
		assert(get_bucket(bucket).load(memory_order_relaxed) == address);
	}

	/**
	 * @brief Get the address stored in the cell specified by index bucket
	 *
	 * @param bucket
	 * 			The index
	 * @return
	 * 			The address stored in the specified cell
	 */
	atomic<node_ptr> get_bucket(unsigned int bucket) {
        int local_size = _size.load(memory_order_relaxed);
        bucket %= local_size;
        
		int segment = bucket / segmentSize;

		/*if the segment is not initialized, initialize it*/
		if (mainArray[segment].load(memory_order_relaxed) == NULL) {
			set_bucket(bucket, NULL);
		}
		return mainArray[segment].load(memory_order_relaxed)[bucket % segmentSize];
	}

	/**
	 * @brief Set an address to the cell specified by bucket
	 *
	 * @param bucket
	 * 			The index
	 * @param bead
	 * 			The specified address would be set
	 */
	void set_bucket(int bucket, node_ptr head) {
		int segment = bucket / segmentSize;

		/*if the segment is not initialized, initialize it*/
		if (mainArray[segment].load(memory_order_relaxed) == NULL) {
			atomic<node_ptr>* new_segment = new atomic<node_ptr> [segmentSize];/*allocate a new segment*/
			for (int i = 0; i < segmentSize; ++i) {/*initialize the segment*/
				new_segment[i] = NULL;
			}
			atomic<node_ptr> *UNINITIALIZE = NULL;
			/*set the address of new segment to the right place in array*/
			if (!mainArray[segment].compare_swap(UNINITIALIZE, new_segment))
				delete[] new_segment;
		}
		/*set the specified address to the right place*/
		if (head != NULL) {
			node_ptr BUCKET_UNINITIALIZE = NULL;
			mainArray[segment].load(memory_order_relaxed)[bucket % segmentSize].compare_swap(
					BUCKET_UNINITIALIZE, head);
			assert(get_bucket(bucket).load(memory_order_relaxed) == head);
		}
	}
};
}

/**
 * @brief Hash function
 *
 * @warning If the keyType is not in the types which is full specialized, user should offer
 * a hash function themselves. else a exception would be thrown.
 *
 * @tparam KeyType
 * 			The type of element
 * @param element
 * 			The specified object who wants to generate a hash key
 * @return
 * 			The hash key with int type.
 */
template<typename KeyType> int hash_function(KeyType element) {
	throw std::runtime_error("Need hash function");
}

/**
 * @brief full specialization for several built-in type
 */
template<> int hash_function<int> (int element) {
	return element;
}

template<> int hash_function<unsigned int> (unsigned int element) {
	return static_cast<int> (element);
}

template<> int hash_function<long> (long element) {
	return static_cast<int> (element);
}

template<> int hash_function<unsigned long> (unsigned long element) {
	return static_cast<int> (element);
}

template<> int hash_function<char> (char element) {
	return static_cast<int> (element);
}

template<> int hash_function<unsigned char> (unsigned char element) {
	return static_cast<int> (element);
}

template<> int hash_function<signed char> (signed char element) {
	return static_cast<int> (element);
}

template<> int hash_function<short> (short element) {
	return static_cast<int> (element);
}

template<> int hash_function<std::string> (std::string element) {
	unsigned long result = 0;
	for (string::iterator iter = element.begin(); iter != element.end(); ++iter)
		result = 5 * result + *iter;

	return static_cast<int> (result);
}

#endif /*LOCKFREE_SET_H*/
