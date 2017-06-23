#pragma once

#include <expat.h>
#include <stdexcept>
#include <istream>

namespace james {

  struct ExpatParser {
    struct Exception
      : std::runtime_error
    {
      Exception(const XML_LChar* msg, XML_Error code, XML_Size line)
        : std::runtime_error("ExpatParser::Exception"), msg_(msg), code_(code), line_(line)
      {}

      const XML_LChar* Message() const { return msg_; }
      XML_Error Code() const { return code_; }
      XML_Size Line() const { return line_; }

    private:
      const XML_LChar* msg_;
      XML_Error code_;
      XML_Size line_;
    };

    struct XMLConsumer {
      virtual ~XMLConsumer() {}

      virtual void StartElement(const char *name, const char **atts) {}
      virtual void EndElement(const char *name) {}

      virtual void CharacterData(const XML_Char *s, int len) {}
    };

    ExpatParser(XMLConsumer&);
    ~ExpatParser();

    ExpatParser(const ExpatParser&) = delete;
    ExpatParser& operator =(const ExpatParser&) = delete;

    void Parse(const char* data, size_t length, bool done);
    void Parse(const std::string&, bool done = true);

  private:
    XMLConsumer& consumer_;
    XML_Parser parser_;
    bool done_;
    std::exception_ptr currentException_;

    static void XMLCALL StartElement(void *userData, const char *name, const char **atts);
    static void XMLCALL EndElement(void *userData, const char *name);
    static void XMLCALL CharacterDataHandler(void *userData, const XML_Char *s, int len);
  };

  //
  // Non-member utility functions
  //

  bool HasAttribute(const char** atts, const char* name);

  const char* FindAttribute(const char** atts, const char* name, const char* defaultVal = nullptr);

  void ParseStream(ExpatParser& parser, std::istream&, size_t bufferSize = 1024);
}