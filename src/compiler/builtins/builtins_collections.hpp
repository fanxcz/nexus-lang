#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace nexus {

    inline std::optional<BuiltinSignature> builtinCollectionSignature(const std::string& name) {

        using T = Type;

        if (name == "ilist_new")
            return BuiltinSignature{name, {}, T::i64(), "nexus_ilist_new"};

        if (name == "ilist_push")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::void_(), "nexus_ilist_push"};

        if (name == "ilist_len")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_ilist_len"};

        if (name == "ilist_get")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::i64(), "nexus_ilist_get"};

        if (name == "ilist_set")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::i64()}, T::void_(), "nexus_ilist_set"};

        if (name == "ilist_push_checked")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::boolean(), "nexus_ilist_push_checked"};

        if (name == "ilist_reserve")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::boolean(), "nexus_ilist_reserve"};

        if (name == "ilist_clear")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_ilist_clear"};

        if (name == "ilist_free")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_ilist_free"};

        if (name == "ilist_insert")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::i64()}, T::boolean(), "nexus_ilist_insert"};

        if (name == "ilist_try_get")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::i64())}, T::boolean(), "nexus_ilist_try_get"};

        if (name == "ilist_set_checked")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::i64()}, T::boolean(), "nexus_ilist_set_checked"};

        if (name == "ilist_remove")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::i64())}, T::boolean(), "nexus_ilist_remove"};

        if (name == "ilist_try_pop")
            return BuiltinSignature{name, {T::i64(), T::ptr(T::i64())}, T::boolean(), "nexus_ilist_try_pop"};

        if (name == "slist_new")
            return BuiltinSignature{name, {}, T::i64(), "nexus_slist_new"};

        if (name == "slist_push")
            return BuiltinSignature{name, {T::i64(), T::string()}, T::void_(), "nexus_slist_push"};

        if (name == "slist_len")
            return BuiltinSignature{name, {T::i64()}, T::i64(), "nexus_slist_len"};

        if (name == "slist_get")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::string(), "nexus_slist_get"};

        if (name == "slist_push_checked")
            return BuiltinSignature{name, {T::i64(), T::string()}, T::boolean(), "nexus_slist_push_checked"};

        if (name == "slist_reserve")
            return BuiltinSignature{name, {T::i64(), T::i64()}, T::boolean(), "nexus_slist_reserve"};

        if (name == "slist_clear")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_slist_clear"};

        if (name == "slist_free")
            return BuiltinSignature{name, {T::i64()}, T::boolean(), "nexus_slist_free"};

        if (name == "slist_insert")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::string()}, T::boolean(), "nexus_slist_insert"};

        if (name == "slist_try_get")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::string())}, T::boolean(), "nexus_slist_try_get"};

        if (name == "slist_set_checked")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::string()}, T::boolean(), "nexus_slist_set_checked"};

        if (name == "slist_remove")
            return BuiltinSignature{name, {T::i64(), T::i64(), T::ptr(T::string())}, T::boolean(), "nexus_slist_remove"};

        if (name == "slist_try_pop")
            return BuiltinSignature{name, {T::i64(), T::ptr(T::string())}, T::boolean(), "nexus_slist_try_pop"};

        return std::nullopt;
    }

} // namespace nexus
