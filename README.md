## IHV - "Interactive Heap Visualizer" (not quite yet)
Tracks heap changes (idea based on [villoc](https://github.com/wapiflapi/villoc) from [@wapiflapi](https://twitter.com/wapiflapi))

Uses PIN to track heap changes and stores the information in a MySQL database. Which can be used to visualize the data like @wapiflapi did with villoc.

This tool is in an *early stage*, there is no visualization yet.


#### Snapshot generation
IHV creates 'snapshots' each time malloc, free, calloc and realloc is called.
An additional intermediate mem-write snapshot is added when the application overwrites some data on the heap which has not been saved yet by a previous snapshot.
This makes it easy to track all write operations on the heap, as you can exactly determine which write has been caused by e.g. a malloc.

#### Data storage
All snapshots are stored in the snapshot table, which shows which event occurred at a certain time and references further details about the event.
Malloc calls for instance have an additional entry in the reason_malloc table with the parameter and return value of the function.

**Note:** The size values show the user specified size, not the actual occupied area.

All written bytes to the heap (address + value pairs) are stored in the memory_write table.
This data could be used later to figure out when a linked list corruption happened.

The block-table is filled with information about which areas are allocated in the target application and what their timespan and size (again, user specified size, not real size) is.
While this is redundant to the stored parameters of the memory management functions, it's much easier and less error-prone to query this table.

#### Example
See https://asciinema.org/a/19626