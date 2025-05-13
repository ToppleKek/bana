#pragma once

#include "bana.hpp"

#define BEGIN_TIMED_BLOCK(NAME) f64 NAME##_TIMED_BLOCK = Bana::Platform::get_current_time()
#define END_TIMED_BLOCK(NAME)   ICHIGO_INFO("Timed block \"" #NAME "\" took %fms!", (Bana::Platform::get_current_time() - NAME##_TIMED_BLOCK) * 1000.0)

namespace Bana {
namespace Platform {
struct File;

File *open_file_write(const String path);
void write_entire_file_sync(const char *path, const u8 *data, usize data_size);

// Append to an open file. Generally works like fwrite() from the CRT.
void append_file_sync(File *file, const u8 *data, usize data_size);
void close_file(File *file);
Bana::Optional<Bana::FixedArray<u8>> read_entire_file_sync(const Bana::String path, Bana::Allocator allocator = Bana::heap_allocator);

bool file_exists(const char *path);
void sleep(f64 t);
f64 get_current_time();

wchar_t *win32_to_wide_char(const Bana::String &str, Bana::Allocator allocator = Bana::heap_allocator);
Bana::String win32_from_wide_char(const wchar_t *str, Bana::Allocator allocator = Bana::heap_allocator);

void init();
}
}
