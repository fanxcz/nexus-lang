# Server Template

HTTP server template.

## Create

```bash
nexus new MyServer --template server
cd MyServer
nexus build
nexus run
```

## Structure

```
MyServer/
├── nexus.toml
├── src/
│   ├── main.nx       # Server entry
│   ├── routes.nx     # Route handlers
│   ├── middleware.nx
│   └── models.nx
├── assets/
│   └── static/       # Static files
└── tests/
```

## Features

- **Routing**: GET, POST, PUT, DELETE
- **Middleware**: Logging, CORS, Auth
- **Static files**: Serve `assets/static/`
- **JSON**: Request/response parsing
- **Query params**: `?key=value`
- **Path params**: `/users/:id`

## Example

```nx
// src/main.nx
import std.net
import std.json

fn main() {
    let server = net.server("127.0.0.1:8080")
    
    // Middleware
    server.use(middleware::logger)
    server.use(middleware::cors)
    
    // Routes
    server.get("/", |_| net.respond(200, "OK", "Hello!"))
    server.get("/health", |_| net.respond(200, "OK", "healthy"))
    
    server.get("/api/users", handlers::list_users)
    server.get("/api/users/:id", handlers::get_user)
    server.post("/api/users", handlers::create_user)
    server.put("/api/users/:id", handlers::update_user)
    server.delete("/api/users/:id", handlers::delete_user)
    
    // Static files
    server.static("/static", "assets/static")
    
    println("Server running on http://127.0.0.1:8080")
    server.run()
}
```

## Handlers

```nx
// src/handlers.nx
import std.net
import std.json

fn list_users(req: Request) -> Response {
    let users = db_query("SELECT * FROM users")?
    net.json_response(200, users)
}

fn get_user(req: Request) -> Response {
    let id = req.path_param("id")?
    let user = db_query("SELECT * FROM users WHERE id = ?", [id])?
    match user {
        Some(u) => net.json_response(200, u),
        None => net.json_response(404, { "error": "Not found" }),
    }
}

fn create_user(req: Request) -> Response {
    let body = req.json_body()?
    let name = body.get_str("name")?
    let email = body.get_str("email")?
    
    let id = db_execute("INSERT INTO users (name, email) VALUES (?, ?)", [name, email])?
    net.json_response(201, { "id": id, "name": name, "email": email })
}
```

## Middleware

```nx
// src/middleware.nx
module middleware {
    export fn logger(req: Request, next: fn(Request) -> Response) -> Response {
        let start = time_now()
        let resp = next(req)
        let ms = time_elapsed(start)
        println("{} {} - {}ms", req.method, req.path, ms)
        resp
    }
    
    export fn cors(req: Request, next: fn(Request) -> Response) -> Response {
        let mut resp = next(req)
        resp.headers.insert("Access-Control-Allow-Origin", "*")
        resp.headers.insert("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE")
        resp
    }
}
```

## Running

```bash
# Development
nexus run

# Production
nexus build --release
./build/MyServer

# With config
./build/MyServer --port 3000 --workers 4
```