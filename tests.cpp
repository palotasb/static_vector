#include <palotasb/static_vector.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <tuple>

using namespace stlpb;

void assert_failure(const char* expression, const char* file, long line);

#define ASSERT(e) ((e) ? true : (assert_failure(#e, __FILE__, __LINE__), false))

// Self-referential object that tests whether copies are semantically correct,
// using the copy constructors of stored objects.
struct Copyable {
    Copyable() : self(this) { constructed_++; }
    Copyable(const Copyable& other) : self(other.verify() ? this : nullptr) {
        constructed_++;
    }
    Copyable& operator=(const Copyable& other) {
        self = other.verify() ? this : nullptr;
        return *this;
    }
    Copyable(Copyable&& other)
        : Copyable(static_cast<const Copyable&>(other)) {}
    Copyable& operator=(Copyable&& other) {
        return (*this) = static_cast<const Copyable&>(other);
    }
    ~Copyable() { constructed_--; }

    bool verify() const noexcept { return self == this; }

    static int constructed() noexcept { return constructed_; }

private:
    const Copyable* self;
    static int constructed_;
};

int Copyable::constructed_ = 0;

// Self-referential object that tests whether moves are semantically correct.
struct Movable {
    Movable() : self(this) { constructed_++; }
    Movable(const Movable&) = delete;
    Movable& operator=(const Movable&) = delete;
    Movable(Movable&& other) : self(other.verify() ? this : nullptr) {
        other.self = nullptr;
        constructed_++;
    }
    Movable& operator=(Movable&& other) {
        self = other.verify() ? this : nullptr;
        other.self = nullptr;
        return *this;
    }
    ~Movable() { constructed_--; }

    bool verify() const noexcept { return self == this; }

    static int constructed() noexcept { return constructed_; }
private:
    const Movable* self;
    static int constructed_;
};

int Movable::constructed_ = 0;

int main(int, char* []) {
    //
    try {
        {
            // Default ctor; capacity
            static_vector<int, 10> v;
            if (!ASSERT(v.capacity() == 10))
                return 1;
            if (!ASSERT(v.size() == 0))
                return 1;
        }
        {
            // "N copy of X" ctor, case N = 0
            static_vector<int, 10> v(0, 100);
            if (!ASSERT(v.size() == 0))
                return 1;
        }
        {
            // "N copy of X" ctor, case 0 < N < capacity
            static_vector<int, 10> v(3, 100);
            if (!ASSERT(v.size() == 3))
                return 1;
            for (auto x : v)
                if (!ASSERT(x == 100))
                    return 1;
        }
        {
            // "N copy of X" ctor, case N = capacity
            static_vector<int, 10> v(10, 100);
            if (!ASSERT(v.size() == 10))
                return 1;
            for (auto x : v)
                if (!ASSERT(x == 100))
                    return 1;
        }
        {
            // Initializer list constructor
            static_vector<int, 10> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            if (!ASSERT(v.size() == 10))
                return 1;
            int i = 1;
            for (auto x : v)
                if (!ASSERT(x == i++))
                    return 1;
        }
        {
            // Iterator constructor
            int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v{std::begin(a), std::end(a)};
            if (!ASSERT(v.size() == 10))
                return 1;
            int i = 1;
            for (auto x : v)
                if (!ASSERT(x == i++))
                    return 1;
        }
        {
            // Copy ctor with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v{u};
            if (!ASSERT(v.size() == 10))
                return 1;
            int i = 1;
            for (auto x : v)
                if (!ASSERT(x == i++))
                    return 1;
        }
        {
            // Copy ctor with nontrivially copyable type
            static_vector<Copyable, 10> u(10, Copyable{});
            static_vector<Copyable, 10> v{u};
            if (!ASSERT(v.size() == 10))
                return 1;
            for (const auto& x : v)
                if (!ASSERT(x.verify()))
                    return 1;
        }
        {
            // Copy assignment with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v;
            v = u;
            if (!ASSERT(v.size() == 10))
                return 1;
            int i = 1;
            for (auto x : v)
                if (!ASSERT(x == i++))
                    return 1;
        }
        {
            // Copy assignment with nontrivially-copyable types
            static_vector<Copyable, 10> u(10, Copyable{});
            static_vector<Copyable, 10> v;
            v = u;
            if (!ASSERT(v.size() == 10))
                return 1;
            for (const auto& x : v)
                if (!ASSERT(x.verify()))
                    return 1;
        }
        {
            // Move ctor with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v{std::move(u)};
            if (!ASSERT(v.size() == 10))
                return 1;
            int i = 1;
            for (auto x : v)
                if (!ASSERT(x == i++))
                    return 1;
        }
        {
            // Move ctor with nontrivially movable type
            static_vector<Movable, 10> u(10);
            static_vector<Movable, 10> v{std::move(u)};
            if (!ASSERT(v.size() == 10))
                return 1;
            for (const auto& x : v)
                if (!ASSERT(x.verify()))
                    return 1;
        }
        {
            // Move assignment with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v;
            v = std::move(u);
            if (!ASSERT(v.size() == 10))
                return 1;
            int i = 1;
            for (auto x : v)
                if (!ASSERT(x == i++))
                    return 1;
        }
        {
            // Move assignment with nontrivially-movable types
            static_vector<Movable, 10> u(10);
            static_vector<Movable, 10> v;
            v = std::move(u);
            if (!ASSERT(v.size() == 10))
                return 1;
            for (const auto& x : v)
                if (!ASSERT(x.verify()))
                    return 1;
        }
        {
            // Insert trivial type into empty vector
            static_vector<int, 10> v;
            v.insert(v.begin(), 100);
            if (!(ASSERT(v.size() == 1)))
                return 1;
            if (!(ASSERT(v[0] == 100)))
                return 1;
        }
        {
            // Insert trivial type into beginning of vector
            static_vector<int, 10> v{1, 2, 3};
            v.insert(v.begin(), 100);
            if (!(ASSERT(v.size() == 4)))
                return 1;
            if (!(ASSERT(v[0] == 100)))
                return 1;
            if (!(ASSERT(v[1] == 1)))
                return 1;
            if (!(ASSERT(v[2] == 2)))
                return 1;
            if (!(ASSERT(v[3] == 3)))
                return 1;
        }
        {
            // Insert trivial type into middle of vector
            static_vector<int, 10> v{1, 2, 3};
            v.insert(v.begin() + 1, 100);
            if (!(ASSERT(v.size() == 4)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 100)))
                return 1;
            if (!(ASSERT(v[2] == 2)))
                return 1;
            if (!(ASSERT(v[3] == 3)))
                return 1;
        }
        {
            // Insert trivial type into end of vector
            static_vector<int, 10> v{1, 2, 3};
            v.insert(v.end(), 100);
            if (!(ASSERT(v.size() == 4)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 2)))
                return 1;
            if (!(ASSERT(v[2] == 3)))
                return 1;
            if (!(ASSERT(v[3] == 100)))
                return 1;
        }
        {
            // Insert trivial type into empty vector
            int data[] = {1, 2, 3};
            static_vector<int, 10> v;
            v.insert(v.begin(), std::begin(data), std::end(data));
            if (!(ASSERT(v.size() == 3)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 2)))
                return 1;
            if (!(ASSERT(v[2] == 3)))
                return 1;
        }
        {
            // Insert trivial type into beginning of vector
            int data[] = {1, 2};
            static_vector<int, 10> v{3, 4};
            v.insert(v.begin(), std::begin(data), std::end(data));
            if (!(ASSERT(v.size() == 4)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 2)))
                return 1;
            if (!(ASSERT(v[2] == 3)))
                return 1;
            if (!(ASSERT(v[3] == 4)))
                return 1;
        }
        {
            // Insert trivial type into middle of vector
            int data[] = {2, 3};
            static_vector<int, 10> v{1, 4};
            v.insert(v.begin() + 1, std::begin(data), std::end(data));
            if (!(ASSERT(v.size() == 4)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 2)))
                return 1;
            if (!(ASSERT(v[2] == 3)))
                return 1;
            if (!(ASSERT(v[3] == 4)))
                return 1;
        }
        {
            // Insert trivial type into end of vector
            int data[] = {3, 4};
            static_vector<int, 10> v{1, 2};
            v.insert(v.end(), std::begin(data), std::end(data));
            if (!(ASSERT(v.size() == 4)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 2)))
                return 1;
            if (!(ASSERT(v[2] == 3)))
                return 1;
            if (!(ASSERT(v[3] == 4)))
                return 1;
        }
        {
            // Insert nontrivial type into empty vector
            static_vector<Copyable, 10> v;
            const Copyable c;
            v.insert(v.begin(), c);
            if (!(ASSERT(v.size() == 1)))
                return 1;
            if (!(ASSERT(v[0].verify())))
                return 1;
        }
        {
            // Insert nontrivial type into beginning of vector
            static_vector<Copyable, 10> v(3);
            const Copyable c;
            v.insert(v.begin(), c);
            if (!(ASSERT(v.size() == 4)))
                return 1;
            for (const auto& x : v)
                if (!(ASSERT(x.verify())))
                    return 1;
        }
        {
            // Insert nontrivial type into middle of vector
            static_vector<Copyable, 10> v(3);
            const Copyable c;
            v.insert(v.begin() + 1, c);
            if (!(ASSERT(v.size() == 4)))
                return 1;
            for (const auto& x : v)
                if (!(ASSERT(x.verify())))
                    return 1;
        }
        {
            // Insert nontrivial type into end of vector
            static_vector<Copyable, 10> v(3);
            const Copyable c;
            v.insert(v.end(), c);
            if (!(ASSERT(v.size() == 4)))
                return 1;
            for (const auto& x : v)
                if (!(ASSERT(x.verify())))
                    return 1;
        }
        {
            // Insert move-only type into beginning of vector
            static_vector<Movable, 10> v(3);
            v.insert(v.begin(), {});
            if (!(ASSERT(v.size() == 4)))
                return 1;
            for (const auto& x : v)
                if (!(ASSERT(x.verify())))
                    return 1;
        }
        {
            // Insert move-only type into middle of vector
            static_vector<Movable, 10> v(3);
            v.insert(v.begin() + 1, {});
            if (!(ASSERT(v.size() == 4)))
                return 1;
            for (const auto& x : v)
                if (!(ASSERT(x.verify())))
                    return 1;
        }
        {
            // Insert move-only type into end of vector
            static_vector<Movable, 10> v(3);
            v.insert(v.end(), {});
            if (!(ASSERT(v.size() == 4)))
                return 1;
            for (const auto& x : v)
                if (!(ASSERT(x.verify())))
                    return 1;
        }
        {
            // Insert multiple copies of trivial types into middle
            static_vector<int, 10> v{1, 2, 3};
            v.insert(v.begin() + 1, 2, 100);
            if (!(ASSERT(v.size() == 5)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 100)))
                return 1;
            if (!(ASSERT(v[2] == 100)))
                return 1;
            if (!(ASSERT(v[3] == 2)))
                return 1;
            if (!(ASSERT(v[4] == 3)))
                return 1;
            // TODO add more exhaustive tests for this method
            // TODO test return value
        }
        {
            // Emplace element
            static_vector<std::tuple<Movable, Copyable>, 10> v(3);
            const Copyable c;
            v.emplace(v.begin() + 1, Movable{}, c);
            if (!(ASSERT(v.size() == 4)))
                return 1;
            for (const auto& x : v) {
                if (!(ASSERT(std::get<0>(x).verify())))
                    return 1;
                if (!(ASSERT(std::get<1>(x).verify())))
                    return 1;
            }
            // TODO maybe add more exhaustive tests for this method
            // TODO test return value
        }
        {
            // Erase one element
            static_vector<int, 10> v{1, 2, 3};
            v.erase(v.begin() + 1);
            if (!(ASSERT(v.size() == 2)))
                return 1;
            if (!(ASSERT(v[0] == 1)))
                return 1;
            if (!(ASSERT(v[1] == 3)))
                return 1;
        }
        {

            static_vector<Copyable, 10> v{3};
            v.erase(v.begin() + 1);
            if (!(ASSERT(v.size() == 2)))
                return 1;
            for (const auto& x : v)
                if (!(ASSERT(x.verify())))
                    return 1;
        }
        {
            // Test STL algorithm support: std::rotate
            // Example code taken from:
            static_vector<int, 20> v{2, 4, 2, 0, 5, 10, 7, 3, 7, 1};
            static_vector<int, 20> w = v;

            // insertion sort from:
            // https://en.cppreference.com/w/cpp/algorithm/rotate
            for (auto i = v.begin(); i != v.end(); ++i) {
                std::rotate(std::upper_bound(v.begin(), i, *i), i, i + 1);
            }

            using std::begin;
            using std::end;
            std::sort(begin(w), end(w)); // regular sort

            // see if the two sorts result in a different order
            static_vector<bool, 20> z;
            std::transform(
                begin(v), end(v), begin(w), std::back_inserter(z),
                std::equal_to<>{});

            // check that equality values are there
            if (!ASSERT(z.size() == v.size()))
                return 1;
            // check that the two sorts produced the same result
            if (!(ASSERT(
                    std::all_of(begin(z), end(z), [](bool b) { return b; }))))
                return 1;
        }
        // TODO test all public methods with all reasonable inputs including
        // edge cases
    } catch (std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << "\n";
        return 1;
    }

    {
        // Check that all destructors ran properly
        // This should be the last test case!
        if (!ASSERT(Copyable::constructed() == 0))
            return 1;
        if (!ASSERT(Movable::constructed() == 0))
            return 1;
    }

    return 0;
}

void assert_failure(const char* expression, const char* file, long line) {
    std::cerr << "Assertion failure: " << expression << " failed at " << file
              << ':' << line << ".\n";
}
