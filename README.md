# Commands
## CMake builds
`cmake -S . -B build -DLLVM_DIR=/usr/local/lib/cmake/llvm -G Ninja`

## Run SimpleLICM
### use the .dylib or .so file
`opt -load-pass-plugin=build/src/SimpleLICM.so -passes=simple-licm -S -o outputs/*.ll inputs/matmul-canonical.ll`

## Run DerivedIV
### use the .dylib or .so file
`opt -load-pass-plugin=build/src/DerivedIV.so -passes=derived-iv -S -o outputs/*.ll inputs/matmul-canonical.ll`
