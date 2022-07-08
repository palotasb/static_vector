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

std::ostream& operator<<(std::ostream& os, char c){
    return os.operator<<(+c);
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
    debug_print(t...);
    std::cerr << " failed at " << file
              << ':' << line << ".\n";
    std::exit(1); // Exit program early
}

#define ASSERT(e) ((e) ? true : (assert_failure(#e, __FILE__, __LINE__), false))
#define ASSERT_MESSAGE(e, ...) ((e) ? true \
        : (assert_failure(#e " note (" #__VA_ARGS__ ") == ", __FILE__, __LINE__, __VA_ARGS__), false))

#define ASSERT_EQUAL(a, b) ASSERT_MESSAGE(a == b, a, b)
#define ASSERT_UNEQUAL(a, b) ASSERT_MESSAGE(a != b, a, b)


template<typename... T>
std::array<char, sizeof...(T)> make_c_array(T&&... t)
{
    return { static_cast<char>(t)... };
}

// Self-referential object that tests whether copies are semantically correct,
// using the copy constructors of stored objects.
//
// It also stores a payload in order to allow us to treat it "like" an int or
// whatever in order to allow us to verify orders
template<typename T = char>
struct Copyable_t {
    Copyable_t() : self(this) { constructed_++; }

    template<typename U,
        typename=std::enable_if_t<
            std::is_same<std::decay_t<U>, T>::value
        >
    >
    Copyable_t(U&& t) : Copyable_t() {
        inner = std::forward<U>(t);
    }

    Copyable_t(const Copyable_t& other)
        : self(other.verify() ? this : nullptr), inner(other.inner) {
        constructed_++;
    }
    Copyable_t& operator=(const Copyable_t& other) {
        self = other.verify() ? this : nullptr;
        this->inner = other.inner;
        return *this;
    }
    Copyable_t(Copyable_t&& other)
        : Copyable_t(static_cast<const Copyable_t&>(other)) {}
    Copyable_t& operator=(Copyable_t&& other) {
        return (*this) = static_cast<const Copyable_t&>(other);
    }
    ~Copyable_t() { constructed_--; }

    bool verify() const noexcept { return self == this; }

    static int constructed() noexcept { return constructed_; }

    friend bool operator==(Copyable_t const& c, T const& t)
    {
        return c.inner == t;
    }

    friend bool operator==(T const& t, Copyable_t const& c)
    {
        return c.inner == t;
    }

    friend std::ostream& operator<<(std::ostream& os, Copyable_t c){
        os << "Copyable(" << c.inner << (c.verify() ? "" : " !") << ")";
        return os;
    }

private:
    const Copyable_t* self;
    T inner = {};
    static int constructed_;
};

template<typename T>
int Copyable_t<T>::constructed_ = 0;

using Copyable = Copyable_t<>;

// Self-referential object that tests whether moves are semantically correct.
template<typename T = char>
struct Movable_t {
    Movable_t() : self(this) { constructed_++; }

    template<typename U,
        typename=std::enable_if_t<
            std::is_same<std::decay_t<U>, T>::value
        >
    >
    Movable_t(U&& t) : Movable_t() {
        inner = std::forward<U>(t);
    }

    Movable_t(const Movable_t&) = delete;
    Movable_t& operator=(const Movable_t&) = delete;
    Movable_t(Movable_t&& other) : self(other.verify() ? this : nullptr),
                                   inner(other.inner)
    {
        other.self = nullptr;
        constructed_++;
    }
    Movable_t& operator=(Movable_t&& other) {
        self = other.verify() ? this : nullptr;
        inner = other.inner;
        other.self = nullptr;
        return *this;
    }
    ~Movable_t() { constructed_--; }

    bool verify() const noexcept { return self == this; }

    static int constructed() noexcept { return constructed_; }

    friend bool operator==(Movable_t const& c, T const& t)
    {
        return c.inner == t;
    }

    friend bool operator==(T const& t, Movable_t const& c)
    {
        return c.inner == t;
    }

    friend std::ostream& operator<<(std::ostream& os, Movable_t const& c){
        os << "Movable(" << c.inner << (c.verify() ? "" : " !") << ")";
        return os;
    }
private:
    const Movable_t* self;
    T inner = {};
    static int constructed_;
};

template<typename T>
int Movable_t<T>::constructed_ = 0;

using Movable = Movable_t<>;

template<typename T>
static_vector<T, 10> get_123_vector()
{
    // Do the emplacing to allow for move only types
    static_vector<T, 10> ret;

    ret.emplace(std::end(ret), static_cast<char>(1));
    ret.emplace(std::end(ret), static_cast<char>(2));
    ret.emplace(std::end(ret), static_cast<char>(3));

    return std::move(ret);
}

template<typename T>
static_vector<T, 10> get_full_vector()
{
    // Do the emplacing to allow for move only types
    static_vector<T, 10> ret;

    ret.emplace(std::end(ret), static_cast<char>(1));
    ret.emplace(std::end(ret), static_cast<char>(2));
    ret.emplace(std::end(ret), static_cast<char>(3));
    ret.emplace(std::end(ret), static_cast<char>(4));
    ret.emplace(std::end(ret), static_cast<char>(5));
    ret.emplace(std::end(ret), static_cast<char>(6));
    ret.emplace(std::end(ret), static_cast<char>(7));
    ret.emplace(std::end(ret), static_cast<char>(8));
    ret.emplace(std::end(ret), static_cast<char>(9));
    ret.emplace(std::end(ret), static_cast<char>(10));

    return std::move(ret);
}

template<typename T>
static_vector<T, 10> get_mostly_full_vector()
{
    // Do the emplacing to allow for move only types
    static_vector<T, 10> ret;

    ret.emplace(std::end(ret), static_cast<char>(1));
    ret.emplace(std::end(ret), static_cast<char>(2));
    ret.emplace(std::end(ret), static_cast<char>(3));
    ret.emplace(std::end(ret), static_cast<char>(4));
    ret.emplace(std::end(ret), static_cast<char>(5));
    ret.emplace(std::end(ret), static_cast<char>(6));
    ret.emplace(std::end(ret), static_cast<char>(7));
    ret.emplace(std::end(ret), static_cast<char>(8));
    ret.emplace(std::end(ret), static_cast<char>(9));

    return std::move(ret);
}

template<typename T>
static_vector<T, 10> get_empty_vector()
{
    return {};
}

template<typename V, typename I, typename F, size_t N>
void insert_single_test(V const& verify, int index, char data, I const& insert,
        F const& get_initial_vector, std::array<char, N> const& final_status)
{
    auto v = get_initial_vector();
    auto initial_size = v.size();
    try
    {
        insert(v, v.begin() + index, data);

        ASSERT_MESSAGE(initial_size + 1 <= 10 && "Should have thrown before getting here", v, initial_size);
    }
    catch (...)
    {
        ASSERT_MESSAGE(initial_size + 1 > 10, v, initial_size);
    }

    ASSERT_MESSAGE(
            std::equal(v.begin(), v.end(), final_status.begin(), final_status.end()),
            v, final_status);
    ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify), v);
}

template<typename T, typename V, typename I>
void insert_single_tests(V const& verify_func, I const& insert, bool only_back = false)
{
    if (!only_back)
    {
        insert_single_test(verify_func, 0, 100, insert, get_empty_vector<T>,
                make_c_array(100));
        insert_single_test(verify_func, 0, 100, insert, get_123_vector<T>,
                make_c_array( 100, 1, 2, 3 ));
        insert_single_test(verify_func, 1, 100, insert, get_123_vector<T>,
                make_c_array( 1, 100, 2, 3 ));
        insert_single_test(verify_func, 2, 100, insert, get_123_vector<T>,
                make_c_array( 1, 2, 100, 3 ));


        insert_single_test(verify_func, 0, 100, insert, get_full_vector<T>,
                make_c_array( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ));
        insert_single_test(verify_func, 1, 100, insert, get_full_vector<T>,
                make_c_array( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ));
        insert_single_test(verify_func, 2, 100, insert, get_full_vector<T>,
                make_c_array( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ));
    }

    insert_single_test(verify_func, 3, 100, insert, get_123_vector<T>,
            make_c_array( 1, 2, 3, 100 ));

    insert_single_test(verify_func, 10, 100, insert, get_full_vector<T>,
            make_c_array( 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ));
}

template<typename V, typename F, size_t N1, size_t N2>
void insert_range_test(V const& verify, int index, std::array<char, N1> data,
        F const& get_initial_vector, std::array<char, N2> const& final_status)
{
    auto v = get_initial_vector();
    auto initial_size = v.size();

    try
    {
        v.insert(v.begin() + index, data.begin(), data.end());
        ASSERT_MESSAGE(initial_size + N1 <= 10 && "Should have thrown before getting here", v, initial_size, N1);
    }
    catch (...)
    {
        ASSERT_MESSAGE(initial_size + N1 > 10, v, N1, initial_size);
    }

    ASSERT_MESSAGE(
            std::equal(v.begin(), v.end(), final_status.begin(), final_status.end()),
            v, final_status);
    ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify), v);
}


template<typename T, typename F>
void n_default_test(F const& verify_func, int n)
{
    // "N copy of X" ctor
    static_vector<T, 10> v(n);
    ASSERT_EQUAL(v.size(), n);
    for (auto const& x : v)
        ASSERT_EQUAL(x, 0);
    ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
}

template<typename T, typename F>
void n_copy_test(F const& verify_func, int n)
{
    // "N copy of X" ctor
    static_vector<T, 10> v(n, T(static_cast<char>(100)));
    ASSERT_EQUAL(v.size(), n);
    for (auto const& x : v)
        ASSERT_EQUAL(x, 100);
    ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
}

template<typename T, typename F, typename... L>
void initializer_list_test(F const& verify_func, L const&... l)
{
    try
    {
        // Initializer list constructor
        static_vector<T, 10> v{
            T(static_cast<char>(l))...
        };

        ASSERT(sizeof...(l) <= 10 && "Should have thrown because size to large");

        auto arry = make_c_array(l...);
        ASSERT_MESSAGE(
                std::equal(v.begin(), v.end(), arry.begin(), arry.end()), v, arry);
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }
    catch (...)
    {
        ASSERT(sizeof...(l) > 10 && "Exception should not have been thrown");
    }
}

template<typename T, typename F, typename... L>
void iterator_const_test(F const& verify_func, L const&... l)
{
    try
    {
        auto arry = make_c_array(l...);

        // Note that this works even for the move only type, because the
        // original array is not of the same type as the value_type of the
        // vector, so it is actually newly constructed objects being inserted
        static_vector<T, 10> v{
            std::begin(arry), std:end(arry)
        };

        ASSERT_MESSAGE(sizeof...(l) <= 10 && "Should have thrown because size to large", v);

        ASSERT_MESSAGE(
                std::equal(v.begin(), v.end(), arry.begin(), arry.end()), v, arry);
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }
    catch (...)
    {
        ASSERT(sizeof...(l) > 10 && "Exception should not have been thrown");
    }
}


// If the type is copyable
template<typename T, typename F>
void copyable_tests(std::true_type, F const& verify_func)
{
    n_default_test<T>(verify_func, 0);
    n_default_test<T>(verify_func, 3);
    n_default_test<T>(verify_func, 10);
    {
        try
        {
            // "N default" ctor
            static_vector<T, 10> v(11);
            ASSERT_MESSAGE(false, v);
        }
        catch (...)
        {
        }
    }

    n_copy_test<T>(verify_func, 0);
    n_copy_test<T>(verify_func, 3);
    n_copy_test<T>(verify_func, 10);
    {
        try
        {
            // "N copy of X" ctor
            static_vector<T, 10> v(11, T(static_cast<char>(100)));
            ASSERT_MESSAGE(false, v);
        }
        catch (...)
        {
        }
    }

    initializer_list_test<T>(verify_func);
    initializer_list_test<T>(verify_func, 1, 2, 3);
    initializer_list_test<T>(verify_func, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    initializer_list_test<T>(verify_func, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
    {
        // Iterator constructor
        char a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        static_vector<T, 10> v{std::begin(a), std::end(a)};
        ASSERT_EQUAL(v.size(), 10);
        int i = 1;
        for (auto x : v)
        {
            ASSERT_EQUAL(x, i);
            i++;
        }
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }
    {
        // Copy ctor
        static_vector<T, 10> u{
            T(static_cast<char>(1)),
            T(static_cast<char>(2)),
            T(static_cast<char>(3)),
            T(static_cast<char>(4)),
            T(static_cast<char>(5)),
            T(static_cast<char>(6)),
            T(static_cast<char>(7)),
            T(static_cast<char>(8)),
            T(static_cast<char>(9)),
            T(static_cast<char>(10))
        };
        static_vector<T, 10> v{u};
        ASSERT_EQUAL(v.size(), 10);
        int i = 1;
        for (auto x : v)
        {
            ASSERT_EQUAL(x, i);
            i++;
        }
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }
    {
        // Copy assignment
        static_vector<T, 10> u{
            T(static_cast<char>(1)),
            T(static_cast<char>(2)),
            T(static_cast<char>(3)),
            T(static_cast<char>(4)),
            T(static_cast<char>(5)),
            T(static_cast<char>(6)),
            T(static_cast<char>(7)),
            T(static_cast<char>(8)),
            T(static_cast<char>(9)),
            T(static_cast<char>(10))
        };
        static_vector<T, 10> v;
        v = u;
        ASSERT_EQUAL(v.size(), 10);
        int i = 1;
        for (auto x : v)
        {
            ASSERT_EQUAL(x, i);
            i++;
        }
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }

    insert_range_test(verify_func, 0, make_c_array(100, 101), get_empty_vector<T>,
            make_c_array( 100, 101 ));

    insert_range_test(verify_func, 0, make_c_array(100, 101), get_123_vector<T>,
            make_c_array( 100, 101, 1, 2, 3 ));
    insert_range_test(verify_func, 1, make_c_array(100, 101), get_123_vector<T>,
            make_c_array( 1, 100, 101, 2, 3 ));
    insert_range_test(verify_func, 2, make_c_array(100, 101), get_123_vector<T>,
            make_c_array( 1, 2, 100, 101, 3 ));
    insert_range_test(verify_func, 3, make_c_array(100, 101), get_123_vector<T>,
            make_c_array( 1, 2, 3, 100, 101 ));

    insert_range_test(verify_func, 0, make_c_array(100, 101), get_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9,10));
    insert_range_test(verify_func, 1, make_c_array(100, 101), get_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9,10));
    insert_range_test(verify_func, 2, make_c_array(100, 101), get_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9,10));
    insert_range_test(verify_func, 10, make_c_array(100, 101), get_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9,10));

    insert_range_test(verify_func, 0, make_c_array(100, 101), get_mostly_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9));
    insert_range_test(verify_func, 1, make_c_array(100, 101), get_mostly_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9));
    insert_range_test(verify_func, 2, make_c_array(100, 101), get_mostly_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9));
    insert_range_test(verify_func, 10, make_c_array(100, 101), get_mostly_full_vector<T>,
            make_c_array(1,2,3,4,5,6,7,8,9));
}

// If the type is move only
template<typename T, typename F>
void copyable_tests(std::false_type, F const& verify_func)
{
}


template<typename T, typename F, typename Copyable=std::is_copy_constructible<T>>
void generic_test(F const& verify_func)
{
    {
        // Default ctor; capacity
        static_vector<T, 10> v;
        ASSERT_EQUAL(v.capacity(), 10);
        ASSERT_EQUAL(v.size(), 0);
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }
    {
        // Move ctor
        static_vector<T, 10> u;
        u.emplace(std::end(u), T(static_cast<char>(1)));
        u.emplace(std::end(u), T(static_cast<char>(2)));
        u.emplace(std::end(u), T(static_cast<char>(3)));
        u.emplace(std::end(u), T(static_cast<char>(4)));
        u.emplace(std::end(u), T(static_cast<char>(5)));
        u.emplace(std::end(u), T(static_cast<char>(6)));
        u.emplace(std::end(u), T(static_cast<char>(7)));
        u.emplace(std::end(u), T(static_cast<char>(8)));
        u.emplace(std::end(u), T(static_cast<char>(9)));
        u.emplace(std::end(u), T(static_cast<char>(10)));

        static_vector<T, 10> v{ std::move(u) };
        ASSERT_EQUAL(v.size(), 10);
        int i = 1;
        for (auto const& x : v)
        {
            ASSERT_EQUAL(x, i);
            i++;
        }
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }
    {
        // Move assignment
        static_vector<T, 10> u;
        u.emplace(std::end(u), T(static_cast<char>(1)));
        u.emplace(std::end(u), T(static_cast<char>(2)));
        u.emplace(std::end(u), T(static_cast<char>(3)));
        u.emplace(std::end(u), T(static_cast<char>(4)));
        u.emplace(std::end(u), T(static_cast<char>(5)));
        u.emplace(std::end(u), T(static_cast<char>(6)));
        u.emplace(std::end(u), T(static_cast<char>(7)));
        u.emplace(std::end(u), T(static_cast<char>(8)));
        u.emplace(std::end(u), T(static_cast<char>(9)));
        u.emplace(std::end(u), T(static_cast<char>(10)));
        static_vector<T, 10> v;
        v = std::move(u);
        ASSERT_EQUAL(v.size(), 10);
        int i = 1;
        for (auto const& x : v)
        {
            ASSERT_EQUAL(x, i);
            i++;
        }
        ASSERT_MESSAGE(std::all_of(v.begin(), v.end(), verify_func), v);
    }
    iterator_const_test<T>(verify_func);
    iterator_const_test<T>(verify_func, 1,2,3,4,5,6,7,8,9,10);
    iterator_const_test<T>(verify_func, 1,2,3,4,5,6,7,8,9,10,11);
    iterator_const_test<T>(verify_func, 1,2,3,4,5,6,7,8,9,10,11,12);

    insert_single_tests<T>(verify_func, [](auto& v, auto it, char data){
        v.insert(it, data);
    });

    // This is not to test the total interface of emplace, just that it inserts
    // properly....the other aspects of emplace are going to be tested
    // elsewhere
    insert_single_tests<T>(verify_func, [](auto& v, auto it, char data){
        v.emplace(it, data);
    });

    insert_single_tests<T>(verify_func, [](auto& v, auto it, char data){
        v.emplace_back(data);
    }, true);


    insert_single_tests<T>(verify_func, [](auto& v, auto it, char data){
        v.push_back(data);
    }, true);

    copyable_tests<T>(Copyable{}, verify_func);
}

template<typename T>
void generic_test()
{
    return generic_test<T>([](auto const& x) { return true; });
}

template<typename E>
void emplace_test(E const& emplace)
{
    {
        // Emplace element
        static_vector<std::tuple<Movable, Copyable>, 10> v;
        const Copyable c(char(12));
        emplace(v, v.end(), Movable{char(42)}, c);
        ASSERT_EQUAL(v.size(), 1);

        ASSERT_EQUAL(std::get<0>(v[0]), 42);
        ASSERT_EQUAL(std::get<1>(v[0]), 12);

        ASSERT(std::get<0>(v[0]).verify());
        ASSERT(std::get<1>(v[0]).verify());
    }
    {
        static_vector<std::string, 10> v;
        emplace(v, v.end(), 10, 'a');
        ASSERT_EQUAL(v[0], "aaaaaaaaaa");
    }
}

int main(int, char* []) {
    //
    try {
        generic_test<char>();
        generic_test<Copyable>([](auto const& x){ return x.verify(); });
        generic_test<Movable>([](auto const& x){ return x.verify(); });
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
        emplace_test([](auto& v, auto it, auto&&... data){
            v.emplace(it, std::forward<decltype(data)>(data)...);
        });
        emplace_test([](auto& v, auto it, auto&&... data){
            v.emplace_back(std::forward<decltype(data)>(data)...);
        });
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

