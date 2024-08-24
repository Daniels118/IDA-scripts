// (c) 2024 Daniele Lombardi
//
// This code is licensed under GPL 3.0

/**Custom importer for types/functions/globals in JSON format. See here for reference:
 * https://github.com/openblack/bw1-decomp/tree/main
 *
 * FEATURES:
 * - handles circular references
 * - automatically define missing types as dummy structures
 *
 * TODO:
 * - assign types to global vars
 * - check defined struct size
 *
 * LIMITATIONS:
 * - can't set function parameter types
 * - the type name "Field" is reserved, must be renamed
 */

#include <idc.idc>

#include "json.idc"

static name_is_struct(name) {
	return get_struc_id(name) != -1;
}

static parse_decl2(name, code) {
	if (parse_decls(code, PT_REPLACE) != 0) {
		auto tinfo = get_tinfo(name);
		auto isGlobal = tinfo != 0 && get_named_type_tid(name) == BADADDR;
		if (isGlobal) {
			throw sprintf("Type %s is a global type and can't be redefined. Please rename it.", name);
		}
		if (tinfo != 0) {
			auto oldKind = getNthWord(tinfo.print(PRTYPE_TYPE), 0);
			if (oldKind == name) {
				if (name_is_struct(name)) {
					oldKind = "struct";
				} else {
					oldKind = "?";
				}
			}
			auto newKind = getNthWord(code, 0);
			if (newKind != oldKind) {
				throw sprintf("Type %s is already defined as '%s' and cannot be redefined as '%s'.", name, oldKind, newKind);
			}
		}
		return 0;
	}
	return 1;
}


class JStructMember {
	JStructMember() {
		this.type = "";
		this.name = "";
		this.offset = "";
	}
	
	getClass() {
		return "JStructMember";
	}
	
	toString() {
		auto type = this.type;
		auto name = this.name;
		//Check if member type is a function
		auto p = strstr(type, "*)(");
		if (p >= 0) {
			return substr(type, 0, p + 1) + name + substr(type, p + 1, -1) + ";";
		} else {
			//Move array size after field name
			p = strstr(type, "[");
			if (p >= 0) {
				name = name + substr(type, p, -1);
				type = substr(type, 0, p);
			}
			//Remove parentheses around pointer operators
			type = str_replace_first(type, "(*)", "*");
			//Remove type specifiers
			if (starts_with(type, "enum ")) {
				type = substr(type, 5, -1);
			} else if (starts_with(type, "struct ")) {
				type = substr(type, 7, -1);
			} else if (starts_with(type, "union ")) {
				type = substr(type, 6, -1);
			}
			return type + " " + name + ";";
		}
	}
}

static JStructMember_allocator() {
	return JStructMember();
}

class JStructDecl {
	JStructDecl() {
		this.kind = "STRUCT_DECL";
		this.type = "";
		this.size = 0;
		this.members = LinkedList(JStructMember_allocator);
	}
	
	getClass() {
		return "JStructDecl";
	}
	
	toString() {
		auto code = this.type + " {\n";
		auto memberEntry = this.members.head;
		for (; memberEntry != 0; memberEntry = memberEntry.next) {
			auto member = memberEntry.val;
			code = code + "\t" + member.toString() + "\n";
		}
		return code + "};";
	}
}


class JUnionAlias {
	JUnionAlias() {
		this.type = "";
		this.name = "";
	}
	
	getClass() {
		return "JUnionAlias";
	}
	
	toString() {
		auto type = this.type;
		auto name = this.name;
		auto p = strstr(type, "[");
		if (p >= 0) {
			name = name + substr(type, p, -1);
			type = substr(type, 0, p);
		}
		return type + " " + name + ";";
	}
}

static JUnionAlias_allocator() {
	return JUnionAlias();
}

class JUnionDecl {
	JUnionDecl() {
		this.kind = "UNION_DECL";
		this.type = "";
		this.aliases = LinkedList(JUnionAlias_allocator);
	}
	
	getClass() {
		return "JUnionDecl";
	}
	
	toString() {
		auto code = this.type + " {\n";
		auto aliasEntry = this.aliases.head;
		for (; aliasEntry != 0; aliasEntry = aliasEntry.next) {
			auto alias = aliasEntry.val;
			code = code + "\t" + alias.toString() + "\n";
		}
		return code + "};";
	}
}


class JFunctionProto {
	JFunctionProto() {
		this.type = "";
		this.result = "";
		this.call_type = "";
		this.signature = "";
	}
	
	getClass() {
		return "JFunctionProto";
	}
	
	toString() {
		auto rett = this.result;
		if (starts_with(rett, "enum ")) {
			rett = substr(rett, 5, -1);
		}
		auto args = substr(this.signature, strlen(this.result), -1);
		auto p = strstr(args, " __attribute__");
		if (p >= 0) {
			args = substr(args, 0, p);
		}
		return sprintf("typedef %s (%s *%s)%s;", rett, this.call_type, this.type, args);
	}
}


class JConstant {
	JConstant() {
		this.name = "";
		this.value = 0;
	}
	
	getClass() {
		return "JConstant";
	}
	
	toString() {
		return sprintf("%s = %d,", this.name, this.value);
	}
}

static JConstant_allocator() {
	return JConstant();
}

class JEnumDecl {
	JEnumDecl() {
		this.kind = "ENUM_DECL";
		this.type = "";
		this.constants = LinkedList(JConstant_allocator);
	}
	
	getClass() {
		return "JEnumDecl";
	}
	
	toString() {
		auto code = this.type + "{\n";
		auto entry = this.constants.head;
		for (; entry != 0; entry = entry.next) {
			auto item = entry.val;
			code = code + "\t" + item.toString() + "\n";
		}
		return code + "};";
	}
}


class JFunction {
	JFunction() {
		this.return_type = "";
		this.decorated_name = "";
		this.undecorated_name = "";
		this.call_type = "";
		this.win_addr = 0;
		this.argument_names = LinkedList();
	}
	
	getClass() {
		return "JFunction";
	}
}

static JFunction_allocator() {
	return JFunction();
}


class JGlobal {
	JGlobal() {
		this.name = "";
		this.type = "";
		this.value = 0;
	}
	
	getClass() {
		return "JGlobal";
	}
}

static JGlobal_allocator() {
	return JGlobal();
}


class IDAJson {
	IDAJson() {
		//We use NullList because we're goint to process json in streaming mode
		this.types = NullList();
		this.functions = NullList(JFunction_allocator);
		this.globals = NullList(JGlobal_allocator);
	}
	
	getClass() {
		return "IDAJson";
	}
}


class IdaJsonProcessor {
	IdaJsonProcessor() {
		this.section = "";
		this.kind = "";
		this.field = "";
		this.deadq = LinkedList();
		this.totalEnumEntries = 0;
	}
	
	getClass() {
		return "IdaJsonProcessor";
	}
	
	processDeadQueue2() {
		auto entry = this.deadq.head;
		while (entry != 0) {
			auto nextEntry = entry.next;
			auto val = entry.val.first;
			if (val.kind == "STRUCT_DECL") {
				auto success = 0;
				auto memberEntry = val.members.head;
				for (; memberEntry != 0; memberEntry = memberEntry.next) {
					auto member = memberEntry.val;
					if (ends_with(member.type, "*")) {
						auto mDef = member.toString();
						auto tName = getNthWord(mDef, 0);
						auto tCode = "struct _tmpstruct { " + tName + "* field; };";
						if (parse_decl2("_tmpstruct", tCode) == 0) {
							auto mName = member.name;
							auto mCode = "struct " + tName + " {};";
							msg("# Type %s not found, will be defined as empty struct.\n", tName);
							if (parse_decl2(tName, mCode)) {
								success = 1;
							}
						}
					}
				}
				if (success) {
					auto type = val.type;
					auto name = getNthWord(type, 1);
					auto code = entry.val.second;
					msg("%s\n\n", code);
					if (!parse_decl2(name, code)) {
						throw sprintf("Failed to define type '%s'", name);
					}
					msg("# Type %s successfully defined.\n\n", name);
					this.deadq.removeEntry(entry);
				} else {
					throw "Failed to define local type " + name;
				}
				entry = nextEntry;
			}
		}
		if (this.deadq.size > 0) {
			throw sprintf("Failed to define %d types", this.deadq.size);
		}
	}
	
	processDeadQueue() {
		auto success = 1;
		auto i;
		for (i = 1; success; i++) {
			msg("### Retrying parse failed types (attempt %d)\n\n", i);
			success = 0;
			auto entry = this.deadq.head;
			while (entry != 0) {
				auto nextEntry = entry.next;
				auto type = entry.val.first.type;
				auto code = entry.val.second;
				auto name = getNthWord(type, 1);
				msg("%s\n\n", code);
				if (parse_decl2(name, code)) {
					msg("Type '%s' correctly defined at attempt %d.\n\n", name, i);
					success = 1;
					this.deadq.removeEntry(entry);
				} else {
					msg("Failed to define local type '%s', will retry later.\n\n", name);
				}
				entry = nextEntry;
			}
		}
	}
	
	enter(depth, type, key, pVal) {
		if (depth == 0) {
			this.section = key;
			msg("### %s ###\n\n", toupper(key));
		} else if (depth == 2) {
			this.field = key;
		}
		return 1;
	}
	
	postProcessType(val) {
		auto code;
		auto name;
		if (val.kind == "TYPEDEF_DECL") {
			auto tdef = val.get("typedef");
			if (substr(tdef, strlen(tdef) - 1, -1) == ")") {
				//msg("# WARNING: typedef %s looks as function prototype, attempt to recovery...\n\n", val.type);
				auto p = strstr(tdef, "(*)(");
				auto rett = substr(tdef, 0, p);
				auto args = substr(tdef, p + 3, -1);
				code = sprintf("typedef %s (*%s)%s;", rett, val.type, args);
			} else {
				code = sprintf("typedef %s %s;", tdef, val.type);
			}
			name = val.type;
		} else if (val.kind == "FUNCTIONPROTO") {
			code = val.toString();
			name = val.type;
		} else {
			code = val.toString();
			name = getNthWord(val.type, 1);
			if (val.kind == "ENUM_DECL") {
				this.totalEnumEntries = this.totalEnumEntries + val.constants.size;
			}
		}
		msg("%s\n\n", code);
		if (!parse_decl2(name, code)) {
			this.deadq.queue(Pair(val, code));
			auto emptyType = substr(code, 0, strstr(code, "{")) + "{};";
			if (parse_decl2(name, emptyType)) {
				msg("Type '%s' has been defined as empty placeholder, will retry later.\n\n", name);
			} else {
				throw sprintf("Type %s cannot be defined.", name);
			}
		}
	}
	
	/* This remove the characters C (constant memeber function) and F (member function) which are
	 * coded by c++filt.
	 */
	fixMangledName(name) {
		auto p0 = strpos(name, "__", 1);
		if (p0 >= 0) {
			//msg("Orig: %s\n", name);
			auto ls = "";
			p0 = p0 + 2;
			auto c = name[p0];
			while (isdigit(char(c))) {
				ls = ls + c;
				c = name[++p0];
			}
			auto cnlen = atol(ls);
			p0 = p0 + cnlen;
			c = name[p0];
			if (c == "C") {
				name = substr(name, 0, p0) + substr(name, p0 + 1, -1);
				c = name[p0];
				//msg("C removed\n");
			}
			if (c == "F") {
				name = substr(name, 0, p0) + substr(name, p0 + 1, -1);
				//msg("F removed\n");
			}
			//msg("New:  %s\n\n", name);
		}
		return name;
	}
	
	postProcessFunction(val) {
		auto uname = this.fixMangledName(val.undecorated_name);
		auto addr = val.win_addr;
		msg("%s %s = .text:%08x;\n", val.return_type, val.decorated_name, addr);
		if (set_name(addr, uname, 0)) {
			auto entry = val.argument_names.head;
			if (val.call_type == "__thiscall") {
				entry = entry.next;
			}
			auto offset = 4;
			auto i = 0;
			for (; entry != 0; entry = entry.next, offset = offset + 4, i++) {
				if (!define_local_var(addr, 0, sprintf("[ebp+%d]", offset), entry.val)) {
					msg("Failed to set name for argument %d: %s\n", i, entry.val);
				}
			}
		} else {
			msg("Failed to set function name: %s\n", val.decorated_name);
		}
	}
	
	postProcessGlobal(val) {
		msg("%s %s = .data:%08x;\n", val.type, val.name, val.value);
		if (!set_name(val.value, val.name, 0)) {
			throw "Failed to set global name: " + val.name;
		}
	}
	
	postProcess(val) {
		//msg("#DEBUG %s\n", getClass(val));
		if (this.section == "types") {
			this.postProcessType(val);
		} else if (this.section == "functions") {
			this.postProcessFunction(val);
		} else if (this.section == "globals") {
			this.postProcessGlobal(val);
		} else {
			throw "Unknown section: " + this.section;
		}
	}
	
	exit(depth, containerType, key, val, pContainer) {
		auto name;
		auto addr;
		auto type;
		auto code;
		auto p;
		if (depth == 2) {
			if (key == "kind") {
				this.kind = val;
				if (val == "STRUCT_DECL") {
					pContainer = JStructDecl();
				} else if (val == "UNION_DECL") {
					pContainer = JUnionDecl();
				} else if (val == "FUNCTIONPROTO") {
					pContainer = JFunctionProto();
				/*} else if (val == "TYPEDEF_DECL") {	//Can't use because member "typedef" is a reserved keyword
					pContainer = JTypedefDecl(); */
				} else if (val == "ENUM_DECL") {
					pContainer = JEnumDecl();
				}
			}
		} else if (depth == 1) {
			this.postProcess(val);
		} else if (depth == 0) {
			msg("\n");
			if (key == "types" && this.deadq.size > 0) {
				this.processDeadQueue();
				if (this.deadq.size > 0) {
					this.processDeadQueue2();
				}
			}
		}
		return 1;
	}
}


static main() {
	auto err;
	try {
		auto filename = ask_file(0, "*.json", "Open JSON file");
		if (filename != "") {
			msg("\nParsing file \"%s\"\n\n", filename);
			auto processor = IdaJsonProcessor();
			auto file = JsonStream(filename, processor);
			file.parse(IDAJson());
			file.close();
			msg("\nDONE!\n\n");
			msg("Total enum entries: %d\n\n", processor.totalEnumEntries);
		}
	} catch (err) {
		if (value_is_object(err)) {
			print(err);
		} else {
			msg("ERROR: %s\n", err);
		}
	}
}