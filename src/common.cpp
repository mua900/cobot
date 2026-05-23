#include "common.hpp"

#include <cmath>
#include <array>


unsigned int pop_count(u64 x)
{
    x = (x & (u64)0x5555555555555555) + ((x >> 1)  & (u64)0x5555555555555555);
    x = (x & (u64)0x3333333333333333) + ((x >> 2)  & (u64)0x3333333333333333);
    x = (x & (u64)0x0F0F0F0F0F0F0F0F) + ((x >> 4)  & (u64)0x0F0F0F0F0F0F0F0F);
    x = (x & (u64)0x00FF00FF00FF00FF) + ((x >> 8)  & (u64)0x00FF00FF00FF00FF);
    x = (x & (u64)0x0000FFFF0000FFFF) + ((x >> 16) & (u64)0x0000FFFF0000FFFF);
    x = (x & (u64)0x00000000FFFFFFFF) + ((x >> 32) & (u64)0x00000000FFFFFFFF);
    return unsigned int(x);
}

NORETURN
void panic(char const* const msg)
{
    fprintf(stderr, "[PANIC]: %s\n", msg);
    exit(1);
}

int pop_lsb(u64* x) {
    int index = TRAILING_ZEROS(*x);
    *x &= *x - 1;
    return index;
}

int pop_msb(u64* x) {
    int index = LEADING_ZEROS(*x);
    *x &= ~BIT(index);
    return index;
}

int lsb_index(u64 x) {
    return TRAILING_ZEROS(x);
}

int msb_index(u64 x) {
    return 63 - LEADING_ZEROS(x);
}

int string_length(const char* cstr) {
    return (int)strlen(cstr);
}

String make_string(const char* s)
{
    int len = (int)strlen(s);
    return { s, len };
}

bool string_compare(String s1, String s2)
{
    if (s1.size != s2.size) return false;
    for (int i = 0; i < s1.size; i++)
    {
        if (s1.data[i] != s2.data[i]) return false;
    }
    return true;
}

String string_slice(String s, int start, int end)
{
    return String { s.data + start, end - start };
}

String string_slice_to_character(String s, int start, char c) {
    int cursor = start;
    while (cursor < s.size && s.data[cursor] != c) {
        cursor += 1;
    }

    return String(s.data + start, cursor - start);
}

int string_match_start(String s1, String s2)
{
    int cursor = 0;
    while (cursor < s1.size && cursor < s2.size)
    {
        if (s1.data[cursor] != s2.data[cursor])
            break;
        cursor += 1;
    }

    return cursor;
}

bool string_starts_with(String s, String start)
{
    return string_match_start(s, start) == start.size;
}

int string_match_character(const String s, int offset, char c)
{
    if (offset >= s.size)
    {
        return 0;
    }

    int count = 0;
    while (s.size - (offset + count) > 0 && s.data[offset + count] == c)
    {
        count += 1;
    }

    return count;
}

int string_find_character(String s, int offset, char c)
{
    if (offset >= s.size)
        return -1;

    int cursor = 0;
    while (s.size - (offset + cursor) > 0 && s.data[offset + cursor] != c)
    {
        cursor += 1;
    }

    return (s.data[offset + cursor] == c) ? (offset + cursor) : -1;
}

String string_get_extension(String s)
{
    for (int i = s.size - 1; i >= 0; i--)
    {
        if (s.data[i] == '.')
        {
            // without the '.'
            return string_slice(s, i + 1, s.size);
        }
    }

    return String{NULL,0};
}

String string_get_file_name(String s) {
    for (int i = s.size - 1; i >= 0; i--) {
        if (s.data[i] == '.') {
            return string_slice(s, 0, i);
        }
    }

    return String{NULL, 0};
}

u64 string_hash(String s)
{
    u64 hash = 5383;

    for (int i = 0; i < s.size; i++)
    {
        int c = s.data[i];
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

int string_to_integer(String s, bool* success)
{
    int accum = 0;
    int power = 1;
    for (int i = s.size - 1; i >= 0; i--)
    {
        if (!(s.data[i] >= '0' && s.data[i] <= '9'))
        {
            *success = false;
            return 0;
        }

        accum += (s.data[i] - '0') * power;

        power *= 10;
    }

    *success = true;
    return accum;
}

double string_to_real(String s, bool* success)
{
    char* end_ptr = NULL;
    SCOPE_STRING(s, cstr);
    double res = std::strtod(cstr, &end_ptr);
    if (end_ptr == cstr)
    {
        if (success)
            *success = false;
        return 0.0;
    }

    if (success)
        *success = true;
    return res;
}

long get_file_size(FILE* file) {
	long pos = std::ftell(file);
    std::fseek(file, 0, SEEK_END);
	long len = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
	return len;
}

bool load_file(const char* filepath, BinaryData& data) {
	FILE* handle = std::fopen(filepath, "rb");
    if (!handle)
    {
        return false;
    }

	auto filesize = get_file_size(handle);

	u8* mem = (u8*) std::malloc(filesize);
	if (!mem) {
		panic("malloc fail");
	}

	size_t written = std::fread(mem, sizeof(u8), filesize, handle);
	if (filesize != written) {
        std::free(mem);
		return false;
	}

    std::fclose(handle);

	data.data = mem;
	data.size = filesize;

	return true;
}

bool load_file_text(const char* filepath, String_Builder& builder)
{
	FILE* handle = fopen(filepath, "rb");
    if (!handle)
    {
        return false;
    }

	auto filesize = get_file_size(handle);

    builder.clear();
    builder.ensure_size(filesize);

	size_t written = fread(builder.buffer, sizeof(u8), filesize, handle);
    if (filesize != written) {
        return false;
	}

    builder.cursor = written;
    fclose(handle);

	return true;
}

void File::write_string(String s) {
	fwrite(&s.size, sizeof(s.size), 1, handle);
	fwrite(s.data, sizeof(s.data[0]), s.size, handle);
}

void File::write_number(double n) {
	fwrite(&n, sizeof(n), 1, handle);
}

void File::write_integer(u64 n) {
	fwrite(&n, sizeof(n), 1, handle);
}

String File::read_string() {
	u32 size = 0;  // type must match String.size
	fread(&size, sizeof(size), 1, handle);

	char* data = (char*) malloc(size + 1);

    if (!data)
    {
        panic("Malloc fail");
    }

	fread(data, sizeof(data[0]), size, handle);
	data[size] = '\0';

	return String(data, size);
}

double File::read_number() {
	double n = 0;
	fread(&n, sizeof(n), 1, handle);
	return n;
}

u64 File::read_integer() {
	u64 n = 0;
	fread(&n, sizeof(n), 1, handle);
	return n;
}

void String_Builder::create(int initial_capacity)
{
    buffer = (char*)malloc(initial_capacity);
    if (!buffer) panic("Malloc fail");
    buffer_capacity = initial_capacity;
    cursor = 0;
    buffer[0] = '\0';
}

String_Builder::String_Builder(int initial_capacity) {
    create(initial_capacity);
}

void String_Builder::remove(int amount)
{
    cursor = MAX(0, cursor - amount);
}

void String_Builder::remove_slice(int start, int end)
{
    if (start >= cursor || start >= end)
        return;

    if (end >= cursor)
        end = cursor;

    int amount = cursor - end;
    std::memmove(buffer + start, buffer + end, amount);

    cursor -= (end - start);
}

void String_Builder::resize() {
    int ncap = buffer_capacity ? buffer_capacity * 2 : 32;
    char* nbuff = (char*) malloc(ncap);
    if (!nbuff) panic("Malloc fail");
    if (buffer)
    {
        std::memcpy(nbuff, buffer, cursor);
        std::free(buffer);
    }
    buffer = nbuff;
    buffer_capacity = ncap;
}

int String_Builder::ensure_size(int size) {
    int count = 0;
    while (size >= buffer_capacity) {
        resize();
        count++;
        if (count > 10) {
            fprintf(stderr, "String builder buffer resize failed repeatedly: Possible memory allocation issue or corrupted buffer state.\n"
                "Relevant: buffer_capacity: %d, cursor: %d, provided string size: %d",
                buffer_capacity, cursor, size);
            return 1;
        }
    }

    return 0;
}

int String_Builder::append(String string) {
    ensure_size(cursor + string.size);

    std::memcpy(buffer + cursor, string.data, string.size);
    cursor += string.size;
    return string.size;
}

int String_Builder::append_path(String string)
{
    int total = 0;

    ensure_size(cursor + string.size);
    for (int i = 0; i < string.size; i++) {
        if (string.data[i] == '/')
        {
            total += append(PathSeparator);
        }
        else
        {
            total += append_char(string.data[i]);
        }
    }

    return total;
}

bool String_Builder::ends_with(String s) const {
    return string_compare(s, String(buffer + cursor - s.size, s.size));
}


int String_Builder::append_char(char ch) {
    ensure_size(cursor + 1);

    buffer[cursor] = ch;
    cursor += 1;
    return 1;
}

int String_Builder::append_integer(int n)
{
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer), "%d", n);
    append(make_string(buffer));
    return len;
}

int String_Builder::append_hex(int n) {
    char buffer[128];
    int len = snprintf(buffer, sizeof(buffer), "%x", n);
    append(make_string(buffer));
    return len;
}

int String_Builder::append_float(float n) {
	char buffer[128];
	int len = snprintf(buffer, sizeof(buffer), "%1.3f", n);
	append(make_string(buffer));
	return len;
}

String String_Builder::put_string(String s) {
    int c = cursor;
    append(s);
    // this makes compatibility with so many things much easier
    append_char('\0');
    return String(buffer + c, s.size);
}

String String_Builder::put_path(String path) {
    int c = cursor;
    append_path(path);
    append_char('\0');
    return String(buffer + c, path.size);
}

int String_Builder::clear_and_append(String s) {
    cursor = 0;
    append(s);
    return s.size;
}

int String_Builder::append_many(String* strings, int n) {
    int total_length = 0;
    for (int i = 0; i < n; i++) {
        total_length += strings[i].size;
    }

    ensure_size(this->cursor + total_length);
    for (int i = 0; i < n; i++) {
        std::memcpy(this->buffer + this->cursor, strings[i].data, strings[i].size);
        cursor += strings[i].size;
    }

    return total_length;
}

const char* String_Builder::c_string() {
    ensure_size(32);

    this->buffer[this->cursor] = '\0';
    return this->buffer;
}

void String_Builder::free_buffer() {
    if (buffer) {
        std::free(this->buffer);
    }
    cursor = 0;
    buffer_capacity = 0;
    buffer = NULL;
}

void String_Builder::clear() {
    cursor = 0;
    buffer[0] = '\0';
}

String String_Builder::to_string()
{
    return String(buffer, cursor);
}

String String_Builder::slice(int start, int length) const
{
    return String(buffer + start, length);
}

bool String::advance(int amount) {
    if (amount >= size)
    {
        return false;
    }
    data += amount; size -= amount;
    return true;
}

void String::trim() {
	while (size > 0 && is_space(data[size-1])) {
		size--;
	}

	while (size > 0 && is_space(data[0])) {
		size--;
		data++;
	}
}

bool String::operator==(const String& other) const
{
    return string_compare(*this, other);
}
