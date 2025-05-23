#!/bin/bash

files=(../haskell/{Ast,CPS,Runtime}.hs)

{
    grep -hE '^import ' "${files[@]}" | grep -vE '^import (Ast|CPS|Runtime)' | sort -u
    cat <<'EOT'

main = toFunc program $ dropContext
EOT
    grep -hvE '^import |^module ' "${files[@]}" 
} > flat-template.hs
