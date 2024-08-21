// (c) 2024 Daniele Lombardi
//
// This code is licensed under GPL 3

#include <idc.idc>

#include "polyfill.idc"

///This class implements an array with fixed capacity and variable size.
class Array {
	Array(capacity, size = 0, allocator = 0) {
		this.capacity = capacity;
		this.size = size;
		auto i;
		for (i = 0; i < capacity; i++) {
			this[i] = 0;
		}
		this._allocator = allocator;
	}
	
	~Array() {
		this.clear();
	}
	
	getClass() {
		return "Array";
	}
	
	clear() {
		this.resize(0);
	}
	
	resize(size) {
		if (size < 0) {
			throw "Invalid new size";
		} else if (size > this.capacity) {
			throw "New size is larger than capacity";
		}
		if (size < this.size) {
			auto i;
			for (i = size; i < this.size; i++) {
				this[i] = 0;
			}
		}
		this.size = size;
	}
	
	add(val) {
		auto index = this.size;
		this.resize(index + 1);
		this[index] = val;
	}
	
	remove(index) {
		if (index < 0 || index >= this.size) {
			throw "Index "+itoa(index, 10)+" out of bounds";
		}
		auto i;
		this.size--;
		for (i = index; i < this.size; i++) {
			this[i] = this[i + 1];
		}
		this[this.size] = 0;
	}
	
	push(val) {
		this.add(val);
	}
	
	pop() {
		auto val = this[this.size - 1];
		this.resize(this.size - 1);
		return val;
	}

}


///This class implements a zero capacity array which discards any value inserted without throwing exceptions.
class NullList : Array {
	NullList(allocator = 0) : Array(0, 0, allocator) {}
	
	add(val) {}
	
	push(val) {}
}


///This class is used by LinkedList to hold individual items.
class ListEntry {
	ListEntry(prev, next, val) {
		this.prev = prev;
		this.next = next;
		this.val = val;
	}
	
	getClass() {
		return "ListEntry";
	}
}


///Doubly linked list implementation
class LinkedList {
	LinkedList(allocator = 0) {
		this._class = "LinkedList";
		this.head = 0;
		this.tail = 0;
		this.size = 0;
		this._allocator = allocator;
	}
	
	~LinkedList() {
		this.clear();
	}
	
	getClass() {
		return "LinkedList";
	}
	
	clear() {
		while (this.head != 0) {
			this.removeEntry(this.head);
		}
	}
	
	add(val) {
		if (this.size == 0) {
			this.head = ListEntry(0, 0, val);
			this.tail = this.head;
		} else {
			auto entry = ListEntry(this.tail, 0, val);
			this.tail.next = entry;
			this.tail = entry;
		}
		this.size++;
	}
	
	get(index) {
		auto entry = this.head;
		for (; index > 0; index--) {
			entry = entry.next;
		}
		return entry.val;
	}
	
	set(index, val) {
		auto entry = this.head;
		for (; index > 0; index--) {
			entry = entry.next;
		}
		entry.val = val;
	}
	
	removeEntry(entry) {
		if (entry.prev != 0) {
			entry.prev.next = entry.next;
		} else {
			this.head = entry.next;
		}
		if (entry.next != 0) {
			entry.next.prev = entry.prev;
		} else {
			this.tail = entry.prev;
		}
		entry.prev = 0;
		entry.next = 0;
		entry.val = 0;
	}
	
	remove(index) {
		auto entry = this.head;
		for (; index > 0; index--) {
			entry = entry.next;
		}
		removeEntry(entry);
	}
	
	push(val) {
		this.add(val);
	}
	
	pop() {
		auto val = this.tail.val;
		this.removeEntry(this.tail);
		return val;
	}
}


///This class is a general purpose container for paired values. Actually serves as key-value pair for Map implementations.
class Pair {
	Pair(first, second) {
		this.first = first;
		this.second = second;
	}
	
	getClass() {
		return "Pair";
	}
}


/**This is a simple Map implementation based on LinkedList. It has poor performances, but preserves the insertion order.
 * The keys can be accessed both with get/set methods or as class members.
 */
class LinkedMap {
	LinkedMap(allocator = 0) {
		this._class = "LinkedMap";
		this._entries = LinkedList();
		this._size = 0;
		this._allocator = allocator;
	}
	
	~LinkedMap() {
		this.clear();
	}
	
	getClass() {
		return "LinkedMap";
	}
	
	clear() {
		this._entries.clear();
		this._size = 0;
	}
	
	__setattr__(attr, value) {
		if (attr[0] == "_") {
			setattr(this, attr, value);
		} else {
			this.put(attr, value);
		}
	}
	
	__getattr__(attr) {
		if (attr[0] == "_") {
			return getattr(this, attr);
		} else {
			return this.get(attr);
		}
	}
	
	getIndex(key) {
		auto entry = this._entries.head;
		auto i;
		for (i = 0; i < this._size; i++) {
			if (entry.val.first == key) {
				return i;
			}
			entry = entry.next;
		}
		return -1;
	}
	
	getEntry(key) {
		auto entry = this._entries.head;
		auto i;
		for (i = 0; i < this._size; i++) {
			if (entry.val.first == key) {
				return entry;
			}
			entry = entry.next;
		}
		return 0;
	}
	
	hasKey(key) {
		return this.getEntry(key) != 0;
	}
	
	put(key, val) {
		auto entry = this.getEntry(key);
		if (entry != 0) {
			entry.val.second = val;
		} else {
			this._entries.add(Pair(key, val));
			this._size++;
		}
	}
	
	get(key) {
		auto entry = this.getEntry(key);
		if (entry == 0) {
			throw "Key \"" + key + "\" not found";
		}
		return entry.val.second;
	}
	
	getOrDefault(key, dflt) {
		auto entry = this.getEntry(key);
		if (entry != 0) {
			return entry.val.second;
		}
		return dflt;
	}
	
	remove(key) {
		auto entry = this.getEntry(key);
		this._entries.removeEntry(entry);
	}
}


class HashMapIterator {
	HashMapIterator(map) {
		this.map = map;
		this.nextIndex = 0;
		this.entry = map._buckets[0].head;
		this.bucketIndex = 0;
	}
	
	hasNext() {
		return this.nextIndex < this.map._size;
	}
	
	next() {
		if (!this.hasNext()) {
			throw "No more entries";
		}
		while (this.entry == 0) {
			this.entry = this.map._buckets[this.bucketIndex++].head;
		}
		this.nextIndex++;
		auto result = this.entry.val;
		this.entry = this.entry.next;
		return result;
	}
}

/**Efficient Map implementation. The keys can be accessed both with get/set methods or as class members.
 * The number of buckets can be decided at creation time, but then it stays fixed.
 */
class HashMap {
	HashMap(nBuckets = 10, allocator = 0) {
		this._class = "HashMap";
		if (nBuckets < 1) throw "Invalid buckets count";
		this._buckets = Array(nBuckets);
		this._buckets.resize(nBuckets);
		auto i;
		for (i = 0; i < nBuckets; i++) {
			this._buckets[i] = LinkedList();
		}
		this._size = 0;
		this._allocator = allocator;
	}
	
	~HashMap() {
		this.clear();
		this._buckets.clear();
		this._buckets = 0;
	}
	
	getClass() {
		return "HashMap";
	}
	
	clear() {
		auto i;
		for (i = 0; i < this._buckets.size; i++) {
			this._buckets[i].clear();
		}
		this._size = 0;
	}
	
	__setattr__(attr, value) {
		if (attr[0] == "_") {
			setattr(this, attr, value);
		} else {
			this.put(attr, value);
		}
	}
	
	__getattr__(attr) {
		if (attr[0] == "_") {
			return getattr(this, attr);
		} else {
			return this.get(attr);
		}
	}
	
	getBucket(key) {
		auto index = hashcode(key) % this._buckets.size;
		return this._buckets[index];
	}
	
	getEntry(key) {
		auto bucket = this.getBucket(key);
		auto entry = bucket.head;
		auto i;
		for (i = 0; i < bucket.size; i++) {
			if (entry.val.first == key) {
				return entry;
			}
			entry = entry.next;
		}
		return 0;
	}
	
	hasKey(key) {
		return this.getEntry(key) != 0;
	}
	
	put(key, val) {
		auto entry = this.getEntry(key);
		if (entry != 0) {
			entry.val.second = val;
		} else {
			auto bucket = this.getBucket(key);
			bucket.add(Pair(key, val));
			this._size++;
		}
	}
	
	get(key) {
		auto entry = this.getEntry(key);
		if (entry == 0) {
			throw "Key \"" + key + "\" not found";
		}
		return entry.val.second;
	}
	
	getOrDefault(key, dflt) {
		auto entry = this.getEntry(key);
		if (entry != 0) {
			return entry.val.second;
		}
		return dflt;
	}
	
	remove(key) {
		auto entry = this.getEntry(key);
		auto bucket = this.getBucket(key);
		bucket.removeEntry(entry);
	}
	
	iterator() {
		return HashMapIterator(this);
	}
}
