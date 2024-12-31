#include "inc.hh"
namespace {

void xml_parse(
    Str   xml,
    void* user_ctx,
    void  (*on_element_open)(void* ctx, Str path, Slice<XmlParseAttribute> attributes),
    void  (*on_element_close)(void* ctx, Str path, Str content)
) {
    constexpr usize MAX_PATH_LEN = 256;
    constexpr usize MAX_ATTRS    = 64;

    cchar* read     = xml.elems;
    cchar* file_end = xml.elems + xml.count;

    char      path_buffer[MAX_PATH_LEN];
    Vec<char> path = Vec<char>::from_ptr(path_buffer, MAX_PATH_LEN);

    XmlParseAttribute      attributes_buffer[MAX_ATTRS];
    Vec<XmlParseAttribute> attributes = Vec<XmlParseAttribute>::from_ptr(attributes_buffer, MAX_ATTRS);

    cchar* content_start = nullptr;

    while (read < file_end) {
        char c = *read++;

        if (c != '<') {
            if (!content_start && !isspace(c)) {
                content_start = read - 1;
            }
            continue;
        }

        Str whole_tag = Str::from_ptr(read, file_end - read).before_first_index('>');
        Str tag       = whole_tag.trim();
        Assert(tag.count > 0);
        bool pop_path = false;

        if (tag.elems[0] == '/') {
            ++tag.elems;
            --tag.count;
            pop_path = true;

            Str content = content_start
                              ? Str::from_ptr(content_start, read - 1 - content_start)
                              : Str{};

            Str str_path = Str::from_ptr(path.elems, path.count);
            on_element_close(user_ctx, str_path, content);

        } else if (tag.elems[0] != '?') {
            pop_path = tag.elems[tag.count - 1] == '/';
            if (pop_path) tag.count--;

            if (path.count) {
                *path.push() = '/';
            }

            attributes.count = 0;

            cchar* cur = tag.elems;
            cchar* end = tag.elems + tag.count;
            while (cur < end && !isspace(*cur)) ++cur;
            Str tag_name = Str::from_ptr(tag.elems, cur - tag.elems);

            str_print(&path, tag_name);

            while (cur < end) {
                while (cur < end && isspace(*cur)) ++cur;
                if (cur >= end) break;

                cchar* key_start = cur;
                while (cur < end && !isspace(*cur) && *cur != '=') ++cur;
                Str key = Str::from_ptr(key_start, cur - key_start);

                while (cur < end && isspace(*cur)) ++cur;
                Assert(cur < end && *cur == '=');
                ++cur;

                while (cur < end && isspace(*cur)) ++cur;
                Assert(cur < end);

                char quote = *cur;
                Assert(quote == '"' || quote == '\'');
                ++cur;

                cchar* val_start = cur;
                while (cur < end && *cur != quote) ++cur;
                Assert(cur < end);

                *attributes.push() = XmlParseAttribute{
                    .key   = key,
                    .value = Str::from_ptr(val_start, cur - val_start),
                };

                ++cur;
            }

            Str str_path = Str::from_ptr(path.elems, path.count);
            on_element_open(user_ctx, str_path, attributes.slice());
            if (pop_path) on_element_close(user_ctx, str_path, Str{});
        }

        read          = whole_tag.elems + whole_tag.count + 1;
        content_start = NULL;

        if (pop_path) {
            while (path.count && path.elems[path.count - 1] != '/') path.count--;
            if (path.count) path.count--;
        }
    }
}

}  // namespace