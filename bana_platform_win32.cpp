#define _CRT_SECURE_NO_WARNINGS
#define OEMRESOURCE
#define UNICODE
#include <windows.h>

#include "bana_platform.hpp"

static i64 performance_frequency = 0;

struct Bana::Platform::File {
    HANDLE file_handle;
};

static Bana::Platform::File open_files[32];

static i64 win32_get_timestamp() {
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    return i.QuadPart;
}

[[maybe_unused]] static i64 win32_get_time_ms() {
    return win32_get_timestamp() * 1000 / performance_frequency;
}

Bana::String Bana::Platform::win32_from_wide_char(const wchar_t *str, Bana::Allocator allocator) {
    i32 u8_buf_size = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    assert(u8_buf_size > 0);

    Bana::String ret = Bana::make_string(u8_buf_size, allocator);
    ret.length       = u8_buf_size;

    WideCharToMultiByte(CP_UTF8, 0, str, -1, ret.data, u8_buf_size, nullptr, nullptr);
    return ret;
}

wchar_t *Bana::Platform::win32_to_wide_char(const Bana::String &str, Bana::Allocator allocator) {
    i32 buf_size = MultiByteToWideChar(CP_UTF8, 0, str.data, str.length, nullptr, 0);
    assert(buf_size > 0);
    wchar_t *wide_buf = (wchar_t *) allocator.alloc((buf_size + 1) * sizeof(wchar_t));
    std::memset(wide_buf, 0, (buf_size + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str.data, str.length, wide_buf, buf_size);
    return wide_buf;
}

Bana::Platform::File *Bana::Platform::open_file_write(Bana::String path) {
    wchar_t *pathw = win32_to_wide_char(path);
    HANDLE file = CreateFile(pathw, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    std::free(pathw);

    if (file == INVALID_HANDLE_VALUE) {
        ICHIGO_ERROR("Failed to open file for writing!");
        return nullptr;
    }

    for (u32 i = 0; i < ARRAY_LEN(open_files); ++i) {
        if (open_files[i].file_handle == INVALID_HANDLE_VALUE) {
            open_files[i].file_handle = file;
            return &open_files[i];
        }
    }

    ICHIGO_ERROR("Too many files open!");
    return nullptr;
}

void Bana::Platform::write_entire_file_sync(const char *path, const u8 *data, usize data_size) {
    wchar_t *pathw = win32_to_wide_char(Bana::temp_string(path));
    HANDLE file = CreateFile(pathw, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    std::free(pathw);

    if (file == INVALID_HANDLE_VALUE) {
        ICHIGO_ERROR("Failed to open file for writing!");
        return;
    }

    DWORD bytes_written = 0;
    if (!WriteFile(file, data, data_size, &bytes_written, nullptr)) {
        ICHIGO_ERROR("Failed to write file!");
    }

    CloseHandle(file);
}

void Bana::Platform::append_file_sync(File *file, const u8 *data, usize data_size) {
    DWORD bytes_written = 0;
    if (!WriteFile(file->file_handle, data, data_size, &bytes_written, nullptr)) {
        ICHIGO_ERROR("Failed to write to file!");
    }
}

void Bana::Platform::close_file(File *file) {
    CloseHandle(file->file_handle);
    file->file_handle = INVALID_HANDLE_VALUE;
}

Bana::Optional<Bana::FixedArray<u8>> Bana::Platform::read_entire_file_sync(const Bana::String path, Bana::Allocator allocator) {
    wchar_t *pathw = win32_to_wide_char(path);
    HANDLE handle = CreateFile(pathw, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    std::free(pathw);

    if (handle == INVALID_HANDLE_VALUE) {
        return {};
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(handle, &file_size)) {
        CloseHandle(handle);
        return {};
    }

    Bana::FixedArray<u8> ret = Bana::make_fixed_array<u8>(file_size.QuadPart, allocator);

    DWORD bytes_read = 0;
    if (!ReadFile(handle, ret.data, file_size.QuadPart, &bytes_read, nullptr)) {
        CloseHandle(handle);
        return {};
    }

    ret.size = file_size.QuadPart;

    CloseHandle(handle);
    return ret;
}

bool Bana::Platform::file_exists(const char *path) {
    wchar_t *wide_path = win32_to_wide_char(Bana::temp_string(path));
    DWORD attributes = GetFileAttributesW(wide_path);
    bool ret = attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
    std::free(wide_path);
    return ret;
}

void Bana::Platform::sleep(f64 t) {
    Sleep((u32) (t * 1000));
}

f64 Bana::Platform::get_current_time() {
    return (f64) win32_get_timestamp() / (f64) performance_frequency;
}

void Bana::Platform::init() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    performance_frequency = frequency.QuadPart;

    for (u32 i = 0; i < ARRAY_LEN(open_files); ++i) {
        open_files[i].file_handle = INVALID_HANDLE_VALUE;
    }

    assert(timeBeginPeriod(1) == TIMERR_NOERROR);
}
