# Small C commandline for storing server information
Save, remove, search servers in a database file using SQLite.

### COMPILE
with:
	gcc main.c -o srvl -lsql

### TODO
	[ ] Finish the open functions
	[ ] New table where config is stored for how each protocol should be executed 
	[ ] - Functions/arguments for changing the config table
	[ ] Encryption (if possible)
	[ ] - Add field for password (If good encryption is possible)
	[ ] Ability to add multiple protocols, and protocol choice in the open function


### ARGUMENTS
	Add:          srvl add <name> <host> <port> <protocol>
	Remove:       srvl remove <id>
	Search:       srvl search <search string>
	Open by id:   srvl o <id>
	Open by name: srvl O <name>
