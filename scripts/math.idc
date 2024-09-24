// (c) 2024 Daniele Lombardi
//
// This code is licensed under GPL 3.0

#include <idc.idc>

static min(a, b) {
	if (a < b) return a;
	return b;
}

static max(a, b) {
	if (a > b) return a;
	return b;
}

static floor(value) {
	auto intpart = long(value);
	if (value < intpart) {
		intpart--;
	}
	return intpart;
}

static ceil(value) {
	auto intpart = long(value);
	if (value > intpart) {
		intpart++;
	}
	return intpart;
}
