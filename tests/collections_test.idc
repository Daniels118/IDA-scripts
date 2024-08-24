// (c) 2024 Daniele Lombardi
//
// This code is licensed under GPL 3.0

#include <idc.idc>

#include "../scripts/collections.idc"

static run_Array_test() {
	msg("# Testing Array class\n");
	auto a = Array(10);
	a.add("b");
	a.add("d");
	a.add("a", 0);
	a.add("c", 2);
	assert(a.size() == 4, "wrong array size");
	assert(a[0] == "a", "wrong items order");
	assert(a[1] == "b", "wrong items order");
	assert(a[2] == "c", "wrong items order");
	assert(a[3] == "d", "wrong items order");
	a.remove(2);
	assert(a.size() == 3, "wrong array size after remove");
	assert(a[0] == "a", "wrong items after remove");
	assert(a[1] == "b", "wrong items after remove");
	assert(a[2] == "d", "wrong items after remove");
	msg("Passed\n\n");
}

static run_List_test(a) {
	msg("# Testing " + a.getClass() + " class\n");
	a.add("b");
	a.add("d");
	a.add("a", 0);
	a.add("c", 2);
	assert(a.size() == 4, "wrong list size");
	assert(a.get(0) == "a", "wrong items order");
	assert(a.get(1) == "b", "wrong items order");
	assert(a.get(2) == "c", "wrong items order");
	assert(a.get(3) == "d", "wrong items order");
	a.remove(2);
	assert(a.size() == 3, "wrong list size after remove");
	assert(a.get(0) == "a", "wrong items after remove");
	assert(a.get(1) == "b", "wrong items after remove");
	assert(a.get(2) == "d", "wrong items after remove");
	msg("Passed\n\n");
}

static run_Map_test(a) {
	msg("# Testing " + a.getClass() + " class\n");
	a.put("alpha", "v0");
	a.put("bravo", "v1");
	a.put("charlie", "v2");
	a.put("delta", "v3");
	a.put("echo", "v4");
	a.put("foxtrot", "v5");
	assert(a.size() == 6, "wrong map size");
	assert(a.get("alpha") == "v0", "wrong item value");
	assert(a.get("bravo") == "v1", "wrong item value");
	assert(a.get("charlie") == "v2", "wrong item value");
	assert(a.get("delta") == "v3", "wrong item value");
	assert(a.get("echo") == "v4", "wrong item value");
	assert(a.get("foxtrot") == "v5", "wrong item value");
	a.remove("charlie");
	assert(a.size() == 5, "wrong map size after remove");
	assert(a.hasKey("charlie") == 0, "item not removed");
	assert(a.hasKey("alpha") == 1, "missing item after remove");
	msg("Passed\n\n");
}

static run_collections_tests() {
	run_Array_test();
	run_List_test(LinkedList());
	run_Map_test(LinkedMap());
	run_Map_test(HashMap(3));
}

static main() {
	auto e;
	try {
		run_collections_tests();
	} catch (e) {
		print(e);
	}
}
