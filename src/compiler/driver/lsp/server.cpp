#include "driver/driver_internal.hpp"

#include "driver/lsp/diagnostics.hpp"
#include "driver/lsp/protocol.hpp"
#include "driver/lsp/text.hpp"
#include "driver/lsp/uri.hpp"

namespace nexus {
    namespace driver {


        int commandLsp() {
            using lsp::byteOffsetForUtf16;
            using lsp::collectDiagnostics;
            using lsp::editDistance;
            using lsp::extractId;
            using lsp::extractNumber;
            using lsp::extractString;
            using lsp::identifierCharacter;
            using lsp::joinRange;
            using lsp::jsonEscape;
            using lsp::pathFromUri;
            using lsp::send;
            using lsp::splitLines;
            using lsp::supportsWatchedFilesRegistration;
            using lsp::uriFromPath;
            using lsp::utf16Length;
#ifdef _WIN32
            // LSP framing uses explicit CRLF bytes; disable Windows text-mode newline translation.
            if (_setmode(_fileno(stdout), _O_BINARY) == -1) {
                std::cerr << "NEXUS LSP: could not switch stdout to binary mode\\n";
                return 2;
            }
#endif
            bool clientSupportsWatchedFilesRegistration = false;
            std::unordered_map<std::string, std::string> openDocuments;
            // Decode the path component of a file:// URI, including UTF-8 bytes.
            // Cache disk-backed source files per workspace. Open buffers overlay this
            // cache, so typing does not trigger a directory walk for every LSP request.
            std::map<std::string, std::map<std::string, std::string>> workspaceDiskCache;
            struct LspFunctionSignatureInfo {
                std::string label;
                std::vector<std::string> parameters;
            };
            struct LspTypeDeclarationIndex {
                std::string source;
                std::set<std::string> structNames;
                std::set<std::string> enumNames;
                std::set<std::string> functionNames;
                std::vector<StructDecl> structs;
                std::vector<EnumDecl> enums;
                std::map<std::string, std::string> functionReturnTypes;
                std::map<std::string, std::string> functionHoverMarkdown;
                std::map<std::string, LspFunctionSignatureInfo> functionSignatures;
                bool declarationsParsed = false;
            };
            std::map<std::string, LspTypeDeclarationIndex> workspaceTypeDeclarationCache;
            const auto workspaceRootForUri = [&](const std::string& uri) -> fs::path {
                const fs::path documentPath = pathFromUri(uri);
                if (documentPath.empty())
                    return {};
                const fs::path projectRoot = findProjectRoot(documentPath);
                return projectRoot.empty() ? documentPath.parent_path() : projectRoot;
            };
            const auto workspaceCacheKeyForUri = [&](const std::string& uri) {
                const fs::path root = workspaceRootForUri(uri);
                if (root.empty() || root == root.root_path())
                    return std::string{};
                std::error_code ec;
                const fs::path normalized = fs::weakly_canonical(root, ec);
                return (ec ? root.lexically_normal() : normalized).generic_string();
            };
            const auto collectWorkspaceDocuments = [&](const std::string& requestUri,
                                                       const std::string& requestSource) {
                std::map<std::string, std::string> byUri;
                const fs::path documentPath = pathFromUri(requestUri);
                const fs::path projectRoot =
                    documentPath.empty() ? fs::path{} : findProjectRoot(documentPath);
                const bool hasProjectManifest = !projectRoot.empty();
                const fs::path root =
                    documentPath.empty()
                        ? fs::path{}
                        : (hasProjectManifest ? projectRoot : documentPath.parent_path());

                // Virtual editor URIs and root-level paths only participate through
                // currently open buffers. Never walk the filesystem root.
                const bool canScan = !root.empty() && root != root.root_path();
                const std::string cacheKey =
                    canScan ? workspaceCacheKeyForUri(requestUri) : std::string{};
                if (canScan && !cacheKey.empty()) {
                    auto cached = workspaceDiskCache.find(cacheKey);
                    if (cached == workspaceDiskCache.end()) {
                        std::map<std::string, std::string> diskDocuments;
                        std::size_t indexedFiles = 0;
                        constexpr std::size_t maxWorkspaceFiles = 10000;
                        std::error_code ec;
                        const auto addFile = [&](const fs::path& path) {
                            if (indexedFiles >= maxWorkspaceFiles || path.extension() != ".nx")
                                return;
                            const std::string fileUri = uriFromPath(path);
                            if (diskDocuments.find(fileUri) != diskDocuments.end())
                                return;
                            try {
                                diskDocuments.emplace(fileUri, readFile(path));
                                ++indexedFiles;
                            } catch (const std::exception&) {
                                // An unreadable or transiently removed file is not fatal.
                            }
                        };
                        if (hasProjectManifest) {
                            const auto options = fs::directory_options::skip_permission_denied;
                            fs::recursive_directory_iterator it(root, options, ec), end;
                            while (!ec && it != end && indexedFiles < maxWorkspaceFiles) {
                                const fs::path current = it->path();
                                if (it->is_directory(ec)) {
                                    const std::string name = current.filename().string();
                                    if (name == ".git" || name == "build" || name == "dist" ||
                                        name == "target" || name == "node_modules" ||
                                        name == ".nexus") {
                                        it.disable_recursion_pending();
                                    }
                                } else if (it->is_regular_file(ec)) {
                                    addFile(current);
                                }
                                it.increment(ec);
                            }
                        } else {
                            // Without nexus.toml, discover only direct .nx siblings.
                            fs::directory_iterator it(
                                root, fs::directory_options::skip_permission_denied, ec),
                                end;
                            while (!ec && it != end && indexedFiles < maxWorkspaceFiles) {
                                if (it->is_regular_file(ec))
                                    addFile(it->path());
                                it.increment(ec);
                            }
                        }
                        cached =
                            workspaceDiskCache.emplace(cacheKey, std::move(diskDocuments)).first;
                    }
                    byUri = cached->second;
                }

                // Unsaved editor buffers override cached disk content. Include all
                // open buffers so cross-file navigation also works for virtual docs.
                for (const auto& opened : openDocuments)
                    byUri[opened.first] = opened.second;
                byUri[requestUri] = requestSource;

                std::vector<std::pair<std::string, std::string>> result;
                result.reserve(byUri.size());
                for (const auto& item : byUri)
                    result.push_back(item);
                return result;
            };
            struct LspIdentifierOccurrence {
                std::string uri;
                std::size_t line = 0;
                std::size_t startCharacter = 0;
                std::size_t endCharacter = 0;
                bool isDeclaration = false;
            };

            // The LSP keeps per-file lexical indexes and rebuilds only files whose
            // source text changed. These are intentionally syntax/lexical indexes,
            // not substitutes for semantic analysis in the compiler.
            struct LspScopeInfo {
                std::size_t parent = static_cast<std::size_t>(-1);
                std::size_t depth = 0;
            };
            struct LspSymbolDecl {
                std::string name;
                std::string identity;
                std::string kind;
                std::size_t tokenIndex = 0;
                std::size_t visibleFrom = 0;
                std::size_t scope = 0;
                bool global = false;
            };
            struct LspDocumentIndex {
                std::string uri;
                std::string source;
                std::vector<Token> tokens;
                std::vector<std::size_t> tokenScope;
                std::vector<std::size_t> braceScope;
                std::vector<LspScopeInfo> scopes;
                std::vector<LspSymbolDecl> declarations;
            };
            std::map<std::string, LspDocumentIndex> workspaceSymbolIndexCache;
            // Contextual completion scans tokens first, and parses only source files that
            // declare the requested type or function. This keeps editor requests cheap.
            const auto workspaceTypeIndexFor =
                [&](const std::string& documentUri,
                    const std::string& source) -> LspTypeDeclarationIndex& {
                auto cached = workspaceTypeDeclarationCache.find(documentUri);
                if (cached == workspaceTypeDeclarationCache.end() ||
                    cached->second.source != source) {
                    LspTypeDeclarationIndex index;
                    index.source = source;
                    try {
                        const auto tokens = Lexer(source).tokenize();
                        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
                            if (tokens[i + 1].kind != TokenKind::Identifier)
                                continue;
                            if (tokens[i].kind == TokenKind::KwStruct)
                                index.structNames.insert(tokens[i + 1].text);
                            else if (tokens[i].kind == TokenKind::KwEnum)
                                index.enumNames.insert(tokens[i + 1].text);
                            else if (tokens[i].kind == TokenKind::KwFn)
                                index.functionNames.insert(tokens[i + 1].text);
                        }
                    } catch (const std::exception&) {
                        // One incomplete workspace buffer must not break completion.
                    }
                    cached = workspaceTypeDeclarationCache
                                 .insert_or_assign(documentUri, std::move(index))
                                 .first;
                }
                return cached->second;
            };
            const auto parseWorkspaceTypeIndex = [&](LspTypeDeclarationIndex& index) {
                if (index.declarationsParsed)
                    return;
                try {
                    Program parsed = Parser(Lexer(index.source).tokenize()).parseProgram();
                    index.structs = std::move(parsed.structs);
                    index.enums = std::move(parsed.enums);
                    const auto describeIndexedType = [&](const auto& self,
                                                         const Type& type) -> std::string {
                        switch (type.kind) {
                        case TypeKind::I8: return "i8";
                        case TypeKind::I16: return "i16";
                        case TypeKind::I32: return "i32";
                        case TypeKind::I64: return "i64";
                        case TypeKind::U8: return "u8";
                        case TypeKind::U16: return "u16";
                        case TypeKind::U32: return "u32";
                        case TypeKind::U64: return "u64";
                        case TypeKind::F32: return "f32";
                        case TypeKind::F64: return "f64";
                        case TypeKind::Bool:
                            return "bool";
                        case TypeKind::String:
                            return "string";
                        case TypeKind::Void:
                            return "void";
                        case TypeKind::Pointer:
                            return type.pointee ? "*" + self(self, *type.pointee) : "*unknown";
                        case TypeKind::Struct:
                            return type.name.empty() ? "struct" : type.name;
                        case TypeKind::Enum:
                            return type.name.empty() ? "enum" : type.name;
                        case TypeKind::Array:
                            return type.element ? "[" + self(self, *type.element) + "; " +
                                                      std::to_string(type.arraySize) + "]"
                                                : "[unknown]";
                        case TypeKind::Slice:
                            return type.element ? "[]" + self(self, *type.element) : "[]unknown";
                        case TypeKind::MutableSlice:
                            return type.element ? "mut []" + self(self, *type.element)
                                                : "mut []unknown";
                        case TypeKind::Unknown:
                            return "inferred";
                        }
                        return "unknown";
                    };
                    for (const auto& function : parsed.functions) {
                        std::ostringstream signature;
                        signature << "fn " << function.name << "(";
                        for (std::size_t i = 0; i < function.params.size(); ++i) {
                            if (i)
                                signature << ", ";
                            signature << function.params[i].name << ": "
                                      << describeIndexedType(describeIndexedType,
                                                             function.params[i].type);
                        }
                        signature << ") -> "
                                  << describeIndexedType(describeIndexedType, function.ret);
                        index.functionHoverMarkdown.insert_or_assign(
                            function.name, "**" + signature.str() + "**\n\nUser-defined function.");

                        LspFunctionSignatureInfo signatureInfo;
                        signatureInfo.label = signature.str();
                        for (const auto& parameter : function.params) {
                            signatureInfo.parameters.push_back(
                                parameter.name + ": " +
                                describeIndexedType(describeIndexedType, parameter.type));
                        }
                        index.functionSignatures.insert_or_assign(function.name,
                                                                  std::move(signatureInfo));
                        if (function.ret.kind == TypeKind::Struct && !function.ret.name.empty()) {
                            index.functionReturnTypes.insert_or_assign(function.name,
                                                                       function.ret.name);
                        }
                    }
                } catch (const std::exception&) {
                    // Incomplete workspace source should never crash an LSP request.
                    index.structs.clear();
                    index.enums.clear();
                    index.functionReturnTypes.clear();
                    index.functionHoverMarkdown.clear();
                    index.functionSignatures.clear();
                }
                index.declarationsParsed = true;
            };
            const auto workspaceTypeDeclarationsFor = [&](const std::string& requestUri,
                                                          const std::string& requestSource,
                                                          const std::string& requestedType,
                                                          bool requestedEnum) {
                std::pair<std::vector<StructDecl>, std::vector<EnumDecl>> result;
                if (requestedType.empty())
                    return result;

                for (const auto& [documentUri, source] :
                     collectWorkspaceDocuments(requestUri, requestSource)) {
                    if (documentUri == requestUri)
                        continue;

                    auto& index = workspaceTypeIndexFor(documentUri, source);
                    const bool typeExists = requestedEnum
                                                ? index.enumNames.contains(requestedType)
                                                : index.structNames.contains(requestedType);
                    if (!typeExists)
                        continue;

                    parseWorkspaceTypeIndex(index);
                    if (requestedEnum) {
                        for (const auto& declaration : index.enums) {
                            if (declaration.name == requestedType)
                                result.second.push_back(declaration);
                        }
                    } else {
                        for (const auto& declaration : index.structs) {
                            if (declaration.name == requestedType)
                                result.first.push_back(declaration);
                        }
                    }
                }
                return result;
            };
            const auto workspaceFunctionReturnTypeFor = [&](const std::string& requestUri,
                                                            const std::string& requestSource,
                                                            const std::string& requestedFunction) {
                if (requestedFunction.empty())
                    return std::string{};

                for (const auto& [documentUri, source] :
                     collectWorkspaceDocuments(requestUri, requestSource)) {
                    if (documentUri == requestUri)
                        continue;

                    auto& index = workspaceTypeIndexFor(documentUri, source);
                    if (!index.functionNames.contains(requestedFunction))
                        continue;

                    parseWorkspaceTypeIndex(index);
                    const auto found = index.functionReturnTypes.find(requestedFunction);
                    if (found != index.functionReturnTypes.end())
                        return found->second;
                }
                return std::string{};
            };

            const auto identifierAtPosition = [&](const std::string& source,
                                                  std::size_t requestedLine,
                                                  std::size_t requestedCharacter) {
                std::istringstream input(source);
                std::vector<std::string> sourceLines;
                std::string line;
                while (std::getline(input, line))
                    sourceLines.push_back(line);
                if (requestedLine >= sourceLines.size())
                    return std::string{};
                const std::string& selected = sourceLines[requestedLine];
                std::size_t cursor = byteOffsetForUtf16(selected, requestedCharacter);
                if (cursor > 0 &&
                    (cursor == selected.size() ||
                     !identifierCharacter(static_cast<unsigned char>(selected[cursor]))) &&
                    identifierCharacter(static_cast<unsigned char>(selected[cursor - 1]))) {
                    --cursor;
                }
                try {
                    const auto tokens = Lexer(source).tokenize();
                    for (const auto& token : tokens) {
                        if (token.kind == TokenKind::String || token.kind == TokenKind::Integer ||
                            token.kind == TokenKind::Float || token.kind == TokenKind::End ||
                            token.pos.line != requestedLine + 1 || token.text.empty())
                            continue;
                        bool wordToken = true;
                        for (const unsigned char ch : token.text) {
                            if (!identifierCharacter(ch)) {
                                wordToken = false;
                                break;
                            }
                        }
                        if (!wordToken)
                            continue;
                        const std::size_t start = token.pos.column > 0 ? token.pos.column - 1 : 0;
                        if (cursor >= start && cursor < start + token.text.size())
                            return token.text;
                    }
                } catch (const std::exception&) {
                    // An incomplete string/comment can make the current buffer un-tokenizable.
                }
                return std::string{};
            };
            const auto findIdentifierOccurrences = [&](const std::string& identifier,
                                                       const std::string& requestUri,
                                                       const std::string& requestSource,
                                                       std::size_t requestedLine,
                                                       std::size_t requestedCharacter) {
                std::vector<LspIdentifierOccurrence> found;
                if (identifier.empty())
                    return found;

                std::vector<LspDocumentIndex*> documents;
                for (const auto& document : collectWorkspaceDocuments(requestUri, requestSource)) {
                    auto cachedIndex = workspaceSymbolIndexCache.find(document.first);
                    if (cachedIndex != workspaceSymbolIndexCache.end() &&
                        cachedIndex->second.source == document.second) {
                        documents.push_back(&cachedIndex->second);
                        continue;
                    }

                    LspDocumentIndex indexed;
                    indexed.uri = document.first;
                    indexed.source = document.second;
                    try {
                        indexed.tokens = Lexer(indexed.source).tokenize();
                    } catch (const std::exception&) {
                        workspaceSymbolIndexCache.erase(document.first);
                        continue;
                    }
                    const std::size_t tokenCount = indexed.tokens.size();
                    indexed.tokenScope.assign(tokenCount, 0);
                    indexed.braceScope.assign(tokenCount, static_cast<std::size_t>(-1));
                    indexed.scopes.push_back({static_cast<std::size_t>(-1), 0});
                    std::vector<std::size_t> scopeStack{0};
                    for (std::size_t i = 0; i < tokenCount; ++i) {
                        const TokenKind kind = indexed.tokens[i].kind;
                        if (kind == TokenKind::LBrace) {
                            indexed.tokenScope[i] = scopeStack.back();
                            const std::size_t parent = scopeStack.back();
                            const std::size_t scope = indexed.scopes.size();
                            indexed.scopes.push_back({parent, indexed.scopes[parent].depth + 1});
                            indexed.braceScope[i] = scope;
                            scopeStack.push_back(scope);
                        } else if (kind == TokenKind::RBrace) {
                            indexed.tokenScope[i] = scopeStack.back();
                            if (scopeStack.size() > 1)
                                scopeStack.pop_back();
                        } else {
                            indexed.tokenScope[i] = scopeStack.back();
                        }
                    }

                    const auto addDeclaration = [&](std::size_t tokenIndex,
                                                    std::size_t visibleFrom,
                                                    std::size_t scope,
                                                    const std::string& kind,
                                                    bool global) {
                        if (tokenIndex >= tokenCount ||
                            indexed.tokens[tokenIndex].kind != TokenKind::Identifier)
                            return;
                        const std::string name = indexed.tokens[tokenIndex].text;
                        const std::string identity =
                            global ? "global:" + kind + ":" + name
                                   : "local:" + indexed.uri + ":" + std::to_string(tokenIndex);
                        indexed.declarations.push_back(
                            {name, identity, kind, tokenIndex, visibleFrom, scope, global});
                    };

                    for (std::size_t i = 1; i < tokenCount; ++i) {
                        if (indexed.tokenScope[i] != 0 ||
                            indexed.tokens[i].kind != TokenKind::Identifier)
                            continue;
                        const TokenKind before = indexed.tokens[i - 1].kind;
                        if (before == TokenKind::KwFn)
                            addDeclaration(i, 0, 0, "function", true);
                        else if (before == TokenKind::KwStruct)
                            addDeclaration(i, 0, 0, "struct", true);
                        else if (before == TokenKind::KwEnum)
                            addDeclaration(i, 0, 0, "enum", true);
                    }

                    for (std::size_t i = 0; i < tokenCount; ++i) {
                        const TokenKind kind = indexed.tokens[i].kind;
                        if (kind == TokenKind::KwLet || kind == TokenKind::KwConst) {
                            std::size_t nameIndex = i + 1;
                            if (kind == TokenKind::KwLet && nameIndex < tokenCount &&
                                indexed.tokens[nameIndex].kind == TokenKind::KwMut)
                                ++nameIndex;
                            if (nameIndex < tokenCount &&
                                indexed.tokens[nameIndex].kind == TokenKind::Identifier) {
                                addDeclaration(nameIndex,
                                               nameIndex,
                                               indexed.tokenScope[nameIndex],
                                               kind == TokenKind::KwConst ? "constant" : "variable",
                                               false);
                            }
                        } else if (kind == TokenKind::KwFor && i + 1 < tokenCount &&
                                   indexed.tokens[i + 1].kind == TokenKind::Identifier) {
                            const std::size_t nameIndex = i + 1;
                            std::size_t bodyOpen = nameIndex + 1;
                            while (bodyOpen < tokenCount &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::LBrace &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::Semicolon)
                                ++bodyOpen;
                            if (bodyOpen < tokenCount &&
                                indexed.tokens[bodyOpen].kind == TokenKind::LBrace &&
                                indexed.braceScope[bodyOpen] != static_cast<std::size_t>(-1)) {
                                addDeclaration(nameIndex,
                                               bodyOpen + 1,
                                               indexed.braceScope[bodyOpen],
                                               "variable",
                                               false);
                            }
                        } else if (kind == TokenKind::KwFn) {
                            std::size_t openParen = i + 1;
                            while (openParen < tokenCount &&
                                   indexed.tokens[openParen].kind != TokenKind::LParen &&
                                   indexed.tokens[openParen].kind != TokenKind::Semicolon)
                                ++openParen;
                            if (openParen >= tokenCount ||
                                indexed.tokens[openParen].kind != TokenKind::LParen)
                                continue;
                            std::size_t closeParen = openParen + 1;
                            std::size_t parenDepth = 1;
                            for (; closeParen < tokenCount && parenDepth > 0; ++closeParen) {
                                if (indexed.tokens[closeParen].kind == TokenKind::LParen)
                                    ++parenDepth;
                                else if (indexed.tokens[closeParen].kind == TokenKind::RParen)
                                    --parenDepth;
                            }
                            if (parenDepth != 0)
                                continue;
                            --closeParen;
                            std::size_t bodyOpen = closeParen + 1;
                            while (bodyOpen < tokenCount &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::LBrace &&
                                   indexed.tokens[bodyOpen].kind != TokenKind::Semicolon)
                                ++bodyOpen;
                            if (bodyOpen >= tokenCount ||
                                indexed.tokens[bodyOpen].kind != TokenKind::LBrace ||
                                indexed.braceScope[bodyOpen] == static_cast<std::size_t>(-1))
                                continue;
                            const std::size_t bodyScope = indexed.braceScope[bodyOpen];
                            for (std::size_t param = openParen + 1; param + 1 < closeParen;
                                 ++param) {
                                if (indexed.tokens[param].kind == TokenKind::Identifier &&
                                    indexed.tokens[param + 1].kind == TokenKind::Colon) {
                                    addDeclaration(
                                        param, bodyOpen + 1, bodyScope, "parameter", false);
                                }
                            }
                        }
                    }
                    auto indexedEntry =
                        workspaceSymbolIndexCache.insert_or_assign(indexed.uri, std::move(indexed))
                            .first;
                    documents.push_back(&indexedEntry->second);
                }

                const auto isAncestorScope =
                    [](const LspDocumentIndex& document, std::size_t ancestor, std::size_t scope) {
                        while (scope != static_cast<std::size_t>(-1)) {
                            if (scope == ancestor)
                                return true;
                            if (scope >= document.scopes.size())
                                break;
                            scope = document.scopes[scope].parent;
                        }
                        return false;
                    };
                const auto declarationAt = [](const LspDocumentIndex& document,
                                              std::size_t tokenIndex) -> const LspSymbolDecl* {
                    for (const auto& declaration : document.declarations)
                        if (declaration.tokenIndex == tokenIndex)
                            return &declaration;
                    return nullptr;
                };
                const auto localIdentityAt = [&](const LspDocumentIndex& document,
                                                 std::size_t tokenIndex) {
                    if (tokenIndex >= document.tokens.size())
                        return std::string{};
                    if (const auto* declaration = declarationAt(document, tokenIndex);
                        declaration && !declaration->global)
                        return declaration->identity;
                    if (document.tokens[tokenIndex].kind != TokenKind::Identifier)
                        return std::string{};
                    const std::string& name = document.tokens[tokenIndex].text;
                    const LspSymbolDecl* selected = nullptr;
                    for (const auto& declaration : document.declarations) {
                        if (declaration.global || declaration.name != name)
                            continue;
                        const bool isOwnDeclaration = declaration.tokenIndex == tokenIndex;
                        const bool visible = tokenIndex >= declaration.visibleFrom &&
                                             isAncestorScope(document,
                                                             declaration.scope,
                                                             document.tokenScope[tokenIndex]);
                        if (!isOwnDeclaration && !visible)
                            continue;
                        if (!selected ||
                            document.scopes[declaration.scope].depth >
                                document.scopes[selected->scope].depth ||
                            (document.scopes[declaration.scope].depth ==
                                 document.scopes[selected->scope].depth &&
                             declaration.visibleFrom > selected->visibleFrom))
                            selected = &declaration;
                    }
                    return selected ? selected->identity : std::string{};
                };
                const auto isMemberOrFieldName = [&](const LspDocumentIndex& document,
                                                     std::size_t tokenIndex) {
                    if (tokenIndex >= document.tokens.size())
                        return true;
                    const auto& tokens = document.tokens;
                    if (tokenIndex > 0 && (tokens[tokenIndex - 1].kind == TokenKind::Dot ||
                                           tokens[tokenIndex - 1].kind == TokenKind::ColonColon))
                        return true;
                    if (tokenIndex + 1 < tokens.size() &&
                        tokens[tokenIndex + 1].kind == TokenKind::Colon && tokenIndex > 0 &&
                        (tokens[tokenIndex - 1].kind == TokenKind::LBrace ||
                         tokens[tokenIndex - 1].kind == TokenKind::Comma)) {
                        const auto* declaration = declarationAt(document, tokenIndex);
                        if (!declaration || declaration->kind != "parameter")
                            return true;
                    }
                    return false;
                };
                const auto tokenAtPosition =
                    [&](const LspDocumentIndex& document, std::size_t line, std::size_t character) {
                        std::istringstream sourceStream(document.source);
                        std::vector<std::string> sourceLines;
                        std::string sourceLine;
                        while (std::getline(sourceStream, sourceLine))
                            sourceLines.push_back(sourceLine);
                        if (line >= sourceLines.size())
                            return document.tokens.size();
                        std::size_t cursor = byteOffsetForUtf16(sourceLines[line], character);
                        if (cursor > 0 &&
                            (cursor == sourceLines[line].size() ||
                             !identifierCharacter(
                                 static_cast<unsigned char>(sourceLines[line][cursor]))) &&
                            identifierCharacter(
                                static_cast<unsigned char>(sourceLines[line][cursor - 1])))
                            --cursor;
                        for (std::size_t i = 0; i < document.tokens.size(); ++i) {
                            const Token& token = document.tokens[i];
                            if (token.kind != TokenKind::Identifier || token.pos.line != line + 1 ||
                                token.text != identifier || token.pos.column == 0)
                                continue;
                            const std::size_t tokenStart = token.pos.column - 1;
                            if (cursor >= tokenStart && cursor < tokenStart + token.text.size())
                                return i;
                        }
                        return document.tokens.size();
                    };

                LspDocumentIndex* requestedDocument = nullptr;
                for (auto* document : documents) {
                    if (document->uri == requestUri) {
                        requestedDocument = document;
                        break;
                    }
                }
                if (!requestedDocument)
                    return found;
                const std::size_t selectedTokenIndex =
                    tokenAtPosition(*requestedDocument, requestedLine, requestedCharacter);
                if (selectedTokenIndex >= requestedDocument->tokens.size())
                    return found;

                const LspSymbolDecl* selectedDeclaration =
                    declarationAt(*requestedDocument, selectedTokenIndex);
                std::string targetIdentity;
                bool targetIsGlobal = false;
                std::string targetKind;
                if (selectedDeclaration) {
                    targetIdentity = selectedDeclaration->identity;
                    targetIsGlobal = selectedDeclaration->global;
                    targetKind = selectedDeclaration->kind;
                } else {
                    targetIdentity = localIdentityAt(*requestedDocument, selectedTokenIndex);
                    if (targetIdentity.empty() &&
                        !isMemberOrFieldName(*requestedDocument, selectedTokenIndex)) {
                        const auto& tokens = requestedDocument->tokens;
                        std::string preferredKind;
                        if (selectedTokenIndex + 1 < tokens.size() &&
                            tokens[selectedTokenIndex + 1].kind == TokenKind::LParen)
                            preferredKind = "function";
                        else if (selectedTokenIndex + 1 < tokens.size() &&
                                 tokens[selectedTokenIndex + 1].kind == TokenKind::LBrace)
                            preferredKind = "struct";
                        else if (selectedTokenIndex + 1 < tokens.size() &&
                                 tokens[selectedTokenIndex + 1].kind == TokenKind::ColonColon)
                            preferredKind = "enum";
                        const LspSymbolDecl* global = nullptr;
                        for (const auto* documentPointer : documents) {
                            const auto& document = *documentPointer;
                            for (const auto& declaration : document.declarations) {
                                if (!declaration.global || declaration.name != identifier)
                                    continue;
                                if (!preferredKind.empty() && declaration.kind != preferredKind)
                                    continue;
                                if (!global)
                                    global = &declaration;
                            }
                            if (global)
                                break;
                        }
                        if (global) {
                            targetIdentity = global->identity;
                            targetIsGlobal = true;
                            targetKind = global->kind;
                        }
                    }
                }
                if (targetIdentity.empty())
                    return found;

                for (const auto* documentPointer : documents) {
                    const auto& document = *documentPointer;
                    for (std::size_t i = 0; i < document.tokens.size(); ++i) {
                        const Token& token = document.tokens[i];
                        if (token.kind != TokenKind::Identifier || token.text != identifier)
                            continue;
                        const LspSymbolDecl* declaration = declarationAt(document, i);
                        bool matches = false;
                        if (!targetIsGlobal) {
                            matches = document.uri == requestedDocument->uri &&
                                      localIdentityAt(document, i) == targetIdentity;
                        } else {
                            if (declaration && declaration->global) {
                                matches = declaration->identity == targetIdentity;
                            } else if (!isMemberOrFieldName(document, i) &&
                                       localIdentityAt(document, i).empty()) {
                                matches = true;
                            }
                        }
                        if (!matches)
                            continue;
                        if (document.uri == requestedDocument->uri && i == selectedTokenIndex) {
                            // Keep the cursor token even for unusual valid syntax.
                        }
                        std::istringstream sourceStream(document.source);
                        std::string sourceLine;
                        for (std::size_t currentLine = 1;
                             currentLine < token.pos.line && std::getline(sourceStream, sourceLine);
                             ++currentLine) {}
                        if (token.pos.line == 0)
                            continue;
                        const std::size_t lineIndex = token.pos.line - 1;
                        const std::size_t startByte =
                            token.pos.column > 0 ? token.pos.column - 1 : 0;
                        sourceLine.clear();
                        std::istringstream lineStream(document.source);
                        for (std::size_t currentLine = 0;
                             currentLine <= lineIndex && std::getline(lineStream, sourceLine);
                             ++currentLine) {
                            if (currentLine == lineIndex)
                                break;
                        }
                        const std::size_t startCharacter = utf16Length(
                            sourceLine.substr(0, std::min(startByte, sourceLine.size())));
                        const LspSymbolDecl* occurrenceDeclaration = declarationAt(document, i);
                        const bool isDeclaration =
                            occurrenceDeclaration &&
                            occurrenceDeclaration->identity == targetIdentity;
                        found.push_back({document.uri,
                                         lineIndex,
                                         startCharacter,
                                         startCharacter + utf16Length(token.text),
                                         isDeclaration});
                    }
                }
                return found;
            };
            const auto isValidIdentifier = [&](const std::string& candidate) {
                try {
                    const auto tokens = Lexer(candidate).tokenize();
                    return tokens.size() == 2 && tokens[0].kind == TokenKind::Identifier &&
                           tokens[0].text == candidate && tokens[1].kind == TokenKind::End;
                } catch (const std::exception&) {
                    return false;
                }
            };
            const auto publishDiagnostics = [&](const std::string& uri, const std::string& source) {
                const auto diagnostics = collectDiagnostics(source);
                std::ostringstream payload;
                payload << "{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\","
                        << "\"params\":{\"uri\":\"" << jsonEscape(uri) << "\",\"diagnostics\":[";
                for (std::size_t i = 0; i < diagnostics.size(); ++i) {
                    if (i)
                        payload << ',';
                    const auto& diagnostic = diagnostics[i];
                    payload << "{\"range\":{\"start\":{\"line\":" << diagnostic.line
                            << ",\"character\":" << diagnostic.character
                            << "},\"end\":{\"line\":" << diagnostic.line << ",\"character\":"
                            << diagnostic.character + std::max<std::size_t>(1, diagnostic.length)
                            << "}},\"severity\":1,\"code\":\"" << diagnostic.code
                            << "\",\"source\":\"nexus\",\"message\":\""
                            << jsonEscape(diagnostic.message) << "\"}";
                }
                payload << "]}}";
                send(payload.str());
            };

            const auto signatureHelpResult = [&](const std::string& uri,
                                                 const std::string& source,
                                                 std::size_t requestedLine,
                                                 std::size_t requestedCharacter) {
                std::size_t lineStart = 0;
                for (std::size_t line = 0; line < requestedLine; ++line) {
                    const auto newline = source.find('\n', lineStart);
                    if (newline == std::string::npos)
                        return std::string("null");
                    lineStart = newline + 1;
                }
                if (lineStart > source.size())
                    return std::string("null");
                const auto lineEnd = source.find('\n', lineStart);
                const std::string lineText = source.substr(
                    lineStart,
                    lineEnd == std::string::npos ? std::string::npos : lineEnd - lineStart);
                const std::size_t cursorOffset =
                    lineStart + byteOffsetForUtf16(lineText, requestedCharacter);

                std::vector<Token> tokens;
                try {
                    tokens = Lexer(source).tokenize();
                } catch (const std::exception&) {
                    return std::string("null");
                }

                struct DelimiterFrame {
                    TokenKind opening = TokenKind::LParen;
                    std::string functionName;
                    std::size_t activeParameter = 0;
                    bool isCall = false;
                };
                std::vector<DelimiterFrame> delimiters;
                for (std::size_t i = 0; i < tokens.size(); ++i) {
                    const Token& token = tokens[i];
                    if (token.kind == TokenKind::End || token.pos.offset >= cursorOffset)
                        break;

                    if (token.kind == TokenKind::LParen) {
                        std::string functionName;
                        if (i > 0 && tokens[i - 1].kind == TokenKind::Identifier) {
                            std::size_t begin = i - 1;
                            while (begin >= 2 && tokens[begin - 1].kind == TokenKind::Dot &&
                                   tokens[begin - 2].kind == TokenKind::Identifier) {
                                begin -= 2;
                            }
                            for (std::size_t part = begin; part < i; ++part)
                                functionName += tokens[part].text;
                        }
                        delimiters.push_back(
                            {TokenKind::LParen, functionName, 0, !functionName.empty()});
                    } else if (token.kind == TokenKind::LBracket) {
                        delimiters.push_back({TokenKind::LBracket, {}, 0, false});
                    } else if (token.kind == TokenKind::LBrace) {
                        delimiters.push_back({TokenKind::LBrace, {}, 0, false});
                    } else if (token.kind == TokenKind::Comma) {
                        if (!delimiters.empty() && delimiters.back().opening == TokenKind::LParen &&
                            delimiters.back().isCall) {
                            ++delimiters.back().activeParameter;
                        }
                    } else if ((token.kind == TokenKind::RParen ||
                                token.kind == TokenKind::RBracket ||
                                token.kind == TokenKind::RBrace) &&
                               !delimiters.empty()) {
                        const TokenKind expected =
                            token.kind == TokenKind::RParen
                                ? TokenKind::LParen
                                : (token.kind == TokenKind::RBracket ? TokenKind::LBracket
                                                                     : TokenKind::LBrace);
                        if (delimiters.back().opening == expected)
                            delimiters.pop_back();
                    }
                }

                const DelimiterFrame* activeCall = nullptr;
                for (auto it = delimiters.rbegin(); it != delimiters.rend(); ++it) {
                    if (it->isCall) {
                        activeCall = &*it;
                        break;
                    }
                }
                if (!activeCall)
                    return std::string("null");

                const auto describeType = [&](const auto& self, const Type& type) -> std::string {
                    switch (type.kind) {
                    case TypeKind::I8: return "i8";
                    case TypeKind::I16: return "i16";
                    case TypeKind::I32: return "i32";
                    case TypeKind::I64: return "i64";
                    case TypeKind::U8: return "u8";
                    case TypeKind::U16: return "u16";
                    case TypeKind::U32: return "u32";
                    case TypeKind::U64: return "u64";
                    case TypeKind::F32: return "f32";
                    case TypeKind::F64: return "f64";
                    case TypeKind::Bool:
                        return "bool";
                    case TypeKind::String:
                        return "string";
                    case TypeKind::Void:
                        return "void";
                    case TypeKind::Unknown:
                        return "unknown";
                    case TypeKind::Struct:
                        return type.name;
                    case TypeKind::Enum:
                        return "enum " + type.name;
                    case TypeKind::Pointer:
                        return type.pointee ? "*" + self(self, *type.pointee) : "*unknown";
                    case TypeKind::Array:
                        return type.element ? "[" + self(self, *type.element) + "; " +
                                                  std::to_string(type.arraySize) + "]"
                                            : "[unknown]";
                    case TypeKind::Slice:
                        return type.element ? "[]" + self(self, *type.element) : "[]unknown";
                    case TypeKind::MutableSlice:
                        return type.element ? "mut []" + self(self, *type.element)
                                            : "mut []unknown";
                    }
                    return "unknown";
                };

                LspFunctionSignatureInfo signature;
                bool foundSignature = false;
                if (const auto builtin = builtinSignature(activeCall->functionName)) {
                    signature.label = activeCall->functionName + "(";
                    for (std::size_t i = 0; i < builtin->params.size(); ++i) {
                        if (i)
                            signature.label += ", ";
                        const std::string parameter =
                            "arg" + std::to_string(i + 1) + ": " +
                            describeType(describeType, builtin->params[i]);
                        signature.parameters.push_back(parameter);
                        signature.label += parameter;
                    }
                    signature.label += ") -> " + describeType(describeType, builtin->ret);
                    foundSignature = true;
                } else {
                    for (const auto& document : collectWorkspaceDocuments(uri, source)) {
                        auto& index = workspaceTypeIndexFor(document.first, document.second);
                        if (!index.functionNames.contains(activeCall->functionName))
                            continue;
                        parseWorkspaceTypeIndex(index);
                        const auto found = index.functionSignatures.find(activeCall->functionName);
                        if (found == index.functionSignatures.end())
                            continue;
                        signature = found->second;
                        foundSignature = true;
                        break;
                    }
                }
                if (!foundSignature)
                    return std::string("null");

                std::ostringstream response;
                response << "{\"signatures\":[{\"label\":\"" << jsonEscape(signature.label)
                         << "\",\"parameters\":[";
                for (std::size_t i = 0; i < signature.parameters.size(); ++i) {
                    if (i)
                        response << ',';
                    response << "{\"label\":\"" << jsonEscape(signature.parameters[i]) << "\"}";
                }
                response << "]}],\"activeSignature\":0";
                if (!signature.parameters.empty()) {
                    const std::size_t activeParameter =
                        std::min(activeCall->activeParameter, signature.parameters.size() - 1);
                    response << ",\"activeParameter\":" << activeParameter;
                }
                response << '}';
                return response.str();
            };

            // Standard LSP symbols support editor Outline and workspace symbol search.
            // Source offsets are bytes; LSP character positions are UTF-16 code units.
            const auto collectDocumentSymbols = [&](const std::string& source) {
                struct SymbolRecord {
                    std::string name;
                    int kind = 12;
                    std::size_t start = 0, end = 0, nameStart = 0, nameEnd = 0;
                };
                std::vector<SymbolRecord> result;
                std::vector<Token> tokens;
                Program program;
                try {
                    tokens = Lexer(source).tokenize();
                    program = Parser(tokens).parseProgram();
                } catch (const std::exception&) {
                    return result;
                }
                const auto add = [&](const std::string& name, int kind, const SourceSpan& span) {
                    std::size_t nameStart = source.size(), nameEnd = source.size();
                    for (const auto& token : tokens) {
                        if (token.kind != TokenKind::Identifier || token.text != name)
                            continue;
                        if (span.valid && (token.pos.offset < span.start.offset ||
                                           token.pos.offset >= span.end.offset))
                            continue;
                        nameStart = token.pos.offset;
                        nameEnd = token.hasEndPos ? token.endPos.offset
                                                  : token.pos.offset + token.text.size();
                        break;
                    }
                    if (nameStart >= source.size() || nameEnd <= nameStart)
                        return;
                    const auto start = span.valid ? span.start.offset : nameStart;
                    const auto end =
                        span.valid && span.end.offset >= start ? span.end.offset : nameEnd;
                    result.push_back({name, kind, start, end, nameStart, nameEnd});
                };
                for (const auto& f : program.functions)
                    add(f.name, 12, f.span);
                for (const auto& st : program.structs)
                    add(st.name, 23, st.span);
                for (const auto& en : program.enums)
                    add(en.name, 10, en.span);
                std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                    return a.start < b.start;
                });
                return result;
            };
            const auto serializeDocumentSymbol = [&](const std::string& source,
                                                     const auto& symbol,
                                                     bool workspaceEntry,
                                                     const std::string& uri) {
                const auto positionAt = [&](std::size_t offset) {
                    offset = std::min(offset, source.size());
                    std::size_t line = 0, lineStart = 0;
                    for (std::size_t i = 0; i < offset; ++i)
                        if (source[i] == '\n') {
                            ++line;
                            lineStart = i + 1;
                        }
                    std::size_t character = 0;
                    for (std::size_t i = lineStart; i < offset;) {
                        const auto first = static_cast<unsigned char>(source[i]);
                        std::size_t width = 1;
                        if ((first & 0xe0) == 0xc0)
                            width = 2;
                        else if ((first & 0xf0) == 0xe0)
                            width = 3;
                        else if ((first & 0xf8) == 0xf0)
                            width = 4;
                        if (i + width > offset)
                            width = 1;
                        character += width == 4 ? 2 : 1;
                        i += width;
                    }
                    return std::pair<std::size_t, std::size_t>{line, character};
                };
                const auto range = [&](std::size_t start, std::size_t end) {
                    const auto a = positionAt(start), b = positionAt(end);
                    std::ostringstream out;
                    out << "{\"start\":{\"line\":" << a.first << ",\"character\":" << a.second
                        << "},\"end\":{\"line\":" << b.first << ",\"character\":" << b.second
                        << "}}";
                    return out.str();
                };
                std::ostringstream out;
                out << "{\"name\":\"" << jsonEscape(symbol.name) << "\",\"kind\":" << symbol.kind;
                if (workspaceEntry)
                    out << ",\"location\":{\"uri\":\"" << jsonEscape(uri)
                        << "\",\"range\":" << range(symbol.start, symbol.end) << "}";
                else
                    out << ",\"range\":" << range(symbol.start, symbol.end)
                        << ",\"selectionRange\":" << range(symbol.nameStart, symbol.nameEnd);
                out << "}";
                return out.str();
            };
            const auto documentSymbolsResult = [&](const std::string& source) {
                const auto symbols = collectDocumentSymbols(source);
                std::ostringstream out;
                out << "[";
                for (std::size_t i = 0; i < symbols.size(); ++i) {
                    if (i)
                        out << ',';
                    out << serializeDocumentSymbol(source, symbols[i], false, std::string{});
                }
                out << "]";
                return out.str();
            };
            const auto workspaceSymbolsResult = [&](const std::string& query) {
                std::map<std::string, std::string> documents;
                for (const auto& open : openDocuments)
                    for (const auto& document : collectWorkspaceDocuments(open.first, open.second))
                        documents.insert_or_assign(document.first, document.second);
                std::ostringstream out;
                out << "[";
                bool first = true;
                std::size_t emitted = 0;
                constexpr std::size_t maxWorkspaceSymbols = 500;
                for (const auto& document : documents) {
                    const auto symbols = collectDocumentSymbols(document.second);
                    for (const auto& symbol : symbols) {
                        if (!query.empty() && symbol.name.find(query) == std::string::npos)
                            continue;
                        if (emitted >= maxWorkspaceSymbols)
                            break;
                        if (!first)
                            out << ',';
                        first = false;
                        out << serializeDocumentSymbol(
                            document.second, symbol, true, document.first);
                        ++emitted;
                    }
                    if (emitted >= maxWorkspaceSymbols)
                        break;
                }
                out << "]";
                return out.str();
            };

            const std::vector<std::pair<std::string, std::string>> completions = {
                {"fn", "Function declaration"},
                {"let", "Variable declaration"},
                {"const", "Constant declaration"},
                {"mut", "Mutable binding modifier"},
                {"if", "Conditional"},
                {"else", "Alternative branch"},
                {"while", "Loop"},
                {"for", "Range or collection loop"},
                {"match", "Pattern matching"},
                {"return", "Return from function"},
                {"struct", "Struct declaration"},
                {"enum", "Enum declaration"},
                {"import std.io", "Import standard input/output"},
                {"import std.fs", "Import standard file system"},
                {"import std.net", "Import standard networking"},
                {"import std.json", "Import standard JSON helpers"},
                {"json_valid", "Validate JSON syntax"},
                {"json_get", "Read a top-level JSON property"},
                {"json_type", "Get a JSON property type"},
                {"json_array_len", "Get a JSON array length"},
                {"json_array_get", "Read a JSON array element"},
                {"json_is_valid", "Validate JSON syntax from std.json"},
                {"json_field", "Read a JSON property from std.json"},
                {"json_value_type", "Get a JSON property type from std.json"},
                {"json_array_length", "Get a JSON array length from std.json"},
                {"json_array_item", "Read a JSON array item from std.json"},
                {"print", "Write a value to stdout"},
                {"input", "Read a line from stdin"},
                {"input_i64", "Read an integer from stdin"},
                {"str_i64", "Convert an integer to a string"},
                {"file_read", "Read a file"},
                {"file_write", "Write a file"},
                {"file_exists", "Check whether a file exists"},
                {"env", "Read an environment variable"},
                {"sleep", "Pause execution"},
                {"gfx_window", "Create a graphics window"},
                {"gfx_clear", "Clear the graphics frame"},
                {"gfx_present", "Present the graphics frame"}};
            std::string header;
            while (std::cin) {
                std::size_t contentLength = 0;
                bool sawHeader = false;
                while (std::getline(std::cin, header)) {
                    if (!header.empty() && header.back() == '\r')
                        header.pop_back();
                    if (header.empty())
                        break;
                    if (header.rfind("Content-Length:", 0) == 0) {
                        const std::string value = trim(header.substr(15));
                        contentLength = static_cast<std::size_t>(std::stoull(value));
                        sawHeader = true;
                    }
                }
                if (!sawHeader)
                    break;
                if (contentLength > 16 * 1024 * 1024) {
                    std::cerr << "NEXUS LSP: message exceeds 16 MiB limit\n";
                    return 2;
                }
                std::string body(contentLength, '\0');
                std::cin.read(body.data(), static_cast<std::streamsize>(contentLength));
                if (static_cast<std::size_t>(std::cin.gcount()) != contentLength) {
                    std::cerr << "NEXUS LSP: incomplete JSON-RPC message\n";
                    return 2;
                }

                const std::string method = extractString(body, "method");
                const std::string id = extractId(body);
                if (method == "exit")
                    break;
                if (method == "initialize" && !id.empty()) {
                    clientSupportsWatchedFilesRegistration = supportsWatchedFilesRegistration(body);
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                         ",\"result\":{\"serverInfo\":{\"name\":\"nexus-lsp\",\"version\":\"" +
                         kVersion +
                         "\"},\"capabilities\":{\"textDocumentSync\":1,"
                         "\"completionProvider\":{\"resolveProvider\":false,\"triggerCharacters\":["
                         "\".\",\":\"]},"
                         "\"signatureHelpProvider\":{\"triggerCharacters\":[\"(\",\",\"]},"
                         "\"definitionProvider\":true,\"referencesProvider\":true,"
                         "\"renameProvider\":true,\"hoverProvider\":true,"
                         "\"documentFormattingProvider\":true,\"documentRangeFormattingProvider\":"
                         "true,\"documentSymbolProvider\":true,\"workspaceSymbolProvider\":true,"
                         "\"codeActionProvider\":{\"codeActionKinds\":[\"quickfix\"]}}}}");
                } else if (method == "initialized") {
                    if (clientSupportsWatchedFilesRegistration) {
                        send("{\"jsonrpc\":\"2.0\",\"id\":\"nexus-register-nx-watcher\","
                             "\"method\":\"client/"
                             "registerCapability\",\"params\":{\"registrations\":[{"
                             "\"id\":\"nexus-watch-nx-workspace\","
                             "\"method\":\"workspace/didChangeWatchedFiles\","
                             "\"registerOptions\":{\"watchers\":[{\"globPattern\":\"**/"
                             "*.nx\",\"kind\":7}]}}]}}");
                    }
                } else if (method == "shutdown" && !id.empty()) {
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":null}");
                } else if (method == "textDocument/didOpen") {
                    const std::string uri = extractString(body, "uri");
                    if (!uri.empty()) {
                        const std::string source = extractString(body, "text");
                        openDocuments[uri] = source;
                        publishDiagnostics(uri, source);
                    }
                } else if (method == "textDocument/didChange") {
                    const std::string uri = extractString(body, "uri");
                    if (!uri.empty()) {
                        // The server advertises full-document synchronization.
                        const std::string source = extractString(body, "text");
                        openDocuments[uri] = source;
                        publishDiagnostics(uri, source);
                    }
                } else if (method == "textDocument/didSave") {
                    const std::string uri = extractString(body, "uri");
                    workspaceDiskCache.clear();
                    workspaceSymbolIndexCache.erase(uri);
                    workspaceTypeDeclarationCache.erase(uri);
                    const auto document = openDocuments.find(uri);
                    if (document != openDocuments.end())
                        publishDiagnostics(uri, document->second);
                } else if (method == "workspace/didChangeWatchedFiles") {
                    // Rebuild source snapshots and discard stale indexes, including
                    // deleted/renamed paths no longer present in the workspace walk.
                    workspaceDiskCache.clear();
                    workspaceSymbolIndexCache.clear();
                    workspaceTypeDeclarationCache.clear();
                } else if (method == "textDocument/didClose") {
                    const std::string uri = extractString(body, "uri");
                    if (!uri.empty()) {
                        openDocuments.erase(uri);
                        workspaceDiskCache.clear();
                        workspaceSymbolIndexCache.erase(uri);
                        workspaceTypeDeclarationCache.erase(uri);
                        send("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\","
                             "\"params\":{\"uri\":\"" +
                             jsonEscape(uri) + "\",\"diagnostics\":[]}}");
                    }

                } else if (method == "textDocument/documentSymbol" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    const std::string result = document == openDocuments.end()
                                                   ? "[]"
                                                   : documentSymbolsResult(document->second);
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if (method == "workspace/symbol" && !id.empty()) {
                    const std::string result = workspaceSymbolsResult(extractString(body, "query"));
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if (method == "textDocument/signatureHelp" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::string result = "null";
                    if (document != openDocuments.end()) {
                        result = signatureHelpResult(document->first,
                                                     document->second,
                                                     extractNumber(body, "line"),
                                                     extractNumber(body, "character"));
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if (method == "textDocument/hover" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::string result = "null";
                    if (document != openDocuments.end()) {
                        const std::string identifier =
                            identifierAtPosition(document->second,
                                                 extractNumber(body, "line"),
                                                 extractNumber(body, "character"));
                        for (const auto& item : completions) {
                            if (item.first == identifier) {
                                const std::string markdown =
                                    "**" + item.first + "**\n\n" + item.second;
                                result = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                                         jsonEscape(markdown) + "\"}}";
                                break;
                            }
                        }

                        // When the cursor is on a user-defined declaration, parse the
                        // current open buffer and show its real signature. Incomplete
                        // editor buffers can fail to parse; hover should then fall back
                        // to null instead of disrupting the LSP session.
                        if (result == "null" && !identifier.empty()) {
                            try {
                                Program program =
                                    Parser(Lexer(document->second).tokenize()).parseProgram();
                                const auto describeType = [&](const auto& self,
                                                              const Type& type) -> std::string {
                                    switch (type.kind) {
                                    case TypeKind::I8: return "i8";
                                    case TypeKind::I16: return "i16";
                                    case TypeKind::I32: return "i32";
                                    case TypeKind::I64: return "i64";
                                    case TypeKind::U8: return "u8";
                                    case TypeKind::U16: return "u16";
                                    case TypeKind::U32: return "u32";
                                    case TypeKind::U64: return "u64";
                                    case TypeKind::F32: return "f32";
                                    case TypeKind::F64: return "f64";
                                    case TypeKind::Bool:
                                        return "bool";
                                    case TypeKind::String:
                                        return "string";
                                    case TypeKind::Void:
                                        return "void";
                                    case TypeKind::Pointer:
                                        return type.pointee ? "*" + self(self, *type.pointee)
                                                            : "*unknown";
                                    case TypeKind::Struct:
                                        return type.name.empty() ? "struct" : type.name;
                                    case TypeKind::Enum:
                                        return type.name.empty() ? "enum" : type.name;
                                    case TypeKind::Array:
                                        return type.element
                                                   ? "[" + self(self, *type.element) + "; " +
                                                         std::to_string(type.arraySize) + "]"
                                                   : "[unknown]";
                                    case TypeKind::Slice:
                                        return type.element ? "[]" + self(self, *type.element)
                                                            : "[]unknown";
                                    case TypeKind::MutableSlice:
                                        return type.element ? "mut []" + self(self, *type.element)
                                                            : "mut []unknown";
                                    case TypeKind::Unknown:
                                        return "inferred";
                                    }
                                    return "unknown";
                                };
                                std::string markdown;
                                for (const auto& function : program.functions) {
                                    if (function.name != identifier)
                                        continue;
                                    std::ostringstream signature;
                                    signature << "fn " << function.name << "(";
                                    for (std::size_t i = 0; i < function.params.size(); ++i) {
                                        if (i)
                                            signature << ", ";
                                        signature
                                            << function.params[i].name << ": "
                                            << describeType(describeType, function.params[i].type);
                                    }
                                    signature << ") -> "
                                              << describeType(describeType, function.ret);
                                    markdown =
                                        "**" + signature.str() + "**\n\nUser-defined function.";
                                    break;
                                }
                                if (markdown.empty()) {
                                    for (const auto& declaration : program.structs) {
                                        if (declaration.name != identifier)
                                            continue;
                                        std::ostringstream description;
                                        description << "**struct " << declaration.name << "**";
                                        if (declaration.fields.empty()) {
                                            description << "\n\nNo fields.";
                                        } else {
                                            description << "\n\nFields:";
                                            for (const auto& field : declaration.fields) {
                                                description
                                                    << "\n- `" << field.name << "`: "
                                                    << describeType(describeType, field.type)
                                                    << "`";
                                            }
                                        }
                                        markdown = description.str();
                                        break;
                                    }
                                }
                                if (markdown.empty()) {
                                    for (const auto& declaration : program.enums) {
                                        if (declaration.name != identifier)
                                            continue;
                                        std::ostringstream description;
                                        description << "**enum " << declaration.name << "**";
                                        if (declaration.variants.empty()) {
                                            description << "\n\nNo variants.";
                                        } else {
                                            description << "\n\nVariants:";
                                            for (const auto& variant : declaration.variants) {
                                                description << "\n- `" << variant.name << "`";
                                            }
                                        }
                                        markdown = description.str();
                                        break;
                                    }
                                }
                                if (!markdown.empty()) {
                                    result = "{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                                             jsonEscape(markdown) + "\"}}";
                                }
                            } catch (const std::exception&) {
                                // Unsaved/incomplete source is normal while an editor is open.
                            }
                        }

                        // Resolve global declarations from unopened or open workspace files.
                        if (result == "null" && !identifier.empty()) {
                            const auto occurrences =
                                findIdentifierOccurrences(identifier,
                                                          uri,
                                                          document->second,
                                                          extractNumber(body, "line"),
                                                          extractNumber(body, "character"));
                            const bool declaredElsewhere = std::any_of(
                                occurrences.begin(),
                                occurrences.end(),
                                [&](const LspIdentifierOccurrence& occurrence) {
                                    return occurrence.isDeclaration && occurrence.uri != uri;
                                });
                            if (declaredElsewhere) {
                                const auto describeWorkspaceHoverType =
                                    [&](const auto& self, const Type& type) -> std::string {
                                    switch (type.kind) {
                                    case TypeKind::I8: return "i8";
                                    case TypeKind::I16: return "i16";
                                    case TypeKind::I32: return "i32";
                                    case TypeKind::I64: return "i64";
                                    case TypeKind::U8: return "u8";
                                    case TypeKind::U16: return "u16";
                                    case TypeKind::U32: return "u32";
                                    case TypeKind::U64: return "u64";
                                    case TypeKind::F32: return "f32";
                                    case TypeKind::F64: return "f64";
                                    case TypeKind::Bool:
                                        return "bool";
                                    case TypeKind::String:
                                        return "string";
                                    case TypeKind::Void:
                                        return "void";
                                    case TypeKind::Pointer:
                                        return type.pointee ? "*" + self(self, *type.pointee)
                                                            : "*unknown";
                                    case TypeKind::Struct:
                                        return type.name.empty() ? "struct" : type.name;
                                    case TypeKind::Enum:
                                        return type.name.empty() ? "enum" : type.name;
                                    case TypeKind::Array:
                                        return type.element
                                                   ? "[" + self(self, *type.element) + "; " +
                                                         std::to_string(type.arraySize) + "]"
                                                   : "[unknown]";
                                    case TypeKind::Slice:
                                        return type.element ? "[]" + self(self, *type.element)
                                                            : "[]unknown";
                                    case TypeKind::MutableSlice:
                                        return type.element ? "mut []" + self(self, *type.element)
                                                            : "mut []unknown";
                                    case TypeKind::Unknown:
                                        return "inferred";
                                    }
                                    return "unknown";
                                };
                                for (const auto& [candidateUri, candidateSource] :
                                     collectWorkspaceDocuments(uri, document->second)) {
                                    if (candidateUri == uri)
                                        continue;
                                    auto& index =
                                        workspaceTypeIndexFor(candidateUri, candidateSource);
                                    if (!index.functionNames.contains(identifier) &&
                                        !index.structNames.contains(identifier) &&
                                        !index.enumNames.contains(identifier)) {
                                        continue;
                                    }

                                    parseWorkspaceTypeIndex(index);
                                    std::string markdown;
                                    const auto function =
                                        index.functionHoverMarkdown.find(identifier);
                                    if (function != index.functionHoverMarkdown.end())
                                        markdown = function->second;

                                    if (markdown.empty()) {
                                        for (const auto& declaration : index.structs) {
                                            if (declaration.name != identifier)
                                                continue;
                                            std::ostringstream description;
                                            description << "**struct " << declaration.name << "**";
                                            if (declaration.fields.empty()) {
                                                description << "\n\nNo fields.";
                                            } else {
                                                description << "\n\nFields:";
                                                for (const auto& field : declaration.fields) {
                                                    description << "\n- " << static_cast<char>(96)
                                                                << field.name
                                                                << static_cast<char>(96) << ": "
                                                                << describeWorkspaceHoverType(
                                                                       describeWorkspaceHoverType,
                                                                       field.type);
                                                }
                                            }
                                            markdown = description.str();
                                            break;
                                        }
                                    }
                                    if (markdown.empty()) {
                                        for (const auto& declaration : index.enums) {
                                            if (declaration.name != identifier)
                                                continue;
                                            std::ostringstream description;
                                            description << "**enum " << declaration.name << "**";
                                            if (declaration.variants.empty()) {
                                                description << "\n\nNo variants.";
                                            } else {
                                                description << "\n\nVariants:";
                                                for (const auto& variant : declaration.variants)
                                                    description << "\n- " << static_cast<char>(96)
                                                                << variant.name << static_cast<char>(96);
                                            }
                                            markdown = description.str();
                                            break;
                                        }
                                    }
                                    if (!markdown.empty()) {
                                        result =
                                            "{\"contents\":{\"kind\":\"markdown\",\"value\":\"" +
                                            jsonEscape(markdown) + "\"}}";
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if ((method == "textDocument/completion" || method == "completion") &&
                           !id.empty()) {
                    std::vector<std::pair<std::string, std::string>> choices;
                    std::string prefix, objectName, enumName;
                    bool memberContext = false, enumContext = false;
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::size_t currentLine = 0, currentCharacter = 0, absoluteCursor = 0;
                    std::string currentLineText;
                    if (document != openDocuments.end()) {
                        currentLine = extractNumber(body, "line");
                        currentCharacter = extractNumber(body, "character");
                        const std::string& source = document->second;
                        std::size_t lineStart = 0;
                        for (std::size_t i = 0; i < currentLine; ++i) {
                            const auto newline = source.find('\n', lineStart);
                            if (newline == std::string::npos) {
                                lineStart = source.size();
                                break;
                            }
                            lineStart = newline + 1;
                        }
                        if (lineStart <= source.size()) {
                            const auto newline = source.find('\n', lineStart);
                            currentLineText =
                                source.substr(lineStart,
                                              newline == std::string::npos ? std::string::npos
                                                                           : newline - lineStart);
                            const std::size_t cursor =
                                std::min(byteOffsetForUtf16(currentLineText, currentCharacter),
                                         currentLineText.size());
                            absoluteCursor = lineStart + cursor;
                            std::size_t prefixStart = cursor;
                            while (prefixStart > 0 &&
                                   identifierCharacter(static_cast<unsigned char>(
                                       currentLineText[prefixStart - 1])))
                                --prefixStart;
                            prefix = currentLineText.substr(prefixStart, cursor - prefixStart);

                            if (prefixStart > 0 && currentLineText[prefixStart - 1] == '.') {
                                memberContext = true;
                                std::size_t objectEnd = prefixStart - 1;
                                while (objectEnd > 0 && std::isspace(static_cast<unsigned char>(
                                                            currentLineText[objectEnd - 1])))
                                    --objectEnd;
                                std::size_t objectStart = objectEnd;
                                while (objectStart > 0 &&
                                       identifierCharacter(static_cast<unsigned char>(
                                           currentLineText[objectStart - 1])))
                                    --objectStart;
                                while (objectStart > 0) {
                                    std::size_t boundary = objectStart;
                                    while (boundary > 0 && std::isspace(static_cast<unsigned char>(
                                                               currentLineText[boundary - 1])))
                                        --boundary;
                                    if (boundary == 0 || currentLineText[boundary - 1] != '.')
                                        break;
                                    --boundary;
                                    while (boundary > 0 && std::isspace(static_cast<unsigned char>(
                                                               currentLineText[boundary - 1])))
                                        --boundary;
                                    std::size_t previousStart = boundary;
                                    while (previousStart > 0 &&
                                           identifierCharacter(static_cast<unsigned char>(
                                               currentLineText[previousStart - 1])))
                                        --previousStart;
                                    if (previousStart == boundary)
                                        break;
                                    objectStart = previousStart;
                                }
                                objectName =
                                    currentLineText.substr(objectStart, objectEnd - objectStart);
                            } else if (prefixStart >= 2 &&
                                       currentLineText.substr(prefixStart - 2, 2) == "::") {
                                enumContext = true;
                                const std::size_t separator = prefixStart - 2;
                                std::size_t typeStart = separator;
                                while (typeStart > 0 &&
                                       identifierCharacter(static_cast<unsigned char>(
                                           currentLineText[typeStart - 1])))
                                    --typeStart;
                                enumName = currentLineText.substr(typeStart, separator - typeStart);
                            }
                        }
                    }

                    bool contextualSuggestions = false;
                    if (document != openDocuments.end() && (memberContext || enumContext)) {
                        try {
                            std::string parseSource = document->second;
                            parseSource.insert(absoluteCursor, "__nexus_completion");
                            Program program = Parser(Lexer(parseSource).tokenize()).parseProgram();

                            if (enumContext) {
                                std::vector<EnumDecl> enumDeclarations = program.enums;
                                const bool hasLocalEnum =
                                    std::any_of(enumDeclarations.begin(),
                                                enumDeclarations.end(),
                                                [&](const EnumDecl& declaration) {
                                                    return declaration.name == enumName;
                                                });
                                if (!hasLocalEnum) {
                                    auto workspaceTypes = workspaceTypeDeclarationsFor(
                                        uri, document->second, enumName, true);
                                    enumDeclarations.insert(enumDeclarations.end(),
                                                            workspaceTypes.second.begin(),
                                                            workspaceTypes.second.end());
                                }
                                for (const auto& declaration : enumDeclarations) {
                                    if (declaration.name != enumName)
                                        continue;
                                    for (const auto& variant : declaration.variants) {
                                        const std::string label = declaration.name + "::" + variant.name;
                                        if (prefix.empty() ||
                                            label.rfind(enumName + "::" + prefix, 0) == 0)
                                            choices.emplace_back(label, "Variant of " + enumName);
                                    }
                                }
                                contextualSuggestions = !choices.empty();
                            } else {
                                std::map<std::string, std::string> variableTypes;
                                for (const auto& function : program.functions) {
                                    for (const auto& parameter : function.params) {
                                        if (!parameter.type.name.empty())
                                            variableTypes[parameter.name] = parameter.type.name;
                                    }
                                }
                                const auto collectTypes = [&](const auto& self,
                                                              const Block& block) -> void {
                                    for (const auto& statement : block.statements) {
                                        if (const auto* declaration =
                                                dynamic_cast<const LetStmt*>(statement.get())) {
                                            std::string typeName = declaration->type.name;
                                            if (const auto* init =
                                                    dynamic_cast<const StructInitExpr*>(
                                                        declaration->init.get()))
                                                typeName = init->typeName;
                                            else if (const auto* init =
                                                         dynamic_cast<const EnumVariantExpr*>(
                                                             declaration->init.get()))
                                                typeName = init->enumName;
                                            else if (const auto* call =
                                                         dynamic_cast<const CallExpr*>(
                                                             declaration->init.get())) {
                                                for (const auto& function : program.functions) {
                                                    if (function.name == call->callee &&
                                                        function.ret.kind == TypeKind::Struct &&
                                                        !function.ret.name.empty()) {
                                                        typeName = function.ret.name;
                                                        break;
                                                    }
                                                }
                                                if (typeName.empty()) {
                                                    typeName = workspaceFunctionReturnTypeFor(
                                                        uri, document->second, call->callee);
                                                }
                                            }
                                            if (!typeName.empty())
                                                variableTypes[declaration->name] = typeName;
                                        } else if (const auto* conditional =
                                                       dynamic_cast<const IfStmt*>(
                                                           statement.get())) {
                                            if (conditional->thenBlock)
                                                self(self, *conditional->thenBlock);
                                            if (conditional->elseBlock)
                                                self(self, *conditional->elseBlock);
                                        } else if (const auto* loop =
                                                       dynamic_cast<const WhileStmt*>(
                                                           statement.get())) {
                                            if (loop->body)
                                                self(self, *loop->body);
                                        } else if (const auto* match =
                                                       dynamic_cast<const MatchStmt*>(
                                                           statement.get())) {
                                            for (const auto& arm : match->arms)
                                                if (arm.body)
                                                    self(self, *arm.body);
                                        } else if (const auto* nested = dynamic_cast<const Block*>(
                                                       statement.get())) {
                                            self(self, *nested);
                                        }
                                    }
                                };
                                for (const auto& function : program.functions)
                                    if (function.body)
                                        collectTypes(collectTypes, *function.body);

                                const auto structFieldType = [&](const std::string& typeName,
                                                                 const std::string& fieldName) {
                                    const auto local =
                                        std::find_if(program.structs.begin(),
                                                     program.structs.end(),
                                                     [&](const StructDecl& declaration) {
                                                         return declaration.name == typeName;
                                                     });
                                    if (local != program.structs.end()) {
                                        const auto field =
                                            std::find_if(local->fields.begin(),
                                                         local->fields.end(),
                                                         [&](const StructField& candidate) {
                                                             return candidate.name == fieldName;
                                                         });
                                        if (field != local->fields.end() &&
                                            field->type.kind == TypeKind::Struct)
                                            return field->type.name;
                                        return std::string{};
                                    }

                                    const auto workspaceTypes = workspaceTypeDeclarationsFor(
                                        uri, document->second, typeName, false);
                                    for (const auto& declaration : workspaceTypes.first) {
                                        if (declaration.name != typeName)
                                            continue;
                                        const auto field =
                                            std::find_if(declaration.fields.begin(),
                                                         declaration.fields.end(),
                                                         [&](const StructField& candidate) {
                                                             return candidate.name == fieldName;
                                                         });
                                        if (field != declaration.fields.end() &&
                                            field->type.kind == TypeKind::Struct)
                                            return field->type.name;
                                    }
                                    return std::string{};
                                };

                                std::string resolvedTypeName;
                                const auto directType = variableTypes.find(objectName);
                                if (directType != variableTypes.end()) {
                                    resolvedTypeName = directType->second;
                                } else {
                                    const std::size_t firstDot = objectName.find('.');
                                    if (firstDot != std::string::npos) {
                                        const auto rootType =
                                            variableTypes.find(objectName.substr(0, firstDot));
                                        if (rootType != variableTypes.end()) {
                                            resolvedTypeName = rootType->second;
                                            std::size_t fieldStart = firstDot + 1;
                                            while (fieldStart < objectName.size() &&
                                                   !resolvedTypeName.empty()) {
                                                const std::size_t fieldEnd =
                                                    objectName.find('.', fieldStart);
                                                const std::string fieldName =
                                                    objectName.substr(fieldStart,
                                                                      fieldEnd == std::string::npos
                                                                          ? std::string::npos
                                                                          : fieldEnd - fieldStart);
                                                resolvedTypeName =
                                                    structFieldType(resolvedTypeName, fieldName);
                                                if (fieldEnd == std::string::npos)
                                                    break;
                                                fieldStart = fieldEnd + 1;
                                            }
                                        }
                                    }
                                }

                                if (!resolvedTypeName.empty()) {
                                    std::vector<StructDecl> structureDeclarations = program.structs;
                                    const bool hasLocalStruct =
                                        std::any_of(structureDeclarations.begin(),
                                                    structureDeclarations.end(),
                                                    [&](const StructDecl& declaration) {
                                                        return declaration.name == resolvedTypeName;
                                                    });
                                    const bool builtinType =
                                        resolvedTypeName == "i64" || resolvedTypeName == "int" ||
                                        resolvedTypeName == "isize" ||
                                        resolvedTypeName == "usize" || resolvedTypeName == "f64" ||
                                        resolvedTypeName == "float" || resolvedTypeName == "bool" ||
                                        resolvedTypeName == "string" || resolvedTypeName == "void";
                                    if (!hasLocalStruct && !builtinType) {
                                        auto workspaceTypes = workspaceTypeDeclarationsFor(
                                            uri, document->second, resolvedTypeName, false);
                                        structureDeclarations.insert(structureDeclarations.end(),
                                                                     workspaceTypes.first.begin(),
                                                                     workspaceTypes.first.end());
                                    }
                                    for (const auto& declaration : structureDeclarations) {
                                        if (declaration.name != resolvedTypeName)
                                            continue;
                                        for (const auto& field : declaration.fields) {
                                            if (prefix.empty() || field.name.rfind(prefix, 0) == 0)
                                                choices.emplace_back(
                                                    field.name, "Field of " + declaration.name);
                                        }
                                    }
                                    contextualSuggestions = !choices.empty();
                                }
                            }
                        } catch (const std::exception&) {
                            // Incomplete edits are normal in an editor; fall back to prefix
                            // completion rather than surfacing parser errors from this request.
                        }
                    }

                    if (!contextualSuggestions) {
                        for (const auto& item : completions) {
                            if (prefix.empty() || item.first.rfind(prefix, 0) == 0)
                                choices.push_back(item);
                        }
                    }

                    std::ostringstream result;
                    result << "{\"jsonrpc\":\"2.0\",\"id\":" << id << ",\"result\":[";
                    for (std::size_t i = 0; i < choices.size(); ++i) {
                        if (i)
                            result << ',';
                        const int kind =
                            choices[i].second.rfind("Field of ", 0) == 0
                                ? 5
                                : (choices[i].second.rfind("Variant of ", 0) == 0 ? 20 : 14);
                        result << "{\"label\":\"" << jsonEscape(choices[i].first)
                               << "\",\"kind\":" << kind << ",\"detail\":\""
                               << jsonEscape(choices[i].second) << "\"}";
                    }
                    result << "]}";
                    send(result.str());
                } else if (method == "textDocument/formatting" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto doc = openDocuments.find(uri);
                    std::string result = "[]";
                    if (doc != openDocuments.end()) {
                        const std::string formatted = formatNexusSource(doc->second);
                        if (formatted != doc->second) {
                            const std::size_t endLine = static_cast<std::size_t>(
                                std::count(doc->second.begin(), doc->second.end(), '\n'));
                            std::size_t endCharacter = 0;
                            if (!doc->second.empty() && doc->second.back() != '\n') {
                                const auto lastNewline = doc->second.find_last_of('\n');
                                const std::size_t lineStart =
                                    lastNewline == std::string::npos ? 0 : lastNewline + 1;
                                endCharacter = utf16Length(doc->second.substr(lineStart));
                            }
                            result = "[{\"range\":{\"start\":{\"line\":0,\"character\":0},"
                                     "\"end\":{\"line\":" +
                                     std::to_string(endLine) +
                                     ",\"character\":" + std::to_string(endCharacter) +
                                     "}},\"newText\":\"" + jsonEscape(formatted) + "\"}]";
                        }
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");

                } else if (method == "textDocument/rangeFormatting" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto doc = openDocuments.find(uri);
                    std::string result = "[]";
                    if (doc != openDocuments.end()) {
                        const std::size_t rangePos = body.find("\"range\"");
                        const std::size_t startPos = rangePos == std::string::npos
                                                         ? std::string::npos
                                                         : body.find("\"start\"", rangePos);
                        const std::size_t endPos = startPos == std::string::npos
                                                       ? std::string::npos
                                                       : body.find("\"end\"", startPos + 7);
                        const auto nestedNumber = [&](std::size_t objectPos,
                                                      const std::string& field) {
                            if (objectPos == std::string::npos)
                                return std::size_t{0};
                            const std::size_t objectEnd = body.find('}', objectPos);
                            const std::string key = "\"" + field + "\"";
                            const std::size_t keyPos = body.find(key, objectPos);
                            if (keyPos == std::string::npos ||
                                (objectEnd != std::string::npos && keyPos >= objectEnd))
                                return std::size_t{0};
                            const std::size_t colon = body.find(':', keyPos + key.size());
                            if (colon == std::string::npos ||
                                (objectEnd != std::string::npos && colon >= objectEnd))
                                return std::size_t{0};
                            std::size_t pos = colon + 1;
                            while (pos < body.size() &&
                                   std::isspace(static_cast<unsigned char>(body[pos])))
                                ++pos;
                            const std::size_t begin = pos;
                            while (pos < body.size() &&
                                   std::isdigit(static_cast<unsigned char>(body[pos])))
                                ++pos;
                            if (begin == pos)
                                return std::size_t{0};
                            try {
                                return static_cast<std::size_t>(
                                    std::stoull(body.substr(begin, pos - begin)));
                            } catch (const std::exception&) {
                                return std::size_t{0};
                            }
                        };

                        if (startPos != std::string::npos && endPos != std::string::npos) {
                            const std::size_t firstLine = nestedNumber(startPos, "line");
                            const std::size_t requestedEndLine = nestedNumber(endPos, "line");
                            const std::size_t requestedEndCharacter =
                                nestedNumber(endPos, "character");


                            const std::string& source = doc->second;
                            const std::string formatted = formatNexusSource(source);
                            const auto sourceLines = splitLines(source);
                            auto formattedLines = splitLines(formatted);
                            if (!source.empty() && source.back() != '\n' &&
                                formattedLines.size() == sourceLines.size() + 1 &&
                                formattedLines.back().empty()) {
                                formattedLines.pop_back();
                            }

                            if (formattedLines.size() == sourceLines.size() &&
                                firstLine < sourceLines.size() &&
                                requestedEndLine < sourceLines.size() &&
                                firstLine <= requestedEndLine) {
                                std::size_t lastLine = requestedEndLine;
                                if (lastLine > firstLine && requestedEndCharacter == 0)
                                    --lastLine;


                                const std::string oldText =
                                    joinRange(sourceLines, firstLine, lastLine);
                                const std::string newText =
                                    joinRange(formattedLines, firstLine, lastLine);
                                if (oldText != newText) {
                                    result = "[{\"range\":{\"start\":{\"line\":" +
                                             std::to_string(firstLine) +
                                             ",\"character\":0},\"end\":{\"line\":" +
                                             std::to_string(lastLine) + ",\"character\":" +
                                             std::to_string(utf16Length(sourceLines[lastLine])) +
                                             "}},\"newText\":\"" + jsonEscape(newText) + "\"}]";
                                }
                            }
                        }
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if (method == "textDocument/codeAction" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::ostringstream actions;
                    actions << '[';
                    bool firstAction = true;
                    if (document != openDocuments.end()) {
                        try {
                            const auto program =
                                Parser(Lexer(document->second).tokenize()).parseProgram();
                            const auto diagnostics = collectDiagnostics(document->second);

                            for (const auto& diagnostic : diagnostics) {
                                if (diagnostic.code != "NX2001")
                                    continue;

                                const std::string variablePrefix = "unknown variable: ";
                                if (diagnostic.message.rfind(variablePrefix, 0) == 0) {
                                    const std::string misspelled =
                                        trim(diagnostic.message.substr(variablePrefix.size()));
                                    (void)findIdentifierOccurrences(misspelled,
                                                                    uri,
                                                                    document->second,
                                                                    diagnostic.line,
                                                                    diagnostic.character);
                                    const auto cached = workspaceSymbolIndexCache.find(uri);
                                    if (cached != workspaceSymbolIndexCache.end() &&
                                        cached->second.source == document->second) {
                                        const auto& index = cached->second;
                                        std::size_t useToken = index.tokens.size();
                                        for (std::size_t i = 0; i < index.tokens.size(); ++i) {
                                            const auto& token = index.tokens[i];
                                            if (token.kind == TokenKind::Identifier &&
                                                token.text == misspelled &&
                                                token.pos.line == diagnostic.line + 1 &&
                                                token.pos.column > 0 &&
                                                token.pos.column - 1 == diagnostic.character) {
                                                useToken = i;
                                                break;
                                            }
                                        }

                                        if (useToken < index.tokens.size()) {
                                            const auto isAncestor = [&](std::size_t ancestor,
                                                                        std::size_t scope) {
                                                while (scope != static_cast<std::size_t>(-1) &&
                                                       scope < index.scopes.size()) {
                                                    if (scope == ancestor)
                                                        return true;
                                                    scope = index.scopes[scope].parent;
                                                }
                                                return false;
                                            };
                                            std::set<std::string> candidates;
                                            const std::size_t useScope = index.tokenScope[useToken];
                                            for (const auto& declaration : index.declarations) {
                                                if (declaration.global ||
                                                    (declaration.kind != "variable" &&
                                                     declaration.kind != "constant" &&
                                                     declaration.kind != "parameter") ||
                                                    declaration.name == misspelled ||
                                                    declaration.visibleFrom > useToken ||
                                                    !isAncestor(declaration.scope, useScope))
                                                    continue;
                                                candidates.insert(declaration.name);
                                            }

                                            std::string bestName;
                                            std::size_t bestDistance =
                                                std::numeric_limits<std::size_t>::max();
                                            bool ambiguous = false;
                                            for (const auto& candidate : candidates) {
                                                const std::size_t distance =
                                                    editDistance(misspelled, candidate);
                                                if (distance < bestDistance) {
                                                    bestDistance = distance;
                                                    bestName = candidate;
                                                    ambiguous = false;
                                                } else if (distance == bestDistance &&
                                                           candidate != bestName) {
                                                    ambiguous = true;
                                                }
                                            }
                                            const std::size_t maxDistance =
                                                misspelled.size() <= 4 ? 1 : 2;
                                            if (!bestName.empty() && bestName != misspelled &&
                                                !ambiguous && bestDistance <= maxDistance) {
                                                if (!firstAction)
                                                    actions << ',';
                                                firstAction = false;
                                                const std::size_t length =
                                                    std::max<std::size_t>(1, diagnostic.length);
                                                actions << "{\"title\":\"Replace '"
                                                        << jsonEscape(misspelled) << "' with '"
                                                        << jsonEscape(bestName)
                                                        << "'\",\"kind\":\"quickfix\",\"edit\":{"
                                                           "\"changes\":{\""
                                                        << jsonEscape(uri)
                                                        << "\":[{\"range\":{\"start\":{\"line\":"
                                                        << diagnostic.line
                                                        << ",\"character\":" << diagnostic.character
                                                        << "},\"end\":{\"line\":" << diagnostic.line
                                                        << ",\"character\":"
                                                        << diagnostic.character + length
                                                        << "}},\"newText\":\""
                                                        << jsonEscape(bestName) << "\"}]}}}";
                                            }
                                        }
                                    }
                                    continue;
                                }

                                const std::string prefix = "unknown field '";
                                const auto start = diagnostic.message.find(prefix);
                                if (start == std::string::npos)
                                    continue;
                                const auto nameStart = start + prefix.size();
                                const auto nameEnd = diagnostic.message.find('\'', nameStart);
                                if (nameEnd == std::string::npos || nameEnd == nameStart)
                                    continue;
                                const std::string misspelled =
                                    diagnostic.message.substr(nameStart, nameEnd - nameStart);
                                std::string requiredType;
                                const auto onType = diagnostic.message.rfind(" on ");
                                if (onType != std::string::npos)
                                    requiredType = diagnostic.message.substr(onType + 4);

                                std::string bestField;
                                std::size_t bestDistance = std::numeric_limits<std::size_t>::max();
                                bool ambiguous = false;
                                for (const auto& structure : program.structs) {
                                    if (!requiredType.empty() && structure.name != requiredType)
                                        continue;
                                    for (const auto& field : structure.fields) {
                                        const auto distance = editDistance(misspelled, field.name);
                                        if (distance < bestDistance) {
                                            bestDistance = distance;
                                            bestField = field.name;
                                            ambiguous = false;
                                        } else if (distance == bestDistance &&
                                                   field.name != bestField) {
                                            ambiguous = true;
                                        }
                                    }
                                }
                                const std::size_t maxDistance = misspelled.size() <= 4 ? 1 : 2;
                                if (bestField.empty() || bestField == misspelled || ambiguous ||
                                    bestDistance > maxDistance)
                                    continue;

                                if (!firstAction)
                                    actions << ',';
                                firstAction = false;
                                const std::size_t length =
                                    std::max<std::size_t>(1, diagnostic.length);
                                actions << "{\"title\":\"Replace '" << jsonEscape(misspelled)
                                        << "' with '" << jsonEscape(bestField)
                                        << "'\",\"kind\":\"quickfix\",\"edit\":{\"changes\":{\""
                                        << jsonEscape(uri)
                                        << "\":[{\"range\":{\"start\":{\"line\":" << diagnostic.line
                                        << ",\"character\":" << diagnostic.character
                                        << "},\"end\":{\"line\":" << diagnostic.line
                                        << ",\"character\":" << diagnostic.character + length
                                        << "}},\"newText\":\"" << jsonEscape(bestField)
                                        << "\"}]}}}";
                            }
                        } catch (const std::exception&) {
                            // Quick fixes are best-effort; malformed or incomplete source
                            // must not crash the language server.
                        }
                    }
                    actions << ']';
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + actions.str() +
                         "}");
                } else if (method == "textDocument/references" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    std::string result = "[]";
                    if (document != openDocuments.end()) {
                        const std::string identifier =
                            identifierAtPosition(document->second,
                                                 extractNumber(body, "line"),
                                                 extractNumber(body, "character"));
                        const auto occurrences =
                            findIdentifierOccurrences(identifier,
                                                      uri,
                                                      document->second,
                                                      extractNumber(body, "line"),
                                                      extractNumber(body, "character"));
                        std::ostringstream payload;
                        payload << "[";
                        for (std::size_t i = 0; i < occurrences.size(); ++i) {
                            if (i)
                                payload << ',';
                            const auto& occurrence = occurrences[i];
                            payload << "{\"uri\":\"" << jsonEscape(occurrence.uri)
                                    << "\",\"range\":{\"start\":{\"line\":" << occurrence.line
                                    << ",\"character\":" << occurrence.startCharacter
                                    << "},\"end\":{\"line\":" << occurrence.line
                                    << ",\"character\":" << occurrence.endCharacter << "}}}";
                        }
                        payload << "]";
                        result = payload.str();
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if (method == "textDocument/rename" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto document = openDocuments.find(uri);
                    const std::string newName = extractString(body, "newName");
                    if (!isValidIdentifier(newName)) {
                        send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                             ",\"error\":{\"code\":-32602,\"message\":\"newName must be a valid "
                             "NEXUS identifier\"}}");
                    } else {
                        std::vector<LspIdentifierOccurrence> occurrences;
                        if (document != openDocuments.end()) {
                            const std::string identifier =
                                identifierAtPosition(document->second,
                                                     extractNumber(body, "line"),
                                                     extractNumber(body, "character"));
                            occurrences =
                                findIdentifierOccurrences(identifier,
                                                          uri,
                                                          document->second,
                                                          extractNumber(body, "line"),
                                                          extractNumber(body, "character"));
                        }
                        std::set<std::string> uris;
                        for (const auto& occurrence : occurrences)
                            uris.insert(occurrence.uri);
                        std::ostringstream changes;
                        changes << "{\"changes\":{";
                        bool firstUri = true;
                        for (const auto& changedUri : uris) {
                            if (!firstUri)
                                changes << ',';
                            firstUri = false;
                            changes << "\"" << jsonEscape(changedUri) << "\":[";
                            bool firstEdit = true;
                            for (const auto& occurrence : occurrences) {
                                if (occurrence.uri != changedUri)
                                    continue;
                                if (!firstEdit)
                                    changes << ',';
                                firstEdit = false;
                                changes << "{\"range\":{\"start\":{\"line\":" << occurrence.line
                                        << ",\"character\":" << occurrence.startCharacter
                                        << "},\"end\":{\"line\":" << occurrence.line
                                        << ",\"character\":" << occurrence.endCharacter
                                        << "}},\"newText\":\"" << jsonEscape(newName) << "\"}";
                            }
                            changes << "]";
                        }
                        changes << "}}";
                        send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + changes.str() +
                             "}");
                    }
                } else if (method == "textDocument/definition" && !id.empty()) {
                    const std::string uri = extractString(body, "uri");
                    const auto doc = openDocuments.find(uri);
                    std::string result = "null";
                    const std::size_t requestedLine = extractNumber(body, "line");
                    const std::size_t requestedCharacter = extractNumber(body, "character");
                    if (doc != openDocuments.end()) {
                        const std::string identifier =
                            identifierAtPosition(doc->second, requestedLine, requestedCharacter);
                        const auto occurrences = findIdentifierOccurrences(
                            identifier, uri, doc->second, requestedLine, requestedCharacter);
                        for (const auto& occurrence : occurrences) {
                            if (!occurrence.isDeclaration)
                                continue;
                            std::ostringstream location;
                            location << "{\"uri\":\"" << jsonEscape(occurrence.uri)
                                     << "\",\"range\":{\"start\":{\"line\":" << occurrence.line
                                     << ",\"character\":" << occurrence.startCharacter
                                     << "},\"end\":{\"line\":" << occurrence.line
                                     << ",\"character\":" << occurrence.endCharacter << "}}}";
                            result = location.str();
                            break;
                        }
                    }
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}");
                } else if (!method.empty() && !id.empty()) {
                    send("{\"jsonrpc\":\"2.0\",\"id\":" + id +
                         ",\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}");
                }
            }
            return 0;
        }

    } // namespace driver
} // namespace nexus
