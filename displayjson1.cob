       identification division.
       program-id. displayjson1.
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

