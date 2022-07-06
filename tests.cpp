#include <palotasb/static_vector.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <tuple>

using namespace stlpb;

template<typename T>
std::ostream& print_cont(std::ostream& os, T const& v){
    os << "[ ";

    for (int x = 0; x < v.size(); x++)
    {
        os << v[x];
        if (x != v.size() - 1)
        {
            os << ", ";
        }
    }

    os << " ]";

    return os;
}

template<typename T, size_t N>
std::ostream& operator<<(std::ostream& os, static_vector<T, N> const& v){
    return print_cont(os, v);
}

template<typename T, size_t N>
std::ostream& operator<<(std::ostream& os, std::array<T, N> const& v){
    return print_cont(os, v);
}

void debug_print_impl()
{
}

template<typename First, typename... T>
void debug_print_impl(First const& f, T const&... t)
{
    std::cerr << f;
    if (sizeof...(T) != 0)
    {
        std::cerr << ", ";
    }
    debug_print_impl(t...);
}

template<typename... T>
void debug_print(T const&... t)
{
    if (sizeof...(T) != 0)
    {
        std::cerr << "(";
    }
    debug_print_impl(t...);
    if (sizeof...(T) != 0)
    {
        std::cerr << ")";
    }
}

template<typename T>
auto debug_transform(T const& t)
{
    return t;
}

template<typename... T>
void assert_failure(const char* expression, const char* file, long line, T const&... t) {
    std::cerr << "Assertion failure: " << expression;
    debug_print(debug_transform(t)...);
    std::cerr << " failed at " << file
              << ':' << line << ".\n";
    std::exit(1); // Exit program early
}

#define ASSERT(e) ((e) ? true : (assert_failure(#e, __FILE__, __LINE__), false))
#define ASSERT_MESSAGE(e, ...) ((e) ? true \
        : (assert_failure(#e " note (" #__VA_ARGS__ ") == ", __FILE__, __LINE__, __VA_ARGS__), false))

#define ASSERT_EQUAL(a, b) ASSERT_MESSAGE(a == b, a, b)
#define ASSERT_UNEQUAL(a, b) ASSERT_MESSAGE(a != b, a, b)


template<typename First, typename... T>
std::array<First, sizeof...(T) + 1> make_array(First&& f, T&&... t)
{
    return { std::forward<First>(f), std::forward<T>(t)... };
}

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

static_vector<int, 10> get_123_vector()
{
    return {1, 2, 3};
}

static_vector<int, 10> get_empty_vector()
{
    return {};
}

template<typename F, size_t N>
void insert_single_test(int index, int data,
        F const& get_initial_vector, std::array<int, N> const& final_status)
{
    auto v = get_initial_vector();
    v.insert(v.begin() + index, data);

    ASSERT_MESSAGE(
            std::equal(v.begin(), v.end(), final_status.begin(), final_status.end()),
            v, final_status);
}

template<typename F, size_t N1, size_t N2>
void insert_range_test(int index, std::array<int, N1> data,
        F const& get_initial_vector, std::array<int, N2> const& final_status)
{
    auto v = get_initial_vector();
    v.insert(v.begin() + index, data.begin(), data.end());

    ASSERT_MESSAGE(
            std::equal(v.begin(), v.end(), final_status.begin(), final_status.end()),
            v, final_status);
}

int main(int, char* []) {
    //
    try {
        {
            // Default ctor; capacity
            static_vector<int, 10> v;
            ASSERT_EQUAL(v.capacity(), 10);
            ASSERT_EQUAL(v.size(), 0);
        }
        {
            // "N copy of X" ctor, case N = 0
            static_vector<int, 10> v(0, 100);
            ASSERT_EQUAL(v.size(), 0);
        }
        {
            // "N copy of X" ctor, case 0 < N < capacity
            static_vector<int, 10> v(3, 100);
            ASSERT_EQUAL(v.size(), 3);
            for (auto x : v)
                ASSERT_EQUAL(x, 100);
        }
        {
            // "N copy of X" ctor, case N = capacity
            static_vector<int, 10> v(10, 100);
            ASSERT_EQUAL(v.size(), 10);
            for (auto x : v)
                ASSERT_EQUAL(x, 100);
        }
        {
            // Initializer list constructor
            static_vector<int, 10> v{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            ASSERT_EQUAL(v.size(), 10);
            int i = 1;
            for (auto x : v)
            {
                ASSERT_EQUAL(x, i);
                i++;
            }
        }
        {
            // Iterator constructor
            int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v{std::begin(a), std::end(a)};
            ASSERT_EQUAL(v.size(), 10);
            int i = 1;
            for (auto x : v)
            {
                ASSERT_EQUAL(x, i);
                i++;
            }
        }
        {
            // Copy ctor with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v{u};
            ASSERT_EQUAL(v.size(), 10);
            int i = 1;
            for (auto x : v)
            {
                ASSERT_EQUAL(x, i);
                i++;
            }
        }
        {
            // Copy ctor with nontrivially copyable type
            static_vector<Copyable, 10> u(10, Copyable{});
            static_vector<Copyable, 10> v{u};
            ASSERT_EQUAL(v.size(), 10);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Copy assignment with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v;
            v = u;
            ASSERT_EQUAL(v.size(), 10);
            int i = 1;
            for (auto x : v)
            {
                ASSERT_EQUAL(x, i);
                i++;
            }
        }
        {
            // Copy assignment with nontrivially-copyable types
            static_vector<Copyable, 10> u(10, Copyable{});
            static_vector<Copyable, 10> v;
            v = u;
            ASSERT_EQUAL(v.size(), 10);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Move ctor with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v{std::move(u)};
            ASSERT_EQUAL(v.size(), 10);
            int i = 1;
            for (auto x : v)
            {
                ASSERT_EQUAL(x, i);
                i++;
            }
        }
        {
            // Move ctor with nontrivially movable type
            static_vector<Movable, 10> u(10);
            static_vector<Movable, 10> v{std::move(u)};
            ASSERT_EQUAL(v.size(), 10);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Move assignment with ints
            static_vector<int, 10> u{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            static_vector<int, 10> v;
            v = std::move(u);
            ASSERT_EQUAL(v.size(), 10);
            int i = 1;
            for (auto x : v)
            {
                ASSERT_EQUAL(x, i);
                i++;
            }
        }
        {
            // Move assignment with nontrivially-movable types
            static_vector<Movable, 10> u(10);
            static_vector<Movable, 10> v;
            v = std::move(u);
            ASSERT_EQUAL(v.size(), 10);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        insert_single_test(0, 100, get_empty_vector, make_array(100));

        insert_single_test(0, 100, get_123_vector, make_array( 100, 1, 2, 3 ));
        insert_single_test(1, 100, get_123_vector, make_array( 1, 100, 2, 3 ));
        insert_single_test(2, 100, get_123_vector, make_array( 1, 2, 100, 3 ));
        insert_single_test(3, 100, get_123_vector, make_array( 1, 2, 3, 100 ));

        insert_range_test(0, make_array(100, 200), get_empty_vector,
                make_array( 100, 200 ));

        insert_range_test(0, make_array(100, 200), get_123_vector,
                make_array( 100, 200, 1, 2, 3 ));
        insert_range_test(1, make_array(100, 200), get_123_vector,
                make_array( 1, 100, 200, 2, 3 ));
        insert_range_test(2, make_array(100, 200), get_123_vector,
                make_array( 1, 2, 100, 200, 3 ));
        insert_range_test(3, make_array(100, 200), get_123_vector,
                make_array( 1, 2, 3, 100, 200 ));

        {
            // Insert nontrivial type into empty vector
            static_vector<Copyable, 10> v;
            const Copyable c;
            v.insert(v.begin(), c);
            ASSERT_EQUAL(v.size(), 1);
            ASSERT(v[0].verify());
        }
        {
            // Insert nontrivial type into beginning of vector
            static_vector<Copyable, 10> v(3);
            const Copyable c;
            v.insert(v.begin(), c);
            ASSERT_EQUAL(v.size(), 4);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Insert nontrivial type into middle of vector
            static_vector<Copyable, 10> v(3);
            const Copyable c;
            v.insert(v.begin() + 1, c);
            ASSERT_EQUAL(v.size(), 4);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Insert nontrivial type into end of vector
            static_vector<Copyable, 10> v(3);
            const Copyable c;
            v.insert(v.end(), c);
            ASSERT_EQUAL(v.size(), 4);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Insert move-only type into beginning of vector
            static_vector<Movable, 10> v(3);
            v.insert(v.begin(), {});
            ASSERT_EQUAL(v.size(), 4);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Insert move-only type into middle of vector
            static_vector<Movable, 10> v(3);
            v.insert(v.begin() + 1, {});
            ASSERT_EQUAL(v.size(), 4);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Insert move-only type into end of vector
            static_vector<Movable, 10> v(3);
            v.insert(v.end(), {});
            ASSERT_EQUAL(v.size(), 4);
            for (const auto& x : v)
                ASSERT(x.verify());
        }
        {
            // Insert multiple copies of trivial types into middle
            static_vector<int, 10> v{1, 2, 3};
            v.insert(v.begin() + 1, 2, 100);
            ASSERT_EQUAL(v.size(), 5);
            ASSERT_EQUAL(v[0], 1);
            ASSERT_EQUAL(v[1], 100);
            ASSERT_EQUAL(v[2], 100);
            ASSERT_EQUAL(v[3], 2);
            ASSERT_EQUAL(v[4], 3);
            // TODO add more exhaustive tests for this method
            // TODO test return value
        }
        {
            // Emplace element
            static_vector<std::tuple<Movable, Copyable>, 10> v(3);
            const Copyable c;
            v.emplace(v.begin() + 1, Movable{}, c);
            ASSERT_EQUAL(v.size(), 4);
            for (const auto& x : v) {
                ASSERT(std::get<0>(x).verify());
                ASSERT(std::get<1>(x).verify());
            }
            // TODO maybe add more exhaustive tests for this method
            // TODO test return value
        }
        {
            // Erase one element
            static_vector<int, 10> v{1, 2, 3};
            v.erase(v.begin() + 1);
            ASSERT_EQUAL(v.size(), 2);
            ASSERT_EQUAL(v[0], 1);
            ASSERT_EQUAL(v[1], 3);
        }
        {

            static_vector<Copyable, 10> v{3};
            v.erase(v.begin() + 1);
            ASSERT_EQUAL(v.size(), 2);
            for (const auto& x : v)
                ASSERT(x.verify());
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
            ASSERT_EQUAL(z.size(), v.size());
            // check that the two sorts produced the same result
            ASSERT(std::all_of(begin(z), end(z), [](bool b) { return b; }));
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
        ASSERT_EQUAL(Copyable::constructed(), 0);
        ASSERT_EQUAL(Movable::constructed(), 0);
    }

    return 0;
}

