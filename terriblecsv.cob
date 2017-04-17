       identification division.
       program-id. terriblecsv.
       environment division.
       configuration section.
       repository.
           function all intrinsic.

       data division.
       working-storage section.

       01 package-json-header.
           20 name          pic x(32) value 'name'.
           20 filler        pic x value ','.
           20 version       pic x(8) value 'version'.
           20 filler        pic x value ','.
           20 description   value 'description'.

       01 package-json-record.
           20 name          pic x(32).
           20 filler        pic x value ','.
           20 version       pic x(8).
           20 filler        pic x value ','.
           20 description   value 'return a simple record'.

       procedure division.
       move module-id to name in package-json-record.
       display package-json-header.
       display package-json-record.

