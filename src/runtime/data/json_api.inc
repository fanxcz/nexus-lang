bool nexus_json_valid(const char* json) {
    std::string source;
    NxJsonValue root;
    return nx_parse_json_source(json, source, root);
}
char* nexus_json_get(const char* json, const char* key) {
    std::string source;
    NxJsonValue root;
    if (!key || !nx_parse_json_source(json, source, root) ||
        root.kind != NxJsonValue::Kind::Object)
        return nx_strdup("");
    const NxJsonValue* value = nx_json_find_member(root, key);
    if (!value)
        return nx_strdup("");
    const std::string result = nx_json_value_text(source, *value);
    return nx_strdup(result.c_str());
}
char* nexus_json_type(const char* json, const char* key) {
    std::string source;
    NxJsonValue root;
    if (!nx_parse_json_source(json, source, root))
        return nx_strdup("invalid");
    if (root.kind != NxJsonValue::Kind::Object)
        return nx_strdup("missing");
    const NxJsonValue* value = nx_json_find_member(root, key);
    return nx_strdup(value ? nx_json_kind_name(value->kind) : "missing");
}
int64_t nexus_json_array_len(const char* json, const char* key) {
    std::string source;
    NxJsonValue root;
    if (!nx_parse_json_source(json, source, root) ||
        root.kind != NxJsonValue::Kind::Object)
        return -1;
    const NxJsonValue* value = nx_json_find_member(root, key);
    if (!value || value->kind != NxJsonValue::Kind::Array)
        return -1;
    return static_cast<int64_t>(value->children.size());
}
char* nexus_json_array_get(const char* json, const char* key, int64_t index) {
    if (index < 0)
        return nx_strdup("");
    std::string source;
    NxJsonValue root;
    if (!nx_parse_json_source(json, source, root) ||
        root.kind != NxJsonValue::Kind::Object)
        return nx_strdup("");
    const NxJsonValue* value = nx_json_find_member(root, key);
    if (!value || value->kind != NxJsonValue::Kind::Array ||
        static_cast<uint64_t>(index) >= value->children.size())
        return nx_strdup("");
    const std::string result =
        nx_json_value_text(source, value->children[static_cast<size_t>(index)]);
    return nx_strdup(result.c_str());
}
