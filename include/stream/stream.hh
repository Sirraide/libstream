#ifndef STREAM_STREAM_HH
#define STREAM_STREAM_HH

#include <cstddef>
#include <format>
#include <optional>
#include <ranges>
#include <source_location>
#include <string_view>
#include <utility>

namespace streams {

#ifndef LIBSTREAM_ASSERTIONS
#    define LIBSTREAM_ASSERTIONS 0
#endif

#if defined(_GLIBCXX_DEBUG) || defined(_LIBCPP_DEBUG) || LIBSTREAM_ASSERTIONS
#    undef LIBSTREAM_ASSERTIONS
#    define LIBSTREAM_ASSERTIONS   1
#    define LIBSTREAM_ASSERT(cond) (cond ? (void) 0 : _m_assert_fail(#cond))
#else
#    define LIBSTREAM_ASSERTIONS  0
#    define LIBSTREAM_ASSERT(...) void()
#endif

#define LIBSTREAM_STRING_LITERAL(lit) [] {                  \
    if constexpr (std::is_same_v<char_type, char>)          \
        return lit;                                         \
    else if constexpr (std::is_same_v<char_type, wchar_t>)  \
        return L"" lit;                                     \
    else if constexpr (std::is_same_v<char_type, char8_t>)  \
        return u8"" lit;                                    \
    else if constexpr (std::is_same_v<char_type, char16_t>) \
        return u"" lit;                                     \
    else if constexpr (std::is_same_v<char_type, char32_t>) \
        return U"" lit;                                     \
    else                                                    \
        static_assert(false, "Unsupported character type"); \
}()

/// \brief A stream of characters.
///
/// This is a non-owning wrapper around a blob of text intended for simple
/// parsing tasks. Streams are cheap to copy and should be passed by value.
template <typename CharType>
class basic_stream {
public:
    using char_type = CharType;
    using char_opt = std::optional<char_type>;
    using text_type = std::basic_string_view<char_type>;
    using size_type = std::size_t;
    using string_type = std::basic_string<char_type>;

private:
    text_type _m_text;

    static consteval auto _s_default_line_separator() -> text_type {
#ifndef _WIN32
        return LIBSTREAM_STRING_LITERAL("\n");
#else
        return LIBSTREAM_STRING_LITERAL("\r\n");
#endif
    }

public:
    /// Construct an empty stream.
    constexpr basic_stream() = default;

    /// Construct a new stream from text.
    constexpr basic_stream(text_type text) noexcept : _m_text(text) {}

    /// Construct a new stream from text.
    constexpr basic_stream(const string_type& text) noexcept : _m_text(text) {}

    // Prevent construction from temporary.
    constexpr basic_stream(const string_type&& text) = delete;

    /// Construct a new stream from characters.
    constexpr basic_stream(const char_type* chars, size_type size) noexcept
        : _m_text(chars, size) {}

    /// \return The last character of the stream, or an empty optional
    ///         if the stream is empty.
    [[nodiscard]] constexpr auto
    back() const noexcept -> char_opt {
        if (empty()) return std::nullopt;
        return _m_text.back();
    }

    /// \return The data pointer as a \c const \c char*.
    [[nodiscard]] constexpr auto
    char_ptr() const noexcept -> const char* {
        // Hack to avoid reinterpret_cast at compile time.
        if constexpr (std::same_as<char_type, char>) return _m_text.data();
        else return reinterpret_cast<const char*>(_m_text.data());
    }

    /// Skip a character.
    ///
    /// If the first character of the stream is the given character,
    /// remove it from the stream. Otherwise, do nothing.
    ///
    /// \param c The character to skip.
    /// \return True if the character was skipped.
    [[nodiscard]] constexpr auto
    consume(char_type c) noexcept -> bool {
        if (not starts_with(c)) return false;
        drop();
        return true;
    }

    /// \return The data pointer.
    [[nodiscard]] constexpr auto
    data() const noexcept -> const char_type* {
        return _m_text.data();
    }

    /// Discard N characters from the stream.
    ///
    /// If the stream contains fewer than N characters, this clears
    /// the entire stream.
    ///
    /// To skip a certain character if it is present, use \c consume().
    ///
    /// \see consume(char_type)
    ///
    /// \param n The number of characters to drop.
    /// \return This.
    template <std::integral size = size_type>
    requires (not std::same_as<std::remove_cvref_t<size>, char_type>)
    constexpr auto
    drop(size n = 1) noexcept -> basic_stream& {
        if constexpr (not std::unsigned_integral<size>) LIBSTREAM_ASSERT(
            n >= 0,
            "Cannot drop a negative number of characters"
        );

        (void) take(static_cast<size_type>(n));
        return *this;
    }

    ///@{
    /// \brief Drop characters from the stream conditionally.
    ///
    /// Same as \c take_until() and friends, but discards the
    /// characters instead and returns the stream.
    ///
    /// \see take_until(char_type)
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return This
    constexpr auto
    drop_until(char_type c) noexcept -> basic_stream& {
        (void) take_until(c);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until(text_type s) noexcept -> basic_stream& {
        (void) take_until(s);
        return *this;
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_until(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_until(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_any(text_type chars) noexcept -> basic_stream& {
        (void) take_until_any(chars);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_any_or_empty(text_type chars) noexcept -> basic_stream& {
        (void) take_until_any_or_empty(chars);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_or_empty(char_type c) noexcept -> basic_stream& {
        (void) take_until_or_empty(c);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_until_or_empty(text_type s) noexcept -> basic_stream& {
        (void) take_until_or_empty(s);
        return *this;
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_until_or_empty(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_until_or_empty(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while(char_type c) noexcept -> basic_stream& {
        (void) take_while(c);
        return *this;
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_while(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_while(std::move(c));
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while_any(text_type chars) noexcept -> basic_stream& {
        (void) take_while_any(chars);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while_any_or_empty(text_type chars) noexcept -> basic_stream& {
        (void) take_while_any_or_empty(chars);
        return *this;
    }

    /// \see take_until(char_type)
    constexpr auto
    drop_while_or_empty(char_type c) noexcept -> basic_stream& {
        (void) take_while_or_empty(c);
        return *this;
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    constexpr auto
    drop_while_or_empty(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> basic_stream& {
        (void) take_while_or_empty(std::move(c));
        return *this;
    }
    ///@}

    /// Check if this stream is empty.
    [[nodiscard]] constexpr auto
    empty() const noexcept -> bool {
        return _m_text.empty();
    }

    ///@{
    /// \return True if the stream ends with the given character(s).
    [[nodiscard]] constexpr auto
    ends_with(char_type suffix) const noexcept -> bool {
        return not _m_text.empty() and _m_text.back() == suffix;
    }

    /// \see ends_with(char_type) const
    [[nodiscard]] constexpr auto
    ends_with(text_type suffix) const noexcept -> bool {
        return _m_text.ends_with(suffix);
    }
    ///@}

    /// \return True if the stream ends with any of the given characters.
    [[nodiscard]] constexpr auto
    ends_with_any(std::initializer_list<char_type> suffix) const -> bool {
        return not _m_text.empty() and
               std::ranges::find(suffix, _m_text.back()) != suffix.end();
    }

    /// Extract a series of characters from a stream.
    template <std::same_as<CharType> ...Chars>
    [[nodiscard]] constexpr auto
    extract(Chars& ...chars) noexcept -> bool {
        if (not has(sizeof...(Chars))) return false;
        auto extracted = take(sizeof...(Chars));
        auto it = extracted.begin();
        ((chars = *it++), ...);
        return true;
    }

    /// \return The first character of the stream, or an empty optional
    ///         if the stream is empty.
    [[nodiscard]] constexpr auto
    front() const noexcept -> char_opt {
        if (empty()) return std::nullopt;
        return _m_text.front();
    }

    /// \return Whether this stream contains at least N characters.
    [[nodiscard]] constexpr auto
    has(size_type n) const noexcept -> bool {
        return size() >= n;
    }

    /// Iterate over all lines in the stream.
    ///
    /// Returns a range that yields each line in the stream; the line
    /// separators are not included. The stream is not modified.
    [[nodiscard]] constexpr auto
    lines(text_type line_separator = _s_default_line_separator())
    const noexcept {
        return _m_text
            | std::views::split(line_separator)
            | std::views::transform([](auto r) { return basic_stream(text_type(r)); });
    }

    /// \return The size (= number of characters) of this stream.
    [[nodiscard]] constexpr auto
    size() const noexcept -> size_type {
        return _m_text.size();
    }

    /// \return The number of bytes in this stream.
    [[nodiscard]] constexpr auto
    size_bytes() const noexcept -> size_type {
        return size() * sizeof(char_type);
    }

    ///@{
    /// \return True if the stream starts with the given character(s).
    [[nodiscard]] constexpr auto
    starts_with(char_type prefix) const noexcept -> bool {
        return not _m_text.empty() and _m_text.front() == prefix;
    }

    /// \see starts_with(char_type) const
    [[nodiscard]] constexpr auto
    starts_with(text_type prefix) const noexcept -> bool {
        return _m_text.starts_with(prefix);
    }
    ///@}

    /// \return True if the stream starts with any of the given characters.
    [[nodiscard]] constexpr auto
    starts_with_any(text_type prefix) const -> bool {
        return not _m_text.empty() and
               std::ranges::find(prefix, _m_text.front()) != prefix.end();
    }

    /// Get N characters from the stream.
    ///
    /// If the stream contains fewer than N characters, this returns only
    /// the characters that are available. The stream is advanced by N
    /// characters.
    ///
    /// \param n The number of characters to get.
    /// \return The characters.
    [[nodiscard]] constexpr auto
    take(size_type n = 1) noexcept -> text_type {
        if (n > size()) n = size();
        return _m_advance(n);
    }

    /// @{
    /// Get a delimited character sequence from the stream.
    ///
    /// If the stream starts with a delimiter, the delimiter is removed
    /// and characters are read into a string view until a matching delimiter
    /// is found. Everything up to and including the matching delimiter is
    /// removed from the stream.
    ///
    /// If no opening or closing delimiter is found, the stream is not advanced
    /// at all and the \p string parameter is not written to.
    ///
    /// The \c _any overload instead looks for any one character in \p string to
    /// use as the delimiter. The closing delimiter must match the opening delimiter,
    /// however.
    ///
    /// \param string String view that will be set to the delimited character
    ///        range, excluding the delimiters.
    ///
    /// \param delimiter A string view to use as or containing the delimiters.
    ///
    /// \return True if a delimited sequence was found.
    [[nodiscard]] constexpr auto
    take_delimited(text_type& string, text_type delimiter) noexcept -> bool {
        if (not starts_with(delimiter)) return false;
        drop(delimiter.size());
        if (auto pos = _m_text.find(delimiter, delimiter.size()); pos != text_type::npos) {
            string = _m_text.substr(0, pos);
            _m_text.remove_prefix(pos + delimiter.size());
            return true;
        }
        return false;
    }

    /// \see take_delimited(text_type&, text_type)
    [[nodiscard]] constexpr auto
    take_delimited(char_type c, text_type& string) noexcept -> bool {
        return take_delimited(string, text_type{&c, 1});
    }

    /// \see take_delimited(text_type&, text_type)
    [[nodiscard]] constexpr auto
    take_delimited_any(text_type& string, text_type delimiters) noexcept -> bool {
        if (not starts_with_any(delimiters)) return false;
        char_type delim = take().front();
        if (auto pos = _m_text.find(delim); pos != text_type::npos) {
            string = _m_text.substr(0, pos);
            _m_text.remove_prefix(pos + 1);
            return true;
        }
        return false;
    }

    /// @}

    ///@{
    /// \brief Get characters from the stream conditionally.
    ///
    /// These take either
    ///
    ///     1. a single character,
    ///     2. a string view, or
    ///     3. a unary predicate.
    ///
    /// The stream is advanced until (in the case of the \c _until overloads)
    /// or while (in the case of the \c _while overloads) we find a character
    ///
    ///     1. equal to the given character, or
    ///     2. equal to the given string (or any of the characters in the
    ///        string for the \c _any overloads), or
    ///     3. for which the given predicate returns \c true.
    ///
    /// If a matching character is found, all characters skipped over this way are
    /// returned as text.
    ///
    /// If the condition is not \c true for any of the characters in the stream,
    /// the entire text is returned. The \c _or_empty overloads, return an empty
    /// string instead, and the stream is not advanced at all.
    ///
    /// \param c A character, \c string_view, or unary predicate.
    /// \return The matched characters.
    [[nodiscard]] constexpr auto
    take_until(char_type c) noexcept -> text_type {
        return _m_advance(std::min(_m_text.find(c), size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until(text_type s) noexcept -> text_type {
        return _m_advance(std::min(_m_text.find(s), size()));
    }

    /// \see take_until(char_type) const
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_until(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> text_type {
        return _m_take_until_cond<false>(std::move(c));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any(text_type chars) noexcept -> text_type {
        return _m_advance(std::min(_m_text.find_first_of(chars), size()));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_any_or_empty(text_type chars) noexcept -> text_type {
        auto pos = _m_text.find_first_of(chars);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_or_empty(char_type c) noexcept -> text_type {
        auto pos = _m_text.find(c);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_until_or_empty(text_type s) noexcept -> text_type {
        auto pos = _m_text.find(s);
        if (pos == text_type::npos) return {};
        return _m_advance(pos);
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_until_or_empty(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> text_type {
        return _m_take_until_cond<true>(std::move(c));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while(char_type c) noexcept -> text_type {
        return _m_take_while<false>(c);
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_while(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> text_type {
        return _m_take_while_cond<false>(std::move(c));
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while_any(text_type chars) noexcept -> text_type {
        return _m_take_while_any<false>(chars);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while_any_or_empty(text_type chars) noexcept -> text_type {
        return _m_take_while_any<true>(chars);
    }

    /// \see take_until(char_type)
    [[nodiscard]] constexpr auto
    take_while_or_empty(char_type c) noexcept -> text_type {
        return _m_take_while<true>(c);
    }

    /// \see take_until(char_type)
    template <typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    take_while_or_empty(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> text_type {
        return _m_take_while_cond<true>(std::move(c));
    }
    ///@}

    /// Get the underlying text for this stream.
    [[nodiscard]] constexpr auto
    text() const noexcept -> text_type { return _m_text; }

    ///@{
    /// \brief Remove characters from either end of the stream.
    ///
    /// Characters are deleted so long as the last/first character
    /// matches any of the given characters.
    ///
    /// \param cs The characters to remove.
    /// \return A reference to this stream.
    constexpr auto
    trim(text_type chars = whitespace()) noexcept -> basic_stream& {
        return trim_front(chars).trim_back(chars);
    }

    /// \see trim(text_type)
    constexpr auto
    trim_front(text_type chars = whitespace()) noexcept -> basic_stream& {
        auto pos = _m_text.find_first_not_of(chars);
        if (pos == text_type::npos) _m_text = {};
        else _m_text.remove_prefix(pos);
        return *this;
    }

    /// \see trim(text_type)
    constexpr auto
    trim_back(text_type chars = whitespace()) noexcept -> basic_stream& {
        auto pos = _m_text.find_last_not_of(chars);
        if (pos == text_type::npos) _m_text = {};
        else _m_text.remove_suffix(size() - pos - 1);
        return *this;
    }

    ///@}

    /// \return A string view containing ASCII whitespace characters
    /// as appropriate for the character type of this stream.
    [[nodiscard]] static consteval auto whitespace() -> text_type {
        return LIBSTREAM_STRING_LITERAL(" \t\n\r\v\f");
    }

    /// Get a character from the stream.
    ///
    /// \param index The index of the character to get.
    /// \return The character at the given index.
    [[nodiscard]] constexpr auto
    operator[](size_type index) const noexcept -> char_opt {
        if (index >= size()) [[unlikely]]
            return std::nullopt;
        return _m_text.at(index);
    }

    /// Get a slice of characters from the stream.
    ///
    /// The \p start and \p end parameters are clamped between
    /// 0 and the length of the stream.
    ///
    /// \param start The index of the first character to get.
    /// \param end The index of the last character to get.
    /// \return The characters in the given range.
    [[nodiscard]] constexpr auto
    operator[](size_type start, size_type end) const noexcept -> basic_stream {
        if (start > size()) [[unlikely]]
            start = size();
        if (end > size()) end = size();
        return basic_stream{_m_text.substr(start, end - start)};
    }

    /// @{
    /// Compare the contents of this stream.
    template <std::convertible_to<text_type> Ty>
    [[nodiscard]] constexpr auto
    operator<=>(Ty other) const noexcept {
        return _m_text <=> other;
    }

    template <std::convertible_to<text_type> Ty>
    [[nodiscard]] constexpr auto
    operator==(Ty other) const noexcept {
        return _m_text == other;
    }
    /// @}

private:
#if LIBSTREAM_ASSERTIONS
    auto _m_assert_fail(
        std::string_view msg,
        std::source_location where = std::source_location::current()
    ) const noexcept {
        std::puts(std::format(
            "{}:{}:{}",
            where.file_name(),
            where.line(),
            where.column()
        ).data());
        std::puts(std::format("libstream: Assertion failed: {}", msg).data());
        std::abort();
    }
#endif

    // Return characters until position `n` (exclusive)
    // and remove them from the stream.
    constexpr auto _m_advance(size_type n) noexcept -> text_type {
        LIBSTREAM_ASSERT(n <= size());
        auto txt = _m_text.substr(0, n);
        _m_text.remove_prefix(n);
        return txt;
    }

    template <bool _or_empty, typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    _m_take_until_cond(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> text_type {
        std::size_t i = 1;
        for (; i <= size(); ++i)
            if (c(_m_text[i - 1]))
                break;

        if (i > size()) {
            if constexpr (_or_empty) return {};
            else return _m_advance(size());
        }

        return _m_advance(i - 1);
    }

    template <bool _or_empty>
    [[nodiscard]] constexpr auto
    _m_take_while(char_type c) noexcept -> text_type {
        return _m_take_while_cond<_or_empty>(
            [c](char_type ch) { return ch == c; }
        );
    }

    template <bool _or_empty>
    [[nodiscard]] constexpr auto
    _m_take_while_any(text_type chars) noexcept -> text_type {
        return _m_take_while_cond<_or_empty>([chars](char_type ch) {
            return chars.find(ch) != text_type::npos;
        });
    }

    template <bool _or_empty, typename UnaryPredicate>
    requires requires (UnaryPredicate c) { c(char_type{}); }
    [[nodiscard]] constexpr auto
    _m_take_while_cond(UnaryPredicate c)
    noexcept(noexcept(c(char_type{}))) -> text_type {
        std::size_t i = 1;
        for (; i <= size(); ++i)
            if (not c(_m_text[i - 1]))
                break;

        if (i > size()) {
            if constexpr (_or_empty) return {};
            else return _m_advance(size());
        }

        return _m_advance(i - 1);
    }
};

using stream = basic_stream<char>;
using wstream = basic_stream<wchar_t>;
using u8stream = basic_stream<char8_t>;
using u16stream = basic_stream<char16_t>;
using u32stream = basic_stream<char32_t>;

#undef LIBSTREAM_ASSERT

} // namespace streams

#endif // STREAM_STREAM_HH
