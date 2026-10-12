# Editor Template

Code editor with syntax highlighting.

## Create

```bash
nexus new MyEditor --template editor
cd MyEditor
nexus build
nexus run
```

## Features

- **Syntax highlighting** for NEXUS
- **Line numbers**
- **Tabbed interface**
- **Find/Replace** (Ctrl+F / Ctrl+H)
- **Go to line** (Ctrl+G)
- **Auto-indent**
- **Bracket matching**
- **Multiple files**

## Structure

```
MyEditor/
├── nexus.toml
├── src/
│   ├── main.nx       # App entry
│   ├── editor.nx     # Core editor
│   ├── syntax.nx     # Highlighting
│   ├── tabs.nx       # Tab management
│   └── find.nx       # Find/replace
├── assets/
│   └── themes/
└── tests/
```

## Syntax Highlighting

```nx
// src/syntax.nx
struct Token {
    kind: TokenKind,
    start: usize,
    end: usize,
}

enum TokenKind {
    Keyword,      // fn, let, if, etc.
    Identifier,
    Number,
    String,
    Comment,
    Operator,
    Punctuation,
    Type,
}

fn highlight(source: str) -> []Token {
    // Lexer-based highlighting
}
```

## Themes

```nx
// assets/themes/dark.nx
const THEME_DARK = ui.Theme {
    background: 0x1a1a2e,
    foreground: 0xe0e0e0,
    cursor: 0xffffff,
    selection: 0x3a3a5a,
    line_number: 0x666666,
    keyword: 0xc792ea,
    string: 0xecc48d,
    number: 0xf78c6c,
    comment: 0x6a9955,
    function: 0x82aaff,
    type: 0xffcb6b,
    operator: 0x89ddff,
    punctuation: 0xffffff,
}
```

## Keybindings

| Key | Action |
|-----|--------|
| `Ctrl+S` | Save |
| `Ctrl+O` | Open |
| `Ctrl+N` | New tab |
| `Ctrl+W` | Close tab |
| `Ctrl+Tab` | Next tab |
| `Ctrl+Shift+Tab` | Prev tab |
| `Ctrl+F` | Find |
| `Ctrl+H` | Replace |
| `Ctrl+G` | Go to line |
| `Ctrl+/` | Toggle comment |
| `F12` | Go to definition |

## Extending

```nx
// Add language support
fn register_language(name: str, extensions: []str, highlighter: fn(str) -> []Token) {
    SYNTAX_REGISTRY.insert(name, { extensions, highlighter })
}

// Register NEXUS
register_language("nexus", ["nx"], highlight_nexus)

// Register Rust
register_language("rust", ["rs"], highlight_rust)
```