#pragma once
#include <memory>

namespace simpleHTTP {

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using i8 = char;
using i16 = short;
using i32 = int;
using i64 = long long;

template <typename T> constexpr bool isSigned = static_cast<T>(0) > static_cast<T>(-1);

static_assert(sizeof(u8) == 1 && !isSigned<u8>);
static_assert(sizeof(u16) == 2 && !isSigned<u16>);
static_assert(sizeof(u32) == 4 && !isSigned<u32>);
static_assert(sizeof(u64) == 8 && !isSigned<u64>);

static_assert(sizeof(i8) == 1 && isSigned<i8>);
static_assert(sizeof(i16) == 2 && isSigned<i16>);
static_assert(sizeof(i32) == 4 && isSigned<i32>);
static_assert(sizeof(i64) == 8 && isSigned<i64>);

template <class T> using Ref = std::shared_ptr<T>;
template <class T> using WRef = std::weak_ptr<T>;
template <class T> using URef = std::unique_ptr<T>;

template <class T, class... Args>
Ref<T> makeRef(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

} // namespace simpleHTTP
