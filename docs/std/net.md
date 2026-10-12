# std.net

Networking (HTTP client, TCP/UDP sockets).

## HTTP Client

```nx
import std.net

// Simple GET
let response = net.get("https://api.example.com/users")?
println("Status: {}", response.status)
println("Body: {}", response.text()?)

// GET with headers
let response = net.get_with_headers("https://api.example.com/data", [
    ("Authorization", "Bearer token123"),
    ("Accept", "application/json"),
])?

// POST JSON
let json = net.json.stringify({ "name": "Test", "value": 42 })
let response = net.post("https://api.example.com/items", json)?
println("Created: {}", response.text()?)

// POST with custom body
let response = net.post_with_headers(
    "https://api.example.com/upload",
    "raw data",
    [("Content-Type", "text/plain")]
)?
```

## Response

```nx
struct Response {
    status: i32,
    headers: Map<str, str>,
    body: []u8,
}

impl Response {
    fn text(&self) -> Result<str, Error>
    fn json(&self) -> Result<JsonValue, Error>
    fn bytes(&self) -> []u8
}
```

## TCP Sockets (Planned)

```nx
// Future API
// let listener = net.tcp_listen("127.0.0.1:8080")?
// for stream in listener.incoming() {
//     // handle connection
// }
//
// let stream = net.tcp_connect("127.0.0.1:8080")?
// stream.write(b"Hello")?
// let mut buf = [0u8; 1024]
// let n = stream.read(&mut buf)?
```

## UDP Sockets (Planned)

```nx
// Future API
// let socket = net.udp_bind("127.0.0.1:9000")?
// socket.send_to(b"data", "127.0.0.1:9001")?
// let (data, addr) = socket.recv_from(&mut buf)?
```

## DNS (Planned)

```nx
// Future API
// let ips = net.resolve("example.com")?
```

## Example

```nx
import std.net
import std.json

fn main() {
    // Fetch JSON API
    let resp = net.get("https://httpbin.org/json")?
    let data = resp.json()?
    
    let slideshow = data.get_object("slideshow")?
    let title = slideshow.get_str("title")?
    println("Title: {title}")
    
    // POST data
    let payload = json.stringify({
        "title": "Test Post",
        "body": "Hello from NEXUS!",
        "userId": 1
    })
    let resp = net.post("https://httpbin.org/post", payload)?
    println("Response: {}", resp.text()?)
}
```

## Error Handling

```nx
match net.get("https://api.example.com") {
    Ok(resp) => { ... }
    Err(e) => match e {
        NetError::ConnectionFailed => print("No connection"),
        NetError::Timeout => print("Request timed out"),
        NetError::InvalidUrl => print("Bad URL"),
        NetError::HttpError(code) => print("HTTP {code}"),
        NetError::IoError(io_err) => print("IO: {io_err}"),
    }
}
```