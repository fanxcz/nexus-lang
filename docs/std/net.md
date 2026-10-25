# `std.net`

The current `stdlib/net.nx` module exposes one helper:

```nx
http_get_text(url: string) -> string
```

It wraps the built-in `http_get(url)` function. The runtime invokes the system `curl` executable directly (the URL is passed as a single argument, not interpreted by a shell), applies a 15-second timeout, and captures at most 16 MiB of response data. It returns an empty string if the request fails, the executable is unavailable, or the response exceeds the limit. It does not expose HTTP status codes, response headers, POST requests, TCP sockets, UDP sockets or a typed response object.

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

Because an empty response body and a failed request both produce an empty string, callers cannot distinguish these cases through this API. It still depends on `curl` being available on the host, so use it for experiments rather than robust production HTTP handling.
