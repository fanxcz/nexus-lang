---
hide:
  - navigation
  - toc
---

# NEXUS

NEXUS is a compiled programming language and native runtime for applications,
tools, and games. The compiler is written in C++20. The runtime, standard
library, and a self-hosting toolchain are part of the same repository.

```nx
fn main() {
    let tool = "NEXUS"
    print("Hello from " + tool + "!")
}
```

```text
nexus new MyApp --template cli
nexus build
nexus run
```

License: MIT  
Platforms: Windows, Linux, macOS

## Documentation

- [Manual](manual.md)
- [Getting Started](getting-started/index.md)
- [Language Reference](language/syntax.md)
- [Standard Library](std/index.md)
- [Command Line Tooling](tooling/commands.md)
- [Self-Hosting Roadmap](development/roadmap.md)
- [GitHub Repository](https://github.com/fanxcz/nexus-lang)

## Downloads

- [Complete documentation as PDF](https://fanxcz.github.io/nexus-lang/pdf/NEXUS-Documentation.pdf)

## Contact

Author: fanxcz

- Telegram: [@fanxcz](https://t.me/fanxcz)
- Email: [sad098184@gmail.com](mailto:sad098184@gmail.com)

Please report issues on [GitHub Issues](https://github.com/fanxcz/nexus-lang/issues).
