// (c) 2024 Daniele Lombardi
//
// This code is licensed under GPL 3.0

///Call PolyfillInit() to initialize this module.

#include <idc.idc>

///Global object used to represent null values.
extern nullobj;

///Class used to represent the null value;
class NullObj {
	NullObj() {
		this._class = "NullObj";
	}
}

///Call this method to initialize this module.
static PolyfillInit() {
	if (!value_is_object(nullobj)) {
		nullobj = NullObj();
	}
}

static assert(cond, txt) {
	if (!cond) {
		msg("Assertion failed: %s\n", txt);
		throw txt;
	}
}

static function_exists(name) {
	return value_is_func(eval(name));
}

static get_function(name, dflt = 0) {
	auto val = eval(name);
	if (value_is_func(val)) {
		return val;
	}
	return dflt;
}

static starts_with(str, prefix) {
	auto l1 = strlen(str);
	auto l2 = strlen(prefix);
	if (l1 >= l2) {
		return substr(str, 0, l2) == prefix;
	}
	return 0;
}

static ends_with(str, suffix) {
	auto l1 = strlen(str);
	auto l2 = strlen(suffix);
	if (l1 >= l2) {
		return substr(str, l1 - l2, -1) == suffix;
	}
	return 0;
}

///Replace the first occurrence of a substring in a string.
static str_replace_first(haystack, needle, repl) {
	auto p = strstr(haystack, needle);
	if (p >= 0) {
		haystack = substr(haystack, 0, p) + repl + substr(haystack, p + strlen(needle), -1);
	}
	return haystack;
}

///Replace all occurrences of a substring in a string.
static str_replace(haystack, needle, repl) {
	auto result = "";
	auto p = strstr(haystack, needle);
	while (p >= 0) {
		result = result + substr(haystack, 0, p) + repl;
		haystack = substr(haystack, p + strlen(needle), -1);
		p = strstr(haystack, needle);
	}
	return result + haystack;
}

///Returns the position of a substring in a string, starting the search from a given index.
static strpos(haystack, needle, start = 0) {
	haystack = substr(haystack, start, -1);
	auto p = strstr(haystack, needle);
	if (p < 0) return p;
	return start + p;
}

///Return whether the given character is a space, a tab, a carrige return or a line feed.
static isblank(c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

///Returns whether the given character is an ASCII digit.
static isdigit(c) {
	auto code = ord(c);
	return '0' <= code && code <= '9';
}

///Converts the given string to a float or long value.
static atof(s) {
	auto neg = 0;
	if (s[0] == "-") {
		s = substr(s, 1, -1);
		neg = 1;
	}
	auto val;
	auto p = strstr(s, ".");
	if (p >= 0) {
		auto ipart = 1.0 * atol(substr(s, 0, p));
		auto dpart = 1.0 * atol(substr(s, p + 1, -1));
		auto ndec = strlen(dpart);
		for (; ndec > 0; ndec--) {
			dpart = dpart / 10.0;
		}
		val = ipart + dpart;
	} else {
		val = atol(s);
	}
	if (neg) val = -val;
	return val;
}

///Returns a long hashcode for the given string
static hashcode(value) {
	auto hash = 0;
	auto multiplier = 1;
	auto i;
	for (i = strlen(value) - 1; i >= 0; i--) {
		hash = hash + ord(value[i]) * multiplier;
		auto shifted = multiplier << 5;
		multiplier = shifted - multiplier;
	}
	return hash >= 0 ? hash : -hash;
}

/**Tries to call the method getClass() on the given object and returns the returned value.
 * If the given value isn't an object, return an empty string.
 * If the given value is an object which doesn't implement the getClass() method, return the string "object".
 */
static getClass(obj) {
	if (value_is_object(obj)) {
		auto err;
		try {
			return obj.getClass();
		} catch (err) {
			return "object";
		}
	}
	return "";
}

///Returns wheter the given value is a float or long.
static value_is_number(val) {
	return value_is_long(val) || value_is_float(val);
}

static getNthWord(s, n) {
	auto p1 = strstr(s, " ");
	if (p1 < 0) p1 = strlen(s);
	while (n-- > 0) {
		s = substr(s, p1 + 1, -1);
		p1 = strstr(s, " ");
		if (p1 < 0) p1 = strlen(s);
	}
	return substr(s, 0, p1);
}

static countWords(s) {
	auto n = 0;
	auto p1 = strstr(s, " ");
	if (p1 < 0) p1 = strlen(s);
	while (p1 > 0) {
		n++;
		s = substr(s, p1 + 1, -1);
		p1 = strstr(s, " ");
		if (p1 < 0) p1 = strlen(s);
	}
	return n;
}
