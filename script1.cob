#!/usr/local/bin/cobc -xj

       identification division.
       program-id. script1.
       environment division.
       configuration section.
       repository.
           function all intrinsic.
       procedure division.

       display '{'
           'name: "' module-id '", '
           'path: "' module-path '", '
           'sourceFile: "' module-source '" '
       '}'
       .


