#include "derive.hh"
namespace a {
// -----------------------------------------------------------------------------
namespace derive_ {

bool read_field(Arena* arena, Str line, Str* out_type, Str* out_name) {
    if (line.count == 0) return false;
    char* type = arena->push_many<char>(128).elems;
    char* name = arena->push_many<char>(128).elems;
    int pos = 0;
    if (sscanf(line.elems, " %127[^ \t\n;] %127[^() \t\n;] ;%n", type, name, &pos) == 2) {
        if (pos == line.count) {
            *out_type = Str::from_cstr(type);
            *out_name = Str::from_cstr(name);
            return true;
        }
    }
    return false;
}

bool check_is_post_deserialize_method(Str line) {
    if (line.count == 0) return false;
    int pos = -1;
    sscanf(line.elems, "void post_deserialize (%n", &pos);
    return pos > -1;
}

bool read_include_path(Arena* arena, Str line, Str* out_path) {
    char* path = arena->push_many<char>(256).elems;
    if (sscanf(line.elems, " #include \"%255[^\"]\"", path) == 1) {
        *out_path = Str::from_cstr(path);
        return true;
    }
    return false;
}

bool has_tag(List<Str> tags, Str tag) {
    foreach (it, tags.iter()) {
        if (it.item->eq(tag)) return true;
    }
    return false;
}

// -----------------------------------------------------------------------------

void handler_derive_struct_info(DeriveStructInfo* info) {
    ScratchArena scratch{};
    auto sb = StrBuilder::make(scratch.arena);

    sb.println("StructMemberInfo x_", info->name, "_MEMBERS_ARRAY[] = {");
    foreach (it, info->fields.iter()) {
        sb.println("{");
        sb.println("    .type = \"", it.item->type, "\",");
        sb.println("    .name = \"", it.item->name, "\",");
        sb.println("    .size = sizeof(", it.item->type, "),");
        sb.println("    .offset = offsetof(", info->name, ", ", it.item->name, "),");
        sb.println("},");
    }
    sb.println("};");

    sb.println(
        "Slice<StructMemberInfo> ",
        info->name, "_MEMBERS = Slice<StructMemberInfo>{x_",
        info->name, "_MEMBERS_ARRAY, RawArrayLen(x_",
        info->name, "_MEMBERS_ARRAY)};"
    );

    sb.println("");
    fs_append_file_bytes(info->target_hh_path, sb.to_str().to_slice().cast<u8>());
}

// -----------------------------------------------------------------------------

void handler_derive_json(DeriveStructInfo* info) {
    ScratchArena scratch{};
    auto sb = StrBuilder::make(scratch.arena);

    sb.println("void json_serialize(Arena* out, ", info->name, "* val, u32 tab);");
    sb.println("bool json_deserialize_field(Arena* arena, void* ctx, cchar* end, cchar** read, ", info->name, "* val, bool* cont);");
    sb.println("bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, ", info->name, "* val);");

    sb.println("");
    fs_append_file_bytes(info->target_hh_path, sb.to_str().to_slice().cast<u8>());
    scratch.arena->cur = sb.arena_start;

    Str arena_name = {};
    foreach (it, info->fields.iter()) {
        if (it.item->type.eq("Arena")) {
            arena_name = it.item->name;
            break;
        }
    }

    sb.println("void json_serialize(Arena* out, ", info->name, "* val, u32 tab) {");
    sb.println("    tab++;");
    sb.println("    Assert(2 * tab < sizeof(JSON_SERIALIZE_INDENTATION));");
    sb.println("    Str tabstr = Str{JSON_SERIALIZE_INDENTATION, 2 * tab};");
    sb.println("    arena_print(out, \"{\\n\");");
    sb.println("    ");
    foreach_idx(i, it, info->fields.iter()) {
        if (it.item->type.eq("Arena") || has_tag(it.item->tags, "serde_skip")) continue;

        sb.println("    arena_print(out, tabstr, \"\\\"", it.item->name, "\\\": \");");
        sb.println("    json_serialize(out, &val->", it.item->name, ", tab);");
        if (i < info->fields.count) {
            sb.println("    arena_print(out, \",\\n\");");
        } else {
            sb.println("    arena_print(out, \"\\n\");");
        }
        sb.println("    ");
    }
    sb.println("    tab--;");
    sb.println("    tabstr = Str{JSON_SERIALIZE_INDENTATION, 2 * tab};");
    sb.println("    arena_print(out, tabstr, \"}\");");
    sb.println("}");
    sb.println("");

    sb.println("bool json_deserialize_field(Arena* arena, void* ctx, cchar* end, cchar** read, ", info->name, "* val, bool* cont) {");
    sb.println("    if (!json_expect(end, read, \"\\\"\")) return false;");
    sb.println("    if (false) {");
    foreach (it, info->fields.iter()) {
        if (it.item->type.eq("Arena") || has_tag(it.item->tags, "serde_skip")) continue;

        sb.println("    } else if (json_expect(end, read, \"", it.item->name, "\\\"\")) {");
        sb.println("        if (!json_expect(end, read, \":\")) return false;");
        sb.println("        if (!json_deserialize(arena, ctx, end, read, &val->", it.item->name, ")) return false;");
    }
    sb.println("    } else {");
    sb.println("        if (!json_skip_opened_string(end, read)) return false;");
    sb.println("        if (!json_expect(end, read, \":\")) return false;");
    sb.println("        if (!json_skip_value(end, read)) return false;");
    sb.println("    }");
    sb.println("    json_chomp_whitespace(end, read);");
    sb.println("    if (json_expect(end, read, \",\")) {");
    sb.println("        if (json_expect(end, read, \"}\")) {");
    sb.println("            *cont = false;");
    sb.println("        } else {");
    sb.println("            *cont = true;");
    sb.println("        }");
    sb.println("    } else if (json_expect(end, read, \"}\")) {");
    sb.println("        *cont = false;");
    sb.println("    } else {");
    sb.println("        return false;");
    sb.println("    }");
    sb.println("    return true;");
    sb.println("}");

    sb.println("bool json_deserialize(Arena* arena, void* ctx, cchar* end, cchar** read, ", info->name, "* val) {");
    sb.println("    StructZero(val);");
    if (arena_name.count) {
        sb.println("    val->", arena_name, ".create();");
        sb.println("    arena = &val->", arena_name, ";");
    }
    sb.println("    if (!json_expect(end, read, \"{\")) return false;");
    sb.println("    bool cont = true;");
    sb.println("    while (cont) {");
    sb.println("        if (!json_deserialize_field(arena, ctx, end, read, val, &cont)) return false;");
    sb.println("    }");
    if (info->has_post_deserialize_method) {
        sb.println("    val->post_deserialize(ctx);");
    }
    sb.println("    return true;");
    sb.println("}");

    sb.println("");
    fs_append_file_bytes(info->target_cc_path, sb.to_str().to_slice().cast<u8>());
}

// -----------------------------------------------------------------------------

void handler_derive_bindump(DeriveStructInfo* info) {
    ScratchArena scratch{};
    auto sb = StrBuilder::make(scratch.arena);

    sb.println("void bin_serialize(Arena* out, ", info->name, "* val);");
    sb.println("bool bin_deserialize(Arena* arena, void* ctx, u8* end, u8** read, ", info->name, "* val);");

    sb.println("");
    fs_append_file_bytes(info->target_hh_path, sb.to_str().to_slice().cast<u8>());
    scratch.arena->cur = sb.arena_start;

    Str arena_name = {};
    foreach (it, info->fields.iter()) {
        if (it.item->type.eq("Arena")) {
            arena_name = it.item->name;
            break;
        }
    }

    sb.println("void bin_serialize(Arena* out, ", info->name, "* val) {");
    foreach (it, info->fields.iter()) {
        if (it.item->type.eq("Arena") || has_tag(it.item->tags, "serde_skip")) continue;
        sb.println("    bin_serialize(out, &val->", it.item->name, ");");
    }
    sb.println("}");
    sb.println("");

    sb.println("bool bin_deserialize(Arena* arena, void* ctx, u8* end, u8** read, ", info->name, "* val) {");
    sb.println("    StructZero(val);");
    if (arena_name.count) {
        sb.println("    val->", arena_name, ".create();");
        sb.println("    arena = &val->", arena_name, ";");
    }
    foreach (it, info->fields.iter()) {
        if (it.item->type.eq("Arena") || has_tag(it.item->tags, "serde_skip")) continue;
        sb.println("    if (!bin_deserialize(arena, ctx, end, read, &val->", it.item->name, ")) return false;");
    }
    if (info->has_post_deserialize_method) {
        sb.println("    val->post_deserialize(ctx);");
    }
    sb.println("    return true;");
    sb.println("}");

    sb.println("");
    fs_append_file_bytes(info->target_cc_path, sb.to_str().to_slice().cast<u8>());
}

// -----------------------------------------------------------------------------

void parse_file(Str dir_path, Str file_path, Slice<DeriveCustomHandler> custom_handlers) {
    ScratchArena scratch{};

    Str file = Str::from_slice_char(fs_read_file_bytes(scratch.arena, file_path).cast<char>());
    u32 line_no = 0;

    Vec<DeriveHandler> handlers = Vec<DeriveHandler>::make(scratch.arena, 64);
    u8* scratch_mark = scratch.arena->cur;
    DeriveStructInfo info = {};

    foreach (it, file.split_char_iter('\n')) {
        Str line = it.item.trim();
        line_no++;

        if (handlers.count > 0) {
            if (line_no == 1) {
                auto it = line.split_whitespace_iter();
                it.next();
                info.name = it.item;

            } else if (line.starts_with("#include")) {
                Str path;
                Assert(read_include_path(scratch.arena, line, &path));

                info.target_hh_path = arena_print(scratch.arena, dir_path, "/", path);
                info.target_cc_path = info.target_hh_path.replace(scratch.arena, ".hh", ".cc");

                fs_remove_file_if_exists(info.target_hh_path);
                fs_remove_file_if_exists(info.target_cc_path);

                foreach (handler, handlers.iter()) {
                    (*handler.item)(&info);
                }
                handlers.count = 0;
                StructZero(&info);

            } else if (!line.starts_with("}") && !line.starts_with("/")) {
                Str type, name;
                Str pre_comment = line.before_first_index('/').trim();

                if (read_field(scratch.arena, pre_comment, &type, &name)) {
                    bool skip = false;

                    DeriveStructField* f = info.fields.push(scratch.arena);
                    f->type = type;
                    f->name = name;

                    Str post_comment = line.after_first_index('/').trim();
                    if (post_comment.count > 0) {
                        auto tags_it = post_comment.split_whitespace_iter();
                        if (tags_it.item.eq("/!")) {
                            tags_it.next();
                            foreach (tag, tags_it) {
                                *f->tags.push(scratch.arena) = tag.item;
                            }
                        }
                    }
                } else {
                    bool pds = check_is_post_deserialize_method(line);
                    if (pds) {
                        info.has_post_deserialize_method = true;
                    }
                }
            }
        } else {
            if (line.starts_with("//! derive")) {
                scratch.arena->cur = scratch_mark;
                line_no = 0;

                auto lines_it = line.split_whitespace_iter();
                lines_it.next();  // //!
                lines_it.next();  // derive
                foreach (it, lines_it) {
                    if (it.item.eq("json")) {
                        *handlers.push() = handler_derive_json;
                    } else if (it.item.eq("bindump")) {
                        *handlers.push() = handler_derive_bindump;
                    } else if (it.item.eq("structinfo")) {
                        *handlers.push() = handler_derive_struct_info;
                    } else {
                        foreach (han, custom_handlers.iter()) {
                            if (it.item.eq(han.item->match_name)) {
                                *handlers.push() = han.item->handler;
                            }
                        }
                    }
                }
            }
        }
    }
}

void walk_dir(Vec<Str>* path_parts, Slice<DeriveCustomHandler> custom_handlers) {
    ScratchArena scratch{};
    Str path = Str::join(scratch.arena, path_parts->slice(), '/');

    foreach (it, FsFolderIter::make(path)) {
        *path_parts->push() = it.item;

        if (it.item_is_dir) {
            if (!it.item.eq("meta")) {
                walk_dir(path_parts, custom_handlers);
            }
        } else {
            Str file_path = Str::join(scratch.arena, path_parts->slice(), '/');
            parse_file(path, file_path, custom_handlers);
        }

        path_parts->pop();
    }
}

}  // namespace derive_
// -----------------------------------------------------------------------------

void derive_run(Str root_dir, Slice<DeriveCustomHandler> custom_handlers) {
    ScratchArena scratch{};
    Vec<Str> path_parts = Vec<Str>::make(scratch.arena, 64);
    *path_parts.push() = "src";
    derive_::walk_dir(&path_parts, custom_handlers);
}

// -----------------------------------------------------------------------------
};  // namespace a
