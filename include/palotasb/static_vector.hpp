#ifndef PALOTASB_STATIC_VECTOR_H
#define PALOTASB_STATIC_VECTOR_H

#pragma once

/** Copyrighted according to the LICENSE file.
 * SPDX-License-Identifier: MIT
 * */

#include <algorithm> // std::for_each, std::move*
#include <array>     // std::array
#include <exception> // std::out_of_range
#include <iterator>  // std::reverse_iterator, std::distance
#include <memory>    // std::uninitialized_*,
#include <utility>   // std::aligned_storage

/** Static vector, a dynamic sized array storage that uses no automatic heap
 * memory allocation.
 *
 * The interface follows that of std::vector as permitted by design constraints.
 * Reference used is the C++17 draft standard [N4659] and [cppref]
 * http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4659.pdf
 * https://en.cppreference.com/w/cpp/container/vector
 * */

namespace stlpb {

// "PalotasB" Static Vector.
// This class template behaves exactly like std::vector except that it
// implements a fixed-size inline storage with the capacity defined by the
// Capacity template parameter.
// Main differences between static_vector and std::vector
//  - static_vector does not have an Allocater template parameter as all
// allocations are inline. (Its behaviour could be mostly implemented with a
// specialized allocater type that only returns inline references.)
//  - static_vector never reallocates and therefore iterators never become
// invalid when the corresponding std::vector method might invalidate because of
// reallocation.
//  - `reserve(size_type)` and `shrink_to_fit()` are intentionally not
// implemented because their existence would be misleading to the user.
template <typename T, std::size_t Capacity> //
struct static_vector {

    // MEMBER TYPES

    // Value type equal to T
    using value_type = T;
    // std::size_t without including a header just for this name
    using size_type = std::size_t;
    // std::ptrdiff_t without including a header just for this name
    using difference_type = std::ptrdiff_t;
    // Reference type is a regular reference, not a proxy
    using reference = value_type&;
    using const_reference = const value_type&;
    // Pointer is a regular pointer
    using pointer = value_type*;
    using const_pointer = const value_type*;
    // Iterator is a regular pointer
    using iterator = pointer;
    using const_iterator = const_pointer;
    // Reverse iterator is what the STL provides for reverse iterating pointers
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    // The static capacity of the static_vector
    static const size_type static_capacity = Capacity;

    // CONSTRUCTORS

    // Default constructor
    // Requires: nothing
    // Ensures: The static_vector contains zero elements.
    // Complexity: constant
    // Exceptions: noexcept
    static_vector() noexcept : m_size(0) {}

    // "N copies of one value" constructor
    // Requires:
    //  `count` is less than or equal to `capacity`
    // Ensures:
    //  The static_vector contains `count` elements copy-constructed from
    //  `value`.
    // Complexity: O(count)
    // Exceptions: if the copy constructor throws or if this trys to create too
    // many objects then throws std::out_of_range
    static_vector(size_type count, const_reference value) //
        : m_size(count <= static_capacity ? count : throw std::out_of_range("size()")) {
        std::uninitialized_fill(begin(), end(), value);
    }

    // "N default constructed items" constructor
    static_vector(size_type count)
        : m_size(count <= static_capacity ? count : throw std::out_of_range("size()")) {
        std::for_each( // C++17 would use std::uninitialized_default_construct
            storage_begin(), storage_end(), [](storage_type& store) {
                new (static_cast<void*>(&store)) value_type;
            });
    }

    // Initializer list constructor
    static_vector(std::initializer_list<value_type> init_list)
        : m_size(init_list.end() - init_list.begin()) {
        if (m_size > static_capacity)
            throw std::out_of_range("size()");

        std::uninitialized_copy(init_list.begin(), init_list.end(), begin());
    }

    // TODO maybe implement trivial copy/move/destruct if `value_type` supports
    // it

    // Copy constructor
    static_vector(const static_vector& other) : m_size(other.m_size) {
        std::uninitialized_copy(other.begin(), other.end(), begin());
    }

    // Copy assignment
    static_vector& operator=(const static_vector& other) {
        if (&other == this)
            return *this;
        clear();
        m_size = other.m_size;
        std::uninitialized_copy(other.begin(), other.end(), begin());
        return *this;
    }

    // Move constructor
    static_vector(static_vector&& other) : m_size(other.m_size) {
        std::uninitialized_copy(
            std::make_move_iterator(other.begin()),
            std::make_move_iterator(other.end()), //
            begin());
    }

    // Move assignment
    static_vector& operator=(static_vector&& other) {
        if (&other == this)
            return *this;
        clear();
        m_size = other.m_size;
        std::uninitialized_copy(
            std::make_move_iterator(other.begin()),
            std::make_move_iterator(other.end()), begin());
        return *this;
    }

    // Iterator constructor with basic SFINAE mechanism to cancel use with
    // non-iterator types
    template <
        typename Iter, typename = decltype(*std::declval<Iter&>()),
        typename = decltype(++std::declval<Iter&>())>
    static_vector(Iter input_begin, Iter input_end) {
        m_size = std::distance(input_begin, input_end);
        if (m_size > static_capacity)
        {
            throw std::out_of_range("size()");
        }
        std::uninitialized_copy(input_begin, input_end, begin());
    }

    // Destructor
    // Ensures: all objects are destructed properly, but trivial destructors are
    // not run.
    // Complexity: O(size()) for non-trivially destructible value_type,
    // otherwise constant.
    ~static_vector() { clear(); }

    // FIXME the equivalent to std::vector::assign functions can be implemented
    // the same way as the corresponding ctors

    // ELEMENT ACCESS

    // Element access with bounds checking
    // Requires: no requirement on index, it is checked at runtime
    // Ensures: element returned or exception thrown without out-of-bounds
    // memory access
    // Returns: the element at `index`
    // Complexity: constant
    // Exceptions: std::out_of_range
    reference at(size_type index) {
        if (index < m_size)
            return data(index);
        else
            throw std::out_of_range("index");
    }
    const_reference at(size_type index) const {
        if (index < m_size)
            return data(index);
        else
            throw std::out_of_range("index");
    }

    // Element access without bounds checking
    // Requires: index is less than size
    // Returns: the element at `index`
    // Complexity: constant
    // Exceptions: noexcept
    reference operator[](size_t index) noexcept { return data(index); }
    const_reference operator[](size_t index) const noexcept {
        return data(index);
    }

    // Get first element, equivalent to v[0]
    // Requires: size() > 0
    reference front() noexcept { return data(0); }
    const_reference front() const noexcept { return data(0); }

    // Get last element, equivalent to v[v.size() - 1]
    // Requires: size() > 0
    reference back() noexcept { return data(m_size - 1); }
    const_reference back() const noexcept { return data(m_size - 1); }

    // Get underlying data as a raw pointer
    // Equivalent to &v[0]
    pointer data() noexcept { return reinterpret_cast<pointer>(&m_data[0]); }
    const_pointer data() const noexcept {
        return reinterpret_cast<const_pointer>(&m_data[0]);
    }

    // ITERATORS

    // Get iterator to the first (`begin`) last (`end`) stored element.
    // Complexity: constant
    // Exceptions: noexcept
    // Returns: iterator to the first element
    iterator begin() noexcept { return data(); }
    const_iterator begin() const noexcept { return data(); }
    const_iterator cbegin() const noexcept { return data(); }
    // Returns: iterator to one past the last element
    iterator end() noexcept { return data() + m_size; }
    const_iterator end() const noexcept { return data() + m_size; }
    const_iterator cend() const noexcept { return data() + m_size; }

    // Reverse iterators behave the same as their regular pairs except that
    // `rbegin()` refers to the last element (`end() - 1`) and `rend()` refers
    // to one past `begin()`
    // Returns: iterator to the first element in reverse order
    reverse_iterator rbegin() noexcept { return data() + m_size; }
    const_reverse_iterator rbegin() const noexcept { return data() + m_size; }
    const_reverse_iterator crbegin() const noexcept { return data() + m_size; }
    // Returns: iterator to one past the last element in reverse order
    reverse_iterator rend() noexcept { return data(); }
    const_reverse_iterator rend() const noexcept { return data(); }
    const_reverse_iterator crend() const noexcept { return data(); }

    // CAPACITY

    // These functions all have wide contracts, constant complexity and noexcept
    // exception specification.

    // The number of elements
    size_type size() const noexcept { return m_size; }

    // Is it empty?
    bool empty() const noexcept { return m_size == 0; }

    // Is it full? Note: added in addition to std::vector interface
    bool full() const noexcept { return m_size == static_capacity; }

    // Max possible size
    size_type capacity() const noexcept { return static_capacity; }

    // Max possible size
    size_type max_size() const noexcept { return static_capacity; }

    // reserve intentionally not defined, but it could be a no-op.
    // shrink_to_fit intentionally not defined, but it could be a no-op.

    // MODIFIERS

    // Clear the vector
    // Ensures: size() = 0, all objects destructed
    // Complexity: O(size()) for non-trivially destructible value_type,
    // otherwise constant.
    void clear() {
        if (!std::is_trivially_destructible<value_type>::value)
            std::for_each(begin(), end(), [](reference r) { r.~value_type(); });
        m_size = 0;
    }

    // Insert element at specific position
    // Requires: valid `pos` iterator, including begin() and end() inclusive.
    // Ensures: new `value_type` copy_constructed at `pos`
    // Complexity: exactly `end()` - `pos` moves and one copy
    iterator insert(const_iterator pos, const value_type& value) {
        if (full())
            throw std::out_of_range("size()");
        // Need mutable iterator to change items. Cast is legal in non-const
        // methos.
        iterator mut_pos = const_cast<iterator>(pos);
        // move_backward is recommended when the end of the target range is
        // outside the input range, last element is moved first
        std::move_backward(mut_pos, end(), end() + 1);
        // Construct value, do not assign nonexistent
        new (mut_pos) value_type(value);
        m_size++;
        return mut_pos;
    }
    iterator insert(const_iterator pos, value_type&& value) {
        if (full())
            throw std::out_of_range("size()");
        // Need mutable iterator to change items. Cast is legal in non-const
        // methos.
        iterator mut_pos = const_cast<iterator>(pos);
        // move_backward is recommended when the end of the target range is
        // outside the input range, last element is moved first
        std::move_backward(mut_pos, end(), end() + 1);
        // Construct value, do not assign nonexistent
        new (mut_pos) value_type(std::move(value));
        m_size++;
        return mut_pos;
    }

    // Insert `count` copies of `value` at `pos`
    iterator
    insert(const_iterator pos, size_type count, const value_type& value) {
        if (m_size + count < m_size /*ovf*/ || static_capacity < m_size + count)
            throw std::out_of_range("count");
        // Need mutable iterator to change items. Cast is legal in non-const
        // methos.
        iterator mut_pos = const_cast<iterator>(pos);
        // move_backward is recommended when the end of the target range is
        // outside the input range, last element is moved first
        std::move_backward(mut_pos, end(), end() + count);
        // Construct value, do not assign nonexistent
        std::for_each(
            storage_begin() + (mut_pos - begin()),
            storage_begin() + (mut_pos - begin()) + count,
            [&](storage_type& store) { new (&store) value_type(value); });
        m_size += count;
        return mut_pos;
    }
    template <typename InputIter>
    auto
    insert(const_iterator pos, InputIter insert_begin, InputIter insert_end)
        -> std::enable_if_t<
            std::is_same<
                typename std::iterator_traits<InputIter>::reference,
                decltype(*insert_begin)>::value,
            iterator> {
        auto count = std::distance(insert_begin, insert_end);
        if (count < 0 ||
            m_size + static_cast<size_type>(count) < m_size /*ovf*/ ||
            static_capacity < m_size + static_cast<size_type>(count)) {
            throw std::out_of_range("std::distance(begin, end)");
        }
        // Need mutable iterator to change items. Cast is legal in non-const
        // methos.
        iterator mut_pos = const_cast<iterator>(pos);
        // move_backward is recommended when the end of the target range is
        // outside the input range, last element is moved first
        std::move_backward(mut_pos, end(), end() + count);
        std::for_each(
            storage_begin() + (mut_pos - begin()),
            storage_begin() + (mut_pos - begin()) + count,
            [&](storage_type& store) {
                new (&store) value_type(*insert_begin++);
            });
        m_size += count;
        return mut_pos;
    }
    // TODO insert(const_iterator pos, InputIter begin, InputIter end)
    // TODO insert(init_list)

    // Inserts new object at `pos` using `args...` as constructor arguments
    // TODO documentation
    template <typename... CtorArgs>
    iterator emplace(const_iterator pos, CtorArgs&&... args) {
        if (full())
            throw std::out_of_range("size()");
        // Need mutable iterator to change items. Cast is legal in non-const
        // methos.
        iterator mut_pos = const_cast<iterator>(pos);
        // move_backward is recommended when the end of the target range is
        // outside the input range, last element is moved first
        std::move_backward(mut_pos, end(), end() + 1);
        // Construct value, do not assign nonexistent
        new (mut_pos) value_type(std::forward<CtorArgs>(args)...);
        m_size++;
        return mut_pos;
    }

    // Erase element at `pos`
    // TODO docs
    iterator erase(const_iterator pos) {
        iterator mut_pos = const_cast<iterator>(pos);
        mut_pos->~value_type();
        // move forward, starting from mut_pos and going towards end()
        std::move(mut_pos + 1, end(), mut_pos);
        m_size--;
        return mut_pos;
    }

    // TODO erase(const_iterator begin, const_iterator end)

    // Add `value` at the end of the list
    void push_back(const value_type& value) {
        if (full())
            throw std::out_of_range("size()");
        new (storage_end()) value_type(value);
        m_size++;
    }
    void push_back(value_type&& value) {
        if (full())
            throw std::out_of_range("size()");
        new (storage_end()) value_type(std::move(value));
        m_size++;
    }

    // TODO emplace_back
    // TODO pop_back
    // TODO resize
    // TODO swap

private:
    // Use a specific storage type to satisfy alignment requirements
    using storage_type =
        std::aligned_storage_t<sizeof(value_type), alignof(value_type)>;
    // The array providing the inline storage for the elements.
    std::array<storage_type, static_capacity> m_data = {};
    // The current occupied size of the static_vector
    size_type m_size = 0;

    // Get data by index, used for convenience instead of (*this)[index]
    // Note that as opposed to data(), these return a `reference`, not `pointer`
    reference data(size_t index) noexcept {
        return *reinterpret_cast<pointer>(&m_data[index]);
    }
    const_reference data(size_t index) const noexcept {
        return *reinterpret_cast<const_pointer>(&m_data[index]);
    }

    // Get iterators for storage
    storage_type* storage_begin() noexcept { return &m_data[0]; }
    storage_type* storage_end() noexcept { return &m_data[m_size]; }
};

// NON-MEMBER OPERATORS

// TODO ==, !=
// TODO < > <= >=

} // namespace stlpb

#endif // PALOTASB_STATIC_VECTOR_H
