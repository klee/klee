#!/bin/bash

clang++ -o test/unittestsmark test/unittestsmark/No_Errors_Build_Test_Code.cpp
clang++ -o test/unittestsmark test/unittestsmark/racecond.cpp
clang++ -o test/unittestsmark test/unittestsmark/undefined_behaviour.cpp
clang++ -o test/unittestsmark test/unittestsmark/deadlock.cpp

klee test/unittestsmark/No_Errors_Build_Test_Code.cpp
klee test/unittestsmark/racecond.cpp
klee test/unittestsmark/undefined_behaviour.cpp
klee test/unittestsmark/deadlock.cpp