module Program where

import Ast



program = objCons(one(lexReq(one(part("print", one(strLit(safeStr("Hello, world from Haskell", charExclam, ""))))))), nil)
