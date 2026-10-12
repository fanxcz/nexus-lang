# `std.net`

The current `stdlib/net.nx` module exposes one helper:

```nx
http_get_text(url: string) -> string
```

It wraps the built-in `http_get(url)` function. The runtime invokes the system `curl` command with fail-on-HTTP-error and a 15-second timeout, captures the response body, and returns an empty string if the request fails or the command cannot run. It does not expose HTTP status codes, response headers, POST requests, TCP sockets, UDP sockets or a typed response object.

## Example

```nx
import std.net

fn main() {
    let body = http_get_text("https://example.com/")
    if body == "" {
        print("Request returned an empty body or failed")
        return
    }
    print(body)
}
```

Because an empty response body and a failed request both produce an empty string, this helper is suited to experiments rather than robust production HTTP handling. It depends on `curl` being available on the host.
