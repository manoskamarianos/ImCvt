FROM aflplusplus/aflplusplus:latest

COPY . /src

WORKDIR /AFLplusplus

RUN ./afl-clang-fast /src/src/*.c -O3 -o ImCvt

ENTRYPOINT  ["./afl-fuzz", "-i", "./testcases/images/bmp", "-o", "/src/fuzz_output", "--", "./ImCvt", "@@", "-o", "/tmp/out", "-f"]
