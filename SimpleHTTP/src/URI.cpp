#include <SimpleHTTP/URI.h>
#include "fsm.h"

#include <array>

namespace simpleHTTP {

enum class URIParserState
{
    S0,
    LAST
};

using URIParser = ParserFSM<URIParserState, URI, i8>;

static URIParserState s0_function(URI*, i8) {
    return URIParserState::S0;
};

constexpr URIParser parser = { s0_function };

URI::URI() {}

URI::~URI() {}

}
