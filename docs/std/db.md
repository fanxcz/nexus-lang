# std.db

SQLite database interface.

## Connection

```nx
import std.db

fn main() {
    // Open database (":memory:" for in-memory)
    let conn = db.open("app.db")?
    defer db.close(conn)
    
    // Or with options
    let conn = db.open_with_flags("app.db", db.OPEN_READWRITE | db.OPEN_CREATE)?
}
```

## Execute

```nx
// Simple execute (no results)
db.execute(conn, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)")?

// Execute with parameters
db.execute(conn, "INSERT INTO users (name, email) VALUES (?, ?)", ["Alice", "alice@example.com"])?
db.execute(conn, "INSERT INTO users (name, email) VALUES (?, ?)", ["Bob", "bob@example.com"])?

// Batch execute
let stmts = [
    "INSERT INTO users (name) VALUES ('Charlie')",
    "INSERT INTO users (name) VALUES ('Diana')",
]
for sql in stmts {
    db.execute(conn, sql)?
}
```

## Query

```nx
// Query with parameters
let rows = db.query(conn, "SELECT id, name, email FROM users WHERE id > ?", [0])?

for row in rows {
    let id = row.get_i64(0)?
    let name = row.get_str(1)?
    let email = row.get_str(2)?
    println("{id}: {name} <{email}>")
}

// Single row
let row = db.query_row(conn, "SELECT name FROM users WHERE id = ?", [1])?
let name = row.get_str(0)?
```

## Prepared Statements

```nx
// Prepare once, execute many
let stmt = db.prepare(conn, "INSERT INTO users (name, email) VALUES (?, ?)")?

for user in users {
    db.execute_stmt(stmt, [user.name, user.email])?
}

// Prepared query
let stmt = db.prepare(conn, "SELECT * FROM users WHERE name LIKE ?")?
let rows = db.query_stmt(stmt, ["A%"])?
```

## Transactions

```nx
db.execute(conn, "BEGIN TRANSACTION")?
do {
    db.execute(conn, "INSERT INTO accounts (user_id, balance) VALUES (1, 100)")?
    db.execute(conn, "INSERT INTO accounts (user_id, balance) VALUES (2, 200)")?
    db.execute(conn, "COMMIT")?
} catch (e) {
    db.execute(conn, "ROLLBACK")?
    return Err(e)
}
```

## Row Methods

```nx
struct Row {
    fn get_i64(&self, index: usize) -> Result<i64, Error>
    fn get_f64(&self, index: usize) -> Result<f64, Error>
    fn get_str(&self, index: usize) -> Result<str, Error>
    fn get_bool(&self, index: usize) -> Result<bool, Error>
    fn get_blob(&self, index: usize) -> Result<[]u8, Error>
    fn get_null(&self, index: usize) -> Result<(), Error>
    fn column_count(&self) -> usize
    fn column_name(&self, index: usize) -> str
}
```

## Row Iteration

```nx
// By index
for row in rows {
    let id = row.get_i64(0)?
}

// By name (slower)
for row in rows {
    let name = row.get_str("name")?
}

// Collect all
let all_rows = rows.collect()
```

## Example: Simple CRUD

```nx
import std.db
import std.io

struct User {
    id: i64,
    name: str,
    email: str,
}

fn create_user(conn: *db.Connection, name: str, email: str) -> Result<i64, Error> {
    db.execute(conn, "INSERT INTO users (name, email) VALUES (?, ?)", [name, email])?
    db.last_insert_rowid(conn)
}

fn get_user(conn: *db.Connection, id: i64) -> Result<Option<User>, Error> {
    let row = db.query_row(conn, "SELECT id, name, email FROM users WHERE id = ?", [id])
    match row {
        Ok(r) => Ok(Some(User {
            id: r.get_i64(0)?,
            name: r.get_str(1)?,
            email: r.get_str(2)?,
        })),
        Err(db.Error::NotFound) => Ok(None),
        Err(e) => Err(e),
    }
}

fn list_users(conn: *db.Connection) -> Result<[]User, Error> {
    let rows = db.query(conn, "SELECT id, name, email FROM users")?
    let mut users = []
    for row in rows {
        users.push(User {
            id: row.get_i64(0)?,
            name: row.get_str(1)?,
            email: row.get_str(2)?,
        })
    }
    Ok(users)
}

fn main() {
    let conn = db.open(":memory:")?
    defer db.close(conn)
    
    db.execute(conn, "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT)")?
    
    let id1 = create_user(conn, "Alice", "alice@example.com")?
    let id2 = create_user(conn, "Bob", "bob@example.com")?
    
    let user = get_user(conn, id1)?
    println("User: {user.name}")
    
    for u in list_users(conn)? {
        println("{u.id}: {u.name} <{u.email}>")
    }
}
```

## Constants

```nx
db.OPEN_READONLY      // 0x0001
db.OPEN_READWRITE     // 0x0002
db.OPEN_CREATE        // 0x0004
db.OPEN_URI           // 0x0040
db.OPEN_MEMORY        // 0x0080
db.OPEN_NOMUTEX       // 0x0800
db.OPEN_FULLMUTEX     // 0x1000
```