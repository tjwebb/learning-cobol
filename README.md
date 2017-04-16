# learning-cobol

A collection of code I'm writing in order to learn cobol. Do not use this code for anything important.

## Install cobol Compiler

#### 1. Download gnu-cobol

[https://sourceforge.net/projects/open-cobol/]

#### 2. Compile and Install

```sh
cd gnu-cobol-2.0
./configure
make
sudo make install
```

## Build Programs!

```sh
# compile to executable
cobc -x executable.cob

# compile to lib
cobc lib.cob
```

## Example

```cob
       identification division.
       program-id. module1.
       environment division.
       configuration section.
       repository.
           function all intrinsic.
       procedure division.
       main.
           display "module-id = " module-id.
```

```sh
$ cobc -x module1.cob
$ ./module1
module-id = module1
```

## References

- [gnu-cobol compiler docs](https://open-cobol.sourceforge.io/guides/GNU%20COBOL%202.0%20Programmers%20Guide.pdf)
- [lynda.com](https://www.lynda.com/COBOL-tutorials/)
