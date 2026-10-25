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
