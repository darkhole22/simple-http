#pragma once
#include <SimpleHTTP/types.h>

#include <initializer_list>
#include <ranges>


#include <string>

namespace simpleHTTP {

template <typename TEnum>
concept StateIndexEnumKey = requires {
    { TEnum::LAST };
};

template <StateIndexEnumKey TEnum, typename TData, typename TChar>
class ParserFSM
{
public:
    using StateFunction = TEnum(*)(TData*, TChar);

    template<std::ranges::input_range TRange>
    void parse(TData* data, TRange&& s, TEnum start) {
        auto first = std::ranges::begin(s);
        auto last = std::ranges::end(s);

        for (; first != last; ++first) {
            if (start >= TEnum::LAST)
                break;

            auto& f = m_States[static_cast<u64>(start)];
            if (!f) 
                break;

            start = f(data, *first);
        }
    }

    StateFunction m_States[static_cast<u64>(TEnum::LAST)] = { nullptr };
};

}
