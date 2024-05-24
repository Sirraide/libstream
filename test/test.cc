#include <stream/stream.hh>
#include <functional>

using namespace streams;
using namespace std::literals;

#define CAT(a, b) CAT_(a, b)
#define CAT_(a, b) a##b

#define STR(x) STR_(x)
#define STR_(x) #x

#define Check(cond) \
    if (not(cond)) return "Failure on line " STR(__LINE__) ""sv;

#if __cpp_static_assert >= 202306L
#define Test(...) constexpr Result CAT(r_, __LINE__) = [] consteval -> Result { __VA_ARGS__; return {}; } ();\
    static_assert(CAT(r_, __LINE__), CAT(r_, __LINE__))
#else
#define Test(...) constexpr Result CAT(r_, __LINE__) = [] consteval -> Result { __VA_ARGS__; return {}; } ();\
    static_assert(CAT(r_, __LINE__), "Error at " STR(__LINE__))
#endif

struct Result {
    bool failed = false;
    std::string_view error_message;

    constexpr Result() = default;
    constexpr Result(std::string_view error_message) : failed(true), error_message(error_message) {}

    constexpr auto data() const -> const char* { return error_message.data(); }
    constexpr auto size() const -> std::size_t { return error_message.size(); }

    explicit constexpr operator bool() const { return not failed; }
};

template <typename stream>
auto Apply(stream&& s, auto cb, auto&&... args) -> stream& {
    std::invoke(cb, s, std::forward<decltype(args)>(args)...);
    return s;
}

constexpr stream empty = ""sv;
constexpr stream one = "a"sv;
constexpr stream word = "hello"sv;
constexpr stream words = "hello world foo bar baz"sv;
constexpr stream whitespace = " \v\f\t\r\n"sv;
constexpr stream lt_spaces = "  hello world        "sv;
constexpr stream multiline = "hello\nworld\n\nfoo\nbar\nbaz\n"sv;

static_assert(empty.back() == std::nullopt);
static_assert(one.back() == 'a');
static_assert(word.back() == 'o');

static_assert(empty.char_ptr() == empty.text().data());
static_assert(one.char_ptr() == one.text().data());
static_assert(word.char_ptr() == word.text().data());

Test(
    stream w{word};
    Check(not stream{empty}.consume('a'));
    Check(stream{one}.consume('a'));
    Check(w.consume('h'));
    Check(w.consume('e'));
    Check(w.consume('l'));
    Check(w.consume('l'));
    Check(w.consume('o'));
    Check(not w.consume('o'));
);

static_assert(empty.data() == empty.text().data());
static_assert(one.data() == one.text().data());
static_assert(word.data() == word.text().data());

Test(
    stream w{word};
    w.drop();
    Check(w == "ello");
    w.drop(2);
    Check(w == "lo");
    w.drop(1231345);
    Check(w.empty());
);

static_assert(stream{word}.drop_until('l') == "llo");
static_assert(stream{word}.drop_until('x').empty());
static_assert(stream{word}.drop_until("lo") == "lo");
static_assert(stream{word}.drop_until([](char c) { return c == 'l'; }) == "llo");

static_assert(stream{word}.drop_until_any("le") == "ello");

static_assert(stream{word}.drop_until_or_empty('x') == "hello");
static_assert(stream{word}.drop_until_or_empty("xqz") == "hello");
static_assert(stream{word}.drop_until_any_or_empty("xqlz") == "llo");
static_assert(stream{word}.drop_until_any_or_empty("x") == "hello");

Test(
    auto lines = stream{multiline}.lines();
    auto it = lines.begin();
    Check(*it++ == "hello");
    Check(*it++ == "world");
    Check(*it++ == "");
    Check(*it++ == "foo");
    Check(*it++ == "bar");
    Check(*it++ == "baz");
    Check(*it++ == "");
    Check(it == lines.end());
);







