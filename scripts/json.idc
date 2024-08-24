// (c) 2024 Daniele Lombardi
//
// This code is licensed under GPL 3.0

#include <idc.idc>

#include "collections.idc"

static JsonInit() {
	PolyfillInit();
}

/**Helper class to process JSON files. It implements the following features:
 * - Input streaming
 * There is no need to load the whole file in memory for processing. This minimizes the memory usages.
 * - Stream and DOM processing
 * Supports both callback for stream processing of JSON nodes, and DOM building for post-processing.
 * Modes aren't mutually exclusive, you can decide which nodes should be discarded or feed into DOM.
 * Discarding nodes can be done dynamically in the streaming callback, or statically using NullList
 * containers.
 * - Static binding, dynamic binding, lazy binding or typeless
 * You can decide which class should be used to hold data for each node. By default the parser will use
 * LinkedList for arrays and HashMap for associative arrays. HashMap keys can be accessed as normal
 * class members. When initiating the parsing of a node you can optionally pass an instance of a class
 * of your choice to store the data. This can be done dinamically using the streaming callbacks, or
 * statically by specifying the item allocator on the collection.
 * Another option is to provide the object used to store data based on the value of a field within the
 * node. This is known as lazy binding, and can be implemented by replacing the default container in
 * the streaming callback.
 */
class JsonStream {
	JsonStream(filename, processor = 0) {
		JsonInit();
		this._class = "JsonStream";
		this.file = fopen(filename, "r");
		this.line = 1;
		this.col = 0;
		this.instr = 0;
		this.escape = 0;
		this.backBuffer = ' ';
		this.backCount = 0;
		this.processor = processor;
	}
	
	getClass() {
		return "JsonStream";
	}
	
	readRaw() {
		auto c = this.backCount > 0 ? this.backBuffer : fgetc(this.file);
		this.backCount = 0;
		this.col++;
		if (c == '\n') {
			this.line++;
			this.col = 0;
		}
		return c;
	}
	
	read() {
		auto c = this.readRaw();
		if (this.instr) {
			if (this.escape) {
				this.escape = 0;
			} else if (c == '\\') {
				this.escape = 1;
			} else if (c == '"') {
				this.instr = 0;
			}
		} else if (c == '"') {
			this.instr = 1;
		} else {
			while (isblank(c)) {
				c = this.readRaw();
			}
			if (c == '"') {
				this.instr = 1;
			}
		}
		return c;
	}
	
	unread(c) {
		if (c == '"') {
			this.instr = !this.instr;
		}
		this.backBuffer = c;
		this.backCount = 1;
		this.col = this.col - 1;
	}
	
	close() {
		fclose(this.file);
	}
	
	err(message) {
		throw sprintf("%s at line %d, col %d", message, this.line, this.col);
	}

	readNumber() {
		auto result = "";
		auto c = this.read();
		while (isdigit(c) || c == '.' || c == '-') {
			result = result + c;
			c = this.read();
		}
		this.unread(c);
		return atof(result);
	}
	
	readNull() {
		auto val = "null";
		auto i;
		for (i = 0; i < 4; i++) {
			auto c = "" + this.read();
			auto e = val[i];
			if (c != e) {
				this.err("JSE06: unexpected character '"+c+"', expected '"+e+"'");
			}
		}
		c = this.read();
		if (c >= 0 && c != ',' && c != ']' && c != '}') {
			this.err("JSE07: unexpected character '"+c+"', expected ',' or ']' or '}'");
		}
		this.unread(c);
		return nullobj;
	}

	readString() {
		auto result = "";
		auto escape = 0;
		auto c = this.read();
		while (c >= 0) {
			if (escape) {
				result = result + c;
				escape = 0;
			} else if (c == '\\') {
				escape = 1;
			} else if (c == '"') {
				break;
			} else {
				result = result + c;
			}
			c = this.read();
		}
		return result;
	}

	readArray(depth, arr) {
		//msg("JsonReadArray %d\n", depth);
		auto i = 0;
		auto val = 0;
		auto c = this.read();
		if (c == ']') return arr;
		this.unread(c);
		while (c >= 0 && c != ']') {
			if (this.processor != 0) {
				this.processor.enter(depth, "array", i, &val);
			}
			if (arr._allocator != 0) {
				val = arr.newItem();
			}
			val = this.readAny(depth + 1, val);
			auto addVal = 1;
			if (this.processor != 0) {
				addVal = this.processor.exit(depth, "array", i, val, &arr);
			}
			if (addVal) {
				arr.add(val);
			}
			val = 0;
			c = this.read();
			if (c != ',' && c != ']') {
				this.err("JSE01: Unexpected character '"+c+"', expected ',' or ']'");
			}
			i++;
		}
		return arr;
	}

	readObject(depth, obj) {
		//msg("JsonReadObject %d\n", depth);
		auto err;
		auto newContainer = obj;
		auto objIsMap = getClass(obj) == "HashMap";
		auto childContainer = 0;
		auto c = this.read();
		while (c >= 0 && c != '}') {
			if (c != '"') {
				this.err("JSE05: Unexpected character '"+c+"', expected '\"'");
			}
			auto key = this.readString();
			auto addKey = 1;
			if (this.processor != 0) {
				addKey = this.processor.enter(depth, "object", &key, 0);
			}
			c = this.read();
			if (c != ':') {
				this.err("JSE02: Unexpected character '"+c+"', expected ':' after key \"" + key + "\"");
			}
			if (addKey) {
				if (objIsMap) {
					childContainer = obj.getOrDefault(key, 0);
				} else {
					//msg("Using %s::%s\n", getClass(obj), key);
					try {
						childContainer = getattr(obj, key);
					} catch (err) {
						addKey = 0;
						childContainer = 0;
					}
				}
			}
			auto val = this.readAny(depth + 1, childContainer);
			if (addKey) {
				auto addVal = 1;
				if (this.processor != 0) {
					addVal = this.processor.exit(depth, "object", key, val, &newContainer);
				}
				if (addVal) {
					if (newContainer != obj) {
						//Lazy binding
						auto iter = obj.iterator();
						while (iter.hasNext()) {
							auto entry = iter.next();
							try {
								setattr(newContainer, entry.first, entry.second);
							} catch (err) {}
						}
						obj = newContainer;
						objIsMap = 0;
					}
					if (objIsMap) {
						obj.put(key, val);
					} else {
						setattr(obj, key, val);
					}
				}
			}
			c = this.read();
			if (c != ',' && c != '}') {
				this.err("JSE03: Unexpected character '"+c+"', expected ',' or '}'");
			}
			if (c == ',') {
				c = this.read();
			}
		}
		return obj;
	}

	readAny(depth, container = 0) {
		//msg("JsonRead %d\n", depth);
		if (depth > 20) {
			throw "Stack overflow";
		}
		auto c = this.read();
		if (c == '{') {
			if (container == 0) {
				container = HashMap();
			}
			return this.readObject(depth, container);
		} else if (c == '[') {
			if (container == 0) {
				container = LinkedList();
			}
			return this.readArray(depth, container);
		} else if (c == '"') {
			return this.readString();
		} else if (c == 'n') {
			this.unread(c);
			return this.readNull();
		} else if (isdigit(c) || c == '-') {
			this.unread(c);
			return this.readNumber();
		}
		this.err("JSE04: Unexpected character: " + c);
	}
	
	parse(container = 0) {
		return this.readAny(0, container);
	}
}

//Example processor class
/*
class MyProcessor {
	MyProcessor() {}
	
	///Called before processing a node.
	///For associative arrays, you can optionally rename the key by setting pKey.
	///You can optionally provide an instance to store node data by setting pVal.
	///Return 0 to discard the value after processing.
	enter(depth, type, pKey, pVal) {
		return 1;
	}
	
	///Called after processing a node.
	///You can optionally replace the container object by setting pContainer.
	///Return 0 to discard the value.
	exit(depth, type, key, val, pContainer) {
		return 1;
	}
}
*/
