#include "main_dep.hh"

struct HasArena {
    Arena arena;
};

void test_on_element_open(void* ctx, Str path, Slice<XmlParseAttribute> attributes) {
    println("open ", path);
    for (u32 i = 0; i < attributes.count; ++i) {
        println("ATTR! ", attributes[i].key, " ", attributes[i].value);
    }
}
void test_on_element_close(void* ctx, Str path, Str content) {
    println("close ", path, ":::", content);
}

#if TEST
i32 main() {
    test_base();
    return 0;
}
#else

i32 main() {
    arena_scratch_thread_local_create(memory_get_global_allocator(), 0);

    auto arr   = (HasArena*)calloc(1, sizeof(HasArena));
    arr->arena = Arena::create(memory_get_global_allocator(), 0);

    auto h = HashArray<u32, u32>::alloc_with_elems(&arr->arena, 4);

    auto vec    = Vec<u32>::alloc(&arr->arena, 8);
    *vec.push() = 99;
    *vec.push() = 11;

    Vec<char> horse = Vec<char>::alloc(&arr->arena, 256);
    str_println(&horse, 3.3, "one", 2);
    println("V~~ec!", horse);

    InlineVec<u32, 16> hellos = {};
    *hellos.push()            = 123;

    println(hellos);

    println(U64PrintedWithCommas{4llu});

    println("Vec!", vec, 2, 3);

    Str doge = Str::from_cstr("Hello doge!");
    println(doge);

    log(vec2(10, 10).add_scaled(9, vec2(1, 1)));

    auto val = *vec.pop();
    printf("--- %u\n", val);

    vec2 hi  = vec2(1, 2);
    vec2 ho  = vec2(11, 9);
    hi      += vec2(99, 99);

    hi *= .1f;

    log(hi + ho);

    log(.5f * vec2(10, 10));

    Str xml = Str::from_cstr("<hello one=\"1\" two=\"2\"><hi x=\"hi\">Inner text</hi></hello>");

    xml_parse(xml, nullptr, test_on_element_open, test_on_element_close);

    {
        ScratchArenaHandle scratch(SliceLit(Arena*, &arr->arena));

        u64* hi = scratch.arena->alloc_one<u64>();
        *hi     = 50;

        u32 a1 = 1, a2 = 2, a3 = 3, a4 = ++*hi;

        *h->insert(&a1) = 91;
        *h->insert(&a2) = 92;
        *h->insert(&a4) = 94;

        for (auto entry : *h) {
            printf("Horse %u %u \n", *entry.key, *entry.value);
        }

        u32* g;
        // clang-format off
        g = h->get(&a1); if (g == nullptr) printf("no\n"); else printf("%u\n", *g);
        g = h->get(&a2); if (g == nullptr) printf("no\n"); else printf("%u\n", *g);
        g = h->get(&a3); if (g == nullptr) printf("no\n"); else printf("%u\n", *g);
        g = h->get(&a4); if (g == nullptr) printf("no\n"); else printf("%u\n", *g);
        // clang-format on
    }

    arr->arena.destroy();

    println("Hello");

    return 0;
}
#endif