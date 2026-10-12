# `std.db`

The current `stdlib/db.nx` module wraps the runtime's basic SQLite open/execute/close functions. It does not expose query rows, prepared statements, bound parameters, transactions, schema reflection or a rich error type.

## Functions

- `db_open(path: string) -> i64` returns a database handle; 0 indicates failure. Use `:memory:` for an in-memory database when the runtime can load SQLite.
- `db_exec(handle: i64, sql: string) -> i64` executes an SQL statement and returns the SQLite result code (0 means success).
- `db_close(handle: i64)` closes the handle.

## Example

```nx
import std.db

fn main() {
    let db = db_open(":memory:")
    if db == 0 {
        print("Could not open SQLite database")
        return
    }

    let create_rc = db_exec(db, "CREATE TABLE notes (id INTEGER PRIMARY KEY, text TEXT)")
    if create_rc != 0 {
        print("Could not create table; SQLite result: " + str_i64(create_rc))
        db_close(db)
        return
    }

    let insert_rc = db_exec(db, "INSERT INTO notes (text) VALUES ('Hello from NEXUS')")
    print("Insert result: " + str_i64(insert_rc))
    db_close(db)
}
```

The runtime loads SQLite dynamically, so availability depends on the bundled runtime library and platform. `db_exec` is for statements that do not need result rows. Do not assume it supports parameter binding or query results.
