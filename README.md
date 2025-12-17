# Commands

## Project Structure

```
.
├── CMakeLists.txt
├── commands.txt
├── inputs
│   ├── iv-test.ll
│   ├── iv-test.opt.ll
│   ├── licm-test.ll
│   ├── licm-test.opt.ll
│   ├── matmul-canonical.ll
│   ├── ptr-test.ll
│   └── ptr-test.opt.ll
├── outputs
│   ├── iv-test-out.ll
│   ├── licm-test-out.opt.ll
│   ├── matmul-derived.ll
│   └── matmul-licm.ll
├── README.md
├── src
│   ├── AffineRecurrence.cpp
│   ├── CMakeLists.txt
│   ├── DerivedInductionVar.cpp
│   └── SimpleLICM.cpp
└── tests
    ├── iv-test.c
    ├── licm-test.c
    └── ptr-test.c
```

## CMake builds
Adjust `-DLLVM_DIR` as needed
`cmake -S . -B build -DLLVM_DIR=/usr/local/lib/cmake/llvm -G Ninja`

## Run SimpleLICM
### use the .dylib or .so file
`opt -load-pass-plugin=build/src/SimpleLICM.so -passes=simple-licm -S -o outputs/*.ll inputs/matmul-canonical.ll`

## Run DerivedIV
### use the .dylib or .so file
`opt -load-pass-plugin=build/src/DerivedIV.so -passes=derived-iv -S -o outputs/*.ll inputs/matmul-canonical.ll`
