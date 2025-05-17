#!/bin/bash

files=(../haskell/{Ast,CPS,Runtime}.hs)

{
    grep -hE '^import ' "${files[@]}" | grep -vE '^import (Ast|CPS|Runtime)' | sort -u
    cat <<'EOT'

main = do
    let func = toFunc program
    func dropContext
EOT
    for f in "${files[@]}"
    do
        grep -vE '^import |^module ' "$f" 
    done
} > flat-template.hs
