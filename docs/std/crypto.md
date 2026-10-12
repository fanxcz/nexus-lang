# std.crypto

Cryptographic primitives.

## Hashing

```nx
import std.crypto

// SHA-256
let data = b"Hello, NEXUS!"
let hash = crypto.sha256(data)
// hash: [u8; 32]

// As hex string
let hex = hash.to_hex()  // "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"

// SHA-256 streaming (for large data)
let mut hasher = crypto.sha256_start()
hasher.update(b"Hello, ")
hasher.update(b"NEXUS!")
let hash = hasher.finalize()
```

## Random (Planned)

```nx
// Future API
// let bytes = crypto.random_bytes(32)  // Cryptographically secure
// let uuid = crypto.random_uuid()      // UUID v4
// let nonce = crypto.random_nonce()    // For encryption
```

## HMAC (Planned)

```nx
// Future API
// let hmac = crypto.hmac_sha256(key, data)
```

## Example

```nx
import std.crypto
import std.io

fn main() {
    let password = "my_secret_password"
    let salt = b"random_salt_1234"
    
    // Simple hash (not for passwords - use Argon2/bcrypt in real apps)
    let hash = crypto.sha256(password.as_bytes())
    println("SHA-256: {}", hash.to_hex())
    
    // Verify
    let input = "my_secret_password"
    if crypto.sha256(input.as_bytes()).to_hex() == hash.to_hex() {
        println("Password matches!")
    }
}
```

## Constants

```nx
crypto.SHA256_DIGEST_SIZE  // 32
crypto.SHA256_BLOCK_SIZE   // 64
```

## Notes

- **SHA-256** is the only algorithm currently implemented
- For **password hashing**, use external library (Argon2, bcrypt, scrypt)
- For **encryption**, use external library (libsodium, OpenSSL)
- **Random** functions planned for v1.2