# PalotasB Static Vector

## Introduction

This code implements a vector with static storage, which in this case means that every contained element is stored inside the `static_vector` object.
This increases the size of the object and limits the capacity but it eliminates dynamic memory allocation.

The interface follows `std::vector` as closely as possible, with exceptions documented in the source code.
The entire source is a single class template, templated by the stored data type and the capacity of the container.
The container is supposed to have the least surprises possible, and implements the interfaces the same way as `std::vector` does.
Support is also implemented for non-trivially copyable and move-only types, and alignment requirements are followed.
This means that any type that can be stored in a vector can be stored in the `static_vector` and the program will work correctly.

## Rationale

For implementation, I relied on the C++14 standard template library for several reasons.
First, implementing everything from scratch is usually redundant and not smart.
Second, C++14 is widely supported, and the STL is a simple dependency, usually already included.
For testing, I wrote a very simple program that helped me verify my code for correctness, but with a larger project, a proper test system would be appropriate.

## Implementation details

Storage is provided by an inline `std::array` of the proper size.
The data array is templated with `std::aligned_storage_t` to satisfy alignment requirements.
New elements are constructed with placement `new` to correctly call constructor code and they are destroyed when appropriate.
Inserting and removing elements causes the others to be shifted using `move` assignment therefore move-only types can be used in the container as well as any other.
The only other element apart from the array storage is a type `std::size_t` size which is equal to the dynamic size of the container.
Const correctness is a goal for the code, and I have also somewhat tried to apply the correct `noexcept` specification but I have not applied it to most methods where this depends on the contained type.

## Testing

I test that values are inserted, removed and iterated the expected way by constructing a `static_vector` of `int`s and manually verifying the values.
I test the correctness of calling constructor, destructors, copies and moves with two special types that have special invariants that would fail if an object is not properly constructed, copied, moved or destructed during the live of a program.
(This caught one bug where I accidentally used `operator=` instead of placement `new` copy constructor to create a new value.)

## Further development

I made the "business decision" not to fully develop all features (i.e., implement all methods) listed above.
I was low on time and the already implemented code already serves the purpose here, to show what kind of code I write and how I develop software.
I've listed some TODOs in the code that signal that I'm aware of a part of the code that might need fine-tuning to pass code review in production.
Testing also does not cover 100%, but I'm fairly certain that my code does not contain bugs at this point thanks to the existing test cases.
I'd be happy to show how I would implement any of the missing features I listed as TODOs in the code or the ones you can come up with.

Testing with address sanitizer or valgrind and undefined behavior sanitizer might be a possibility but the code necessarily uses uninitialized storage and dynamic object creation in a way that might be undefined behavior according to a strict reading of the standard.
This is the same for `std::vector` so I would not be alarmed by all warnings produced by those tools but I haven't used them here.

## Try out the code

Just compile and run the tests.cpp and make sure to include C++14 features.
Or use the CMake project template.
