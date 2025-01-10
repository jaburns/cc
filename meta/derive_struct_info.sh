#!/usr/bin/env bash
set -e
#-------------------------------------------------------------------------------

if [[ -z "$1" || -z "$2" ]]; then
    echo ''
    echo 'Tool to generate struct member metadata for structs marked with //#!derive_struct_info.'
    echo ''
    echo 'Usage: meta/derive_struct_info.sh "{search root dir}" "{filename pattern}"'
    echo ''
    exit 1
fi

#-------------------------------------------------------------------------------

generate() {
    rm -f /tmp/gen.h
    while read -r line; do
        if [[ "$line" == //* || "$line" == }* ]]; then
            continue
        elif [[ "$line" == struct* ]]; then
            local struct_name="$(echo "$line" | tr -s ' ' '\t' | cut -f2)"
            >>/tmp/gen.h echo "StructMemberInfo x_${struct_name}_MEMBERS_ARRAY[] = {"
        elif [[ "$line" == '#include'* ]]; then
            local out_path="$(echo "$line" | tr -s ' ' '\t' | cut -f2 | tr -d '"')"
            >>/tmp/gen.h echo '};'
            >>/tmp/gen.h echo "Slice<StructMemberInfo> ${struct_name}_MEMBERS = Slice<StructMemberInfo>{x_${struct_name}_MEMBERS_ARRAY, RawArrayLen(x_${struct_name}_MEMBERS_ARRAY)};"
            mv /tmp/gen.h "$(dirname "$1")/$out_path"
        else
            local type="$(echo "$line" | tr -s ' ' '\t' | cut -f1)"
            local name="$(echo "$line" | tr -s ' ' '\t' | cut -f2 | tr -d ';')"
            >>/tmp/gen.h echo '{'
            >>/tmp/gen.h echo "    .type   = StrLit(\"$type\"),"
            >>/tmp/gen.h echo "    .name   = StrLit(\"$name\"),"
            >>/tmp/gen.h echo "    .size   = sizeof($type),"
            >>/tmp/gen.h echo "    .offset = offsetof($struct_name, $name),"
            >>/tmp/gen.h echo '},'
        fi
   done <<< "$2"
}

for file in $(find "$1" -name "$2" -print0 | xargs -0 grep -l "#!derive_struct_info"); do
    generate "$file" "$(sed -n '/#!derive_struct_info/,/#include/p' $file)"
done

#-------------------------------------------------------------------------------