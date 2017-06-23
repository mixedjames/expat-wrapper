#include "expat-parser.hpp"

#include <vector>
#include <assert.h>

namespace james {

  ExpatParser::ExpatParser(XMLConsumer& consumer)
    : consumer_(consumer), parser_(XML_ParserCreate(nullptr)), done_(false)
  {
    if (!parser_) {
      throw std::runtime_error("Unable to create Expat parser (XML_ParserCreate failed)");
    }

    XML_SetUserData(parser_, this);
    XML_SetElementHandler(parser_, StartElement, EndElement);
    XML_SetCharacterDataHandler(parser_, CharacterDataHandler);
  }

  ExpatParser::~ExpatParser() {
    XML_ParserFree(parser_);
  }

  void ExpatParser::Parse(const char* data, size_t length, bool done) {
    assert(!done_);

    done_ = done;
    if (XML_Parse(parser_, data, length, done) == XML_STATUS_ERROR) {
      throw Exception(
        XML_ErrorString(XML_GetErrorCode(parser_)),
        XML_GetErrorCode(parser_),
        XML_GetCurrentLineNumber(parser_)
      );
    }

    if (currentException_) {
      std::rethrow_exception(currentException_);
    }
  }

  void ExpatParser::Parse(const std::string& xml, bool done) {
    Parse(xml.c_str(), xml.size(), done);
  }

  void ExpatParser::StartElement(void *userData, const char *name, const char **atts) {
    ExpatParser* parser = (ExpatParser*)userData;

    try {
      parser->consumer_.StartElement(name, atts);
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::EndElement(void *userData, const char *name) {
    ExpatParser* parser = (ExpatParser*)userData;

    try {
      parser->consumer_.EndElement(name);
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::CharacterDataHandler(void *userData, const XML_Char *s, int len) {
    ExpatParser* parser = (ExpatParser*)userData;

    try {
      parser->consumer_.CharacterData(s, len);
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  //
  // **************************************
  // Non-member utility functions
  // **************************************
  //
  bool HasAttribute(const char** atts, const char* name) {
    return !!FindAttribute(atts, name, nullptr);
  }

  const char* FindAttribute(const char** atts, const char* name, const char* defaultVal) {
    for (size_t i = 0; atts[i]; i += 2) {
      if (strcmp(atts[i], name) == 0) {
        return atts[i+1];
      }
    }
    return defaultVal;
  }

  void ParseStream(ExpatParser& parser, std::istream& src, size_t bufferSize) {
    std::vector<char> buffer(bufferSize, 0);
    std::streamsize bytesRead;

    std::ios::iostate exceptionState(src.exceptions());
    src.exceptions(exceptionState & ~std::ios::eofbit);

    try {
      while (true) {
        src.read(&buffer[0], buffer.size());
        bytesRead = src.gcount();

        if (bytesRead > 0) {
          parser.Parse(&buffer[0], (size_t) bytesRead, XML_FALSE);
        }
        if (src.eof()) {
          break;
        }
      }

      parser.Parse(&buffer[0], 0, XML_TRUE);
    }
    catch (...) {
      src.clear(src.rdstate() & ~std::ios::eofbit);
      src.exceptions(exceptionState);
      throw;
    }

    src.clear(src.rdstate() & ~std::ios::eofbit);
    src.exceptions(exceptionState);
  }
}
