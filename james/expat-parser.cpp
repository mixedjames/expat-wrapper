#include "expat-parser.hpp"

#include <vector>
#include <assert.h>

namespace james {

  ExpatParser::ExpatParser(XMLConsumer& consumer, RegisteredHandlers handlers)
    : consumer_(consumer), parser_(XML_ParserCreate(nullptr)), done_(false)
  {
    if (!parser_) {
      throw std::runtime_error("Unable to create Expat parser (XML_ParserCreate failed)");
    }

    XML_SetUserData(parser_, this);
    XML_SetElementHandler(parser_, StartElement, EndElement);
    XML_SetCharacterDataHandler(parser_, CharacterDataHandler);

    if (handlers & DEFAULT_HANDLER) {
      XML_SetDefaultHandler(parser_, DefaultHandler);
    }

    if (handlers & PI_HANDLER) {
      XML_SetProcessingInstructionHandler(parser_, ProcessingInstruction);
    }

    if (handlers & COMMENT_HANDLER) {
      XML_SetCommentHandler(parser_, Comment);
    }

    if (handlers & CDATA_HANDLER) {
      XML_SetCdataSectionHandler(parser_, StartCData, EndCData);
    }
  }

  ExpatParser::~ExpatParser() {
    XML_ParserFree(parser_);
  }

  void ExpatParser::Parse(const char* data, size_t length, bool done) {
    assert(!done_);

    done_ = done;
    currentException_ = nullptr;

    if (XML_Parse(parser_, data, length, done) == XML_STATUS_ERROR) {
      // Two distinct reasons for XML_STATUS_ERROR are possible & require different action:
      // (1) A user callback threw an exception - in which case the exception will have been stored
      //     in currentException_ and simply needs to be rethrow.
      // (2) An Expat error occured, probably because of duff XML data but could be anything.
      //     In this case we throw an ExpatParser::Exception 

      if (currentException_) {
        std::rethrow_exception(currentException_);
      }
      else {
        throw Exception(
          XML_ErrorString(XML_GetErrorCode(parser_)),
          XML_GetErrorCode(parser_),
          XML_GetCurrentLineNumber(parser_)
        );
      }
    }

  }

  void ExpatParser::Parse(const std::string& xml, bool done) {
    Parse(xml.c_str(), xml.size(), done);
  }

  void ExpatParser::StartElement(void *userData, const char *name, const char **atts) {
    // Note on commenting:
    //
    // ExpatParser::StartElement is heavily commented but other callback handlers
    // have no comments at all. Since they are all carbon copies of StartElement with
    // a few context specific changes copying the comments seemed pointless.

    ExpatParser* parser = (ExpatParser*)userData;

    // Don't forward callbacks if an exception has been thrown.
    //
    // Check is needed because some callbacks may still be called after XML_StopParser
    // is called. The documentation does not specify which ones so I've included the check
    // everywhere in the spirit of "belt n braces"
    if (parser->currentException_) {
      return;
    }

    try {
      parser->consumer_.StartElement(name, atts);
    }
    catch (...) {
      // Basic idea: if an exception is thrown:
      // (1) Stop the parser (passing resumable = XML_FALSE to prevent resuming later)
      // (2) Store the exception for rethrowing later
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::EndElement(void *userData, const char *name) {
    ExpatParser* parser = (ExpatParser*)userData;

    if (parser->currentException_) {
      return;
    }

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

    if (parser->currentException_) {
      return;
    }

    try {
      parser->consumer_.CharacterData(s, len);
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::DefaultHandler(void *userData, const XML_Char *s, int len) {
    ExpatParser* parser = (ExpatParser*)userData;

    if (parser->currentException_) {
      return;
    }

    try {
      parser->consumer_.DefaultHandler(s, len);
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::ProcessingInstruction(void *userData, const XML_Char *target, const XML_Char *data) {
    ExpatParser* parser = (ExpatParser*)userData;

    if (parser->currentException_) {
      return;
    }

    try {
      parser->consumer_.ProcessingInstruction(target, data);
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::Comment(void *userData, const XML_Char *data) {
    ExpatParser* parser = (ExpatParser*)userData;

    if (parser->currentException_) {
      return;
    }

    try {
      parser->consumer_.Comment(data);
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::StartCData(void *userData) {
    ExpatParser* parser = (ExpatParser*)userData;

    if (parser->currentException_) {
      return;
    }

    try {
      parser->consumer_.StartCData();
    }
    catch (...) {
      XML_StopParser(parser->parser_, XML_FALSE);
      parser->currentException_ = std::current_exception();
    }
  }

  void ExpatParser::EndCData(void *userData) {
    ExpatParser* parser = (ExpatParser*)userData;

    if (parser->currentException_) {
      return;
    }

    try {
      parser->consumer_.EndCData();
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
