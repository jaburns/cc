#pragma once
#include "inc.hh"
namespace {

struct XmlParseAttribute {
    Str key, value;
};

void xml_parse(
    Str   xml,
    void* user_ctx,
    void  (*on_element_open)(void* ctx, Str path, Slice<XmlParseAttribute> attributes),
    void  (*on_element_close)(void* ctx, Str path, Str content)
);

}  // namespace