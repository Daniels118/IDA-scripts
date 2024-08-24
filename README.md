# IDA-scripts

A collection of utility scripts written in *IDC* language to be used with *IDA Free*.
The scripts have been developed and tested using *IDA Free 8.4.240527*.

## Modules

### polyfill.idc

A collection of basic functions to simplify development of more complex ones.
It also provides a global variable that can be used to simulate null references.

### collections.idc

Provides a set of classes which implement the most common data structures, with an
API inspired by *Java* and *C++* classes.

- `Array` of fixed capacity and variable size;
- `LinkedList` (doubly linked);
- `HashMap` with fixed buckets.

### json.idc

A *JSON* parser which can be used in *streaming* or *DOM* mode.

### IDA_json_importer.idc

An importer for a database of symbols based on a custom *JSON* schema.
