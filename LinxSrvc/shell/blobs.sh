#!/bin/bash
# Batch process .prototxt files: remove "dim: 1" lines, re-insert at line 7, strip blanks, merge split outputs
idx=0
for file in ./*.prototxt; do
    [ -f "$file" ] || continue
    echo "$file"

    # Split large files (>99 lines) at first "blobs {" block
    if [ "$(wc -l < "$file")" -gt 99 ]; then
        sed -n "1,/blobs {/p" "$file" > "h_${idx}"
        sed "1,/blobs {/d" "$file" > "t_${idx}"
        file="h_${idx}"
    fi

    # Strip existing "dim: 1", then insert aligned at line 7
    sed -i '{:loop /dim: 1$/!{N;b loop};s///g}' "$file"
    sed -i '7a#      dim: 1' "$file"
    sed -i '8s/^#//' "$file"
    sed -i '/^ *$/d' "$file"

    # Reassemble split file and clean up temp files
    if [ -f "t_${idx}" ]; then
        cat "$file" "t_${idx}" > "${file}.tmp" && mv "${file}.tmp" "$file"
        rm -f "h_${idx}" "t_${idx}"
    fi

    ((idx++))
done
