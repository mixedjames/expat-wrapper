#pragma once

#include <james/expat-parser.hpp>
#include <vector>
#include <utility>

namespace james {

  struct ExpatParserDispatcher
    : ExpatParser::XMLConsumer
  {
    struct NodeID {
      const char* name;
      std::string path;
      int depth;

      NodeID() : name(nullptr), depth(0) {}
    };

    struct XMLConsumer {
      virtual ~XMLConsumer() {}

      virtual void StartElement(const NodeID& id, const char **atts) {}
      virtual void EndElement(const NodeID& id) {}

      virtual void CharacterData(const NodeID& id, const XML_Char *s, int len) {}
    };

    ExpatParserDispatcher() : defaultConsumer_(nullptr) {}

    void AddConsumer(const std::string& tagName, XMLConsumer* consumer);
    void SetDefaultConsumer(XMLConsumer* consumer);

    void StartElement(const char *name, const char **atts) override;
    void EndElement(const char *name) override;
    void CharacterData(const XML_Char *s, int len) override;

  private:
    XMLConsumer* defaultConsumer_;
    std::vector<std::pair<std::string, XMLConsumer*>> consumers_;
    NodeID currentNode_;
  };

} // james