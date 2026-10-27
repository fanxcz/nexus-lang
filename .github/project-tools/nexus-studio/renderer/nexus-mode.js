ace.define("ace/mode/nexus_highlight_rules", [
  "require", "exports", "module", "ace/lib/oop", "ace/mode/text_highlight_rules"
], function(require, exports, module) {
  "use strict";
  const oop = require("ace/lib/oop");
  const TextHighlightRules = require("ace/mode/text_highlight_rules").TextHighlightRules;

  const NexusHighlightRules = function() {
    this.$rules = {
      start: [
        { token: "comment", regex: "//.*$" },
        { token: "string", regex: '"(?:\\\\.|[^"\\\\])*"' },
        { token: "string", regex: "'(?:\\\\.|[^'\\\\])*'" },
        { token: "constant.numeric", regex: "\\b(?:0[xX][0-9a-fA-F]+|[0-9]+(?:\\.[0-9]+)?)\\b" },
        { token: "constant.language", regex: "\\b(?:true|false|null|none)\\b" },
        { token: "keyword", regex: "\\b(?:fn|let|mut|const|if|else|while|for|in|return|break|continue|match|struct|enum|import|as|pub|private|extern|type|impl|trait|self)\\b" },
        { token: "support.type", regex: "\\b(?:i64|i32|i16|i8|u64|u32|u16|u8|usize|isize|f64|f32|bool|string|str|char|void|Result|Option|Vec)\\b" },
        { token: "support.function", regex: "\\b(?:print|input|str_i64|len|assert|panic|json_[A-Za-z0-9_]+|http_get|http_get_text|gfx_[A-Za-z0-9_]+)\\b" },
        { token: "keyword.operator", regex: "===?|!==?|==|!=|<=|>=|=>|->|\\.\\.|\\+|-|\\*|/|%|&&|\\|\\||!|=|<|>" },
        { token: "paren.lparen", regex: "[({\\[]" },
        { token: "paren.rparen", regex: "[)}\\]]" },
        { token: "punctuation.operator", regex: "[;,.:]" },
        { token: "identifier", regex: "[A-Za-z_$][A-Za-z0-9_$]*" },
        { token: "text", regex: "\\s+" }
      ]
    };
    this.normalizeRules();
  };
  oop.inherits(NexusHighlightRules, TextHighlightRules);
  exports.NexusHighlightRules = NexusHighlightRules;
});

ace.define("ace/mode/nexus", [
  "require", "exports", "module", "ace/lib/oop", "ace/mode/text", "ace/mode/nexus_highlight_rules"
], function(require, exports, module) {
  "use strict";
  const oop = require("ace/lib/oop");
  const TextMode = require("ace/mode/text").Mode;
  const Mode = function() {
    this.HighlightRules = require("ace/mode/nexus_highlight_rules").NexusHighlightRules;
    this.lineCommentStart = "//";
  };
  oop.inherits(Mode, TextMode);
  exports.Mode = Mode;
});

// NEXUS Studio theme: near-black editor with a restrained yellow syntax accent.
ace.define("ace/theme/nexus", [
  "require", "exports", "module", "ace/lib/dom"
], function(require, exports, module) {
  "use strict";
  exports.isDark = true;
  exports.cssClass = "ace-nexus";
  exports.cssText = [
    ".ace-nexus { color: #e8e8e2; background-color: #090a0b; }",
    ".ace-nexus .ace_gutter { background: #0d0e0f; color: #777870; border-right: 1px solid #2a2b2c; }",
    ".ace-nexus .ace_print-margin { width: 1px; background: #202122; }",
    ".ace-nexus .ace_cursor { color: #f5c518; border-left: 2px solid #f5c518; }",
    ".ace-nexus .ace_marker-layer .ace_active-line { background: #19180f; }",
    ".ace-nexus .ace_gutter-active-line { background: #19180f; }",
    ".ace-nexus .ace_marker-layer .ace_selection { background: #4b400e; }",
    ".ace-nexus .ace_keyword, .ace-nexus .ace_storage { color: #f5c518; font-weight: 600; }",
    ".ace-nexus .ace_constant.ace_numeric, .ace-nexus .ace_constant.ace_language { color: #e3cf72; }",
    ".ace-nexus .ace_string { color: #d6d6c9; }",
    ".ace-nexus .ace_comment { color: #7e8078; font-style: italic; }",
    ".ace-nexus .ace_support.ace_function { color: #f0d66a; }",
    ".ace-nexus .ace_support.ace_type, .ace-nexus .ace_storage.ace_type { color: #c8c8bd; }",
    ".ace-nexus .ace_paren, .ace-nexus .ace_punctuation { color: #c2c2b8; }",
    ".ace-nexus .ace_indent-guide { background: linear-gradient(to bottom, transparent, transparent 50%, #252619 50%, #252619); background-size: 1px 4px; }",
    ".ace-nexus .ace_invisible { color: #3d3e38; }"
  ].join("\n");
  const dom = require("ace/lib/dom");
  dom.importCssString(exports.cssText, exports.cssClass);
});

