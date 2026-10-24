---
hide:
  - navigation
  - toc
---

# NEXUS

Компилируемый язык программирования и нативный рантайм для приложений,
инструментов и игр. Компилятор написан на C++20, часть экосистемы уже
переписывается на самом NEXUS ([self-hosting](development/roadmap.md)).

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

MIT · Windows · Linux · macOS

## Документация

- [Быстрый старт](getting-started/index.md)
- [Синтаксис языка](language/syntax.md)
- [Стандартная библиотека](std/index.md)
- [Команды CLI](tooling/commands.md)
- [Дорожная карта и self-hosting](development/roadmap.md)
- [Репозиторий на GitHub](https://github.com/fanxcz/nexus-lang)

## Контакты

Автор: **fanxcz**

- Telegram: [@fanxcz](https://t.me/fanxcz)
- Почта: [sad098184@gmail.com](mailto:sad098184@gmail.com)

Баг-репорты и предложения — в [issues на GitHub](https://github.com/fanxcz/nexus-lang/issues).
