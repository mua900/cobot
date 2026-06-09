#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <array>

#define BIT(x) ((uint64_t)1 << (x))

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

constexpr s16 MAX_SIGNED_16_BIT = s16(0x7FFF);
constexpr s32 MAX_SIGNED_32_BIT = s32(0x7FFFFFFF);
constexpr s64 MAX_SIGNED_64_BIT = s64(0x7FFFFFFFFFFFFFFF);

constexpr int MAX_INTEGER = int(-1) ^ (1 << (sizeof(int) * 8 - 1));

unsigned int pop_count(u64 x);

#ifdef _MSC_VER

#include <intrin.h>

#define NORETURN __declspec(noreturn)

static inline unsigned int msvc_trailing_zeros(u64 x)
{
    unsigned long pos = 0;
    unsigned char is_zero = _BitScanForward64(&pos, x);
    // @note no checking for zero here since we assume non-zero input.
    return pos;
}

static inline unsigned int msvc_leading_zeros(u64 x)
{
    unsigned long pos = 0;
    unsigned char is_zero = _BitScanReverse64(&pos, x);
    // @note no checking for zero here since we assume non-zero input.
    return pos;
}

#define POP_COUNT(x)      pop_count(x)
#define LEADING_ZEROS(x)  msvc_leading_zeros(x)
#define TRAILING_ZEROS(x) msvc_trailing_zeros(x)

#else // _MSC_VER

#define NORETURN __attribute__((noreturn))

#define POP_COUNT(x)      __builtin_popcountll(x)
#define LEADING_ZEROS(x)  __builtin_clzll(x)
#define TRAILING_ZEROS(x) __builtin_ctzll(x)

#endif

#define ASSERT(x)   do {    \
        if (!(x)) {             \
            fprintf(stderr, "-----*****----- Assertion failed at %s:%d   %s\n", __FILE__, __LINE__, #x); \
            exit(1);    \
        }               \
    } while(0)


NORETURN
void panic(char const* const msg);

#define NOT_IMPLEMENTED(x) panic(x " not implemeneted");

int pop_lsb(u64* x);
int pop_msb(u64* x);

int lsb_index(u64 x);
int msb_index(u64 x);

struct BinaryData {
	u8* data = nullptr;
	size_t size = 0;

    ~BinaryData() {
        if (data) {
            free(data);

            data = nullptr;
            size = 0;
        }
    }

	void release() {
		if (data) {
			free(data);
			data = nullptr;
		}
	}
};

struct Find_Result {
	int index = 0;
	bool found = false;

    Find_Result(int index, bool found) : index(index), found(found) {}
    Find_Result() {}
};

int string_length(const char* cstr);

struct String {
    const char* data = NULL;
    int size = 0;

    String () {}
	explicit String (const char* d) : data(d), size(string_length(d)) {}
    String (const char* d, int s) : data(d), size(s) {}
	String (const BinaryData& b) : data((const char*)b.data), size(b.size) {}

    bool advance(int amount);

    char operator[](int index) const { return data[index]; }
    bool operator==(const String& other) const;
	void trim();
};

#define STRING_EMPTY ((String){.data=NULL,.size=0})
#define CSTRING_LENGTH(s) (sizeof(s)-1)

#define SCOPE_STRING_EXP(p_s, p_name, p_size)				\
	char p_name[p_size];  \
	memcpy(p_name, p_s.data, p_s.size);			\
	p_name[p_s.size] = '\0';

#define SCOPE_STRING(str, name) SCOPE_STRING_EXP(str, name, 256)

String make_string(const char* s);
bool string_compare(String s1, String s2);
bool string_starts_with(String s, String start);
int string_match_start(String s1, String s2);
int string_match_character(const String s, int offset, char c);
int string_find_character(String s, int offset, char c);
String string_slice(String s, int start, int end);
String string_slice_to_character(String s, int start, char c);
String string_get_extension(String s);  // the extension
String string_get_file_name(String s);  // the string except the extension
u64 string_hash(String s);

int string_to_integer(String s, bool* success);
double string_to_real(String s, bool* success);

struct MutableString {
    char* data = nullptr;
    int size = 0;
    int cap = 0;

    void clear_values() { data = nullptr; size = 0; cap = 0; }

    MutableString() {
        create(128);
    }

    MutableString(int init_cap) {
        create(init_cap);
    }

    MutableString(String s) {
        create(s.size);
        ASSERT(data);
        memcpy(data, s.data, s.size * sizeof(char));
        size = s.size;
    }

    MutableString(MutableString& other) = delete;
    void operator=(MutableString& other) = delete;
    MutableString(MutableString&& other) noexcept {
        data = other.data;
        size = other.size;
        cap = other.cap;
        other.clear_values();
    }
    void operator=(MutableString&& other) noexcept {
        data = other.data;
        size = other.size;
        cap = other.cap;
        other.clear_values();
    }

    ~MutableString()
    {
        delete[] data;
    }

    void create(int init_cap)
    {
        data = new char[init_cap];
        cap = init_cap;
    }

    void set_str(const char* str)
    {
        int len = (int) strlen(str);
        ensure_size(len);
        memcpy(data, str, len * sizeof(char));
        size = len;
    }

    void set_string(String string)
    {
        ensure_size(string.size);
        memcpy(data, string.data, string.size * sizeof(char));
        size = string.size;
    }

    String to_string()
    {
        return String(data, size);
    }

	void ensure_size(int capacity) {
		if (cap < capacity) {
			resize(capacity);
		}
	}

	void resize(int ncap) {
		char* new_buffer = new char[ncap];

		int new_size = (ncap < size) ? ncap : size;

		for (int i = 0; i < new_size; i++) {
			new_buffer[i] = data[i];
		}

		delete[] data;

		data = new_buffer;
		cap = ncap;
		size = new_size;
	}
};

struct StringReference {
    s64 offset = 0;
    s64 length = 0;

    StringReference() {}
    StringReference(s64 off, s64 len) : offset(off), length(len) {}
};

struct String_Builder {
    char* buffer = nullptr;
    int buffer_capacity = 0;
    int cursor = 0;

    String_Builder() {
        create(128);
    }

    String_Builder(int initial_capacity);

    ~String_Builder() {
        if (buffer) {
            free(buffer);
            buffer = nullptr;
        }
    }

    String_Builder(const String_Builder& other) {
        free_buffer();

        ensure_size(other.buffer_capacity);
        ASSERT(buffer && other.buffer);
        memcpy(buffer, other.buffer, other.cursor);
        cursor = other.cursor;
    }

    void operator=(const String_Builder& other) {
        free_buffer();

        ensure_size(other.buffer_capacity);
        ASSERT(buffer && other.buffer);
        memcpy(buffer, other.buffer, other.cursor);
        cursor = other.cursor;
    }

    String_Builder(String_Builder&& other) noexcept {
        free_buffer();

        buffer = other.buffer;
        cursor = other.cursor;
        buffer_capacity = other.buffer_capacity;
        other.clear_values();
    }

    void operator=(String_Builder&& other) noexcept {
        free_buffer();

        buffer = other.buffer;
        cursor = other.cursor;
        buffer_capacity = other.buffer_capacity;
        other.clear_values();
    }

    const char* get_end() const { return buffer + cursor; }
    void create(int initial_capacity);
    int append(String string);
    int append_path(String string);  // expect / as the separator and replace it with \\ on windows
    int append_char(char ch);
    int append_integer(int n);
    int append_hex(int n);
	int append_float(float n);
    String put_string(String s);
    String put_path(String path);
    const char* c_string();
    bool ends_with(String s) const;
    void remove(int amount);  // remove the last n characters from the buffer
    void remove_slice(int start, int end);
    int clear_and_append(String s);
    int clear_and_append_float(float n);
    int append_many(String* strings, int n);
    void free_buffer();
    void clear();
    String to_string();
    String slice(int start, int length) const;
    String get_string(StringReference ref) const { return slice(ref.offset, ref.length); }
    StringReference get_reference(String s) const {
        s64 offset = ptrdiff_t(s.data - buffer);
        if (offset > 100000 || offset < 0) {
            panic("Probably wrong pointers");
        }
        return StringReference{ offset, s.size };
    }
    int ensure_size(int size);
private:
    void resize();
    void clear_values() { buffer = nullptr; buffer_capacity = 0; cursor = 0; }
};

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define MIN(x,y) (((x) > (y)) ? (y) : (x))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define CLAMP(x, lower, upper) (MIN(upper, MAX(x, lower)))

bool load_file(const char* filepath, BinaryData& bdata);
bool load_file_text(const char* filepath, String_Builder& s);

long get_file_size(FILE* file);

struct File {
	FILE* handle = nullptr;

    File(FILE* handle) : handle(handle) {}
	File(String filepath, const char* access) {
		SCOPE_STRING(filepath, buffer);

		handle = fopen(buffer, access);
	}
	~File() {
		fclose(handle);
	}

	void write_string(String s);
	void write_number(double n);
	void write_integer(u64 n);

	String read_string();
	double read_number();
	u64 read_integer();
};

#ifdef _WIN32
    static const String PathSeparator = make_string("\\");
    static const String NewLine = make_string("\r\n");
#else
    static const String PathSeparator = make_string("/");
    static const String NewLine = make_string("\n");
#endif

static inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static inline bool is_alpha_lower(char c)
{
    return c >= 'a' && c <= 'z';
}

static inline bool is_alpha_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static inline bool is_alpha(char c)
{
    return is_alpha_lower(c) || is_alpha_upper(c);
}

static inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline char to_lower_ascii(char c)
{
    return is_alpha_upper(c) ? (c - 'A' + 'a') : (c);
}

static inline char to_upper_ascii(char c)
{
    return is_alpha_lower(c) ? (c - 'a' + 'A') : (c);
}

#define BOOL_STRING(b) ((b) ? ("true") : ("false"))
