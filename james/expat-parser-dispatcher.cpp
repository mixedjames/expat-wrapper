#include "expat-parser-dispatcher.hpp"

#include <algorithm>
#include <iostream>

namespace james {

  namespace {
    bool ComparePair(const std::pair<std::string, ExpatParserDispatcher::XMLConsumer*>& a, const std::pair<std::string, ExpatParserDispatcher::XMLConsumer*>& b) {
      return a.first < b.first;
    }
  }

  void ExpatParserDispatcher::AddConsumer(const std::string& tagName, XMLConsumer* consumer) {
    consumers_.insert(
      std::upper_bound(consumers_.begin(), consumers_.end(), std::make_pair(tagName, (ExpatParserDispatcher::XMLConsumer*)nullptr), ComparePair),
      std::make_pair(tagName, consumer)
    );
  }

  void ExpatParserDispatcher::SetDefaultConsumer(XMLConsumer* consumer) {
    defaultConsumer_ = consumer;
  }

  void ExpatParserDispatcher::StartElement(const char *name, const char **atts) {
    currentNode_.name = name;
    currentNode_.path += name + std::string("/");
    currentNode_.depth++;

    auto i = std::lower_bound(
      consumers_.begin(), consumers_.end(),
      std::make_pair(currentNode_.path, (ExpatParserDispatcher::XMLConsumer*)nullptr),
      ComparePair
    );

    auto end(consumers_.end());
    if (i != end) {
      for (; i != end && i->first == currentNode_.path; ++i) {
        i->second->StartElement(currentNode_, atts);
      }
    }
    else if (defaultConsumer_) {
      defaultConsumer_->StartElement(currentNode_, atts);
    }
  }

  void ExpatParserDispatcher::EndElement(const char *name) {
    currentNode_.name = name;

    auto i = std::lower_bound(
      consumers_.begin(), consumers_.end(),
      std::make_pair(currentNode_.path, (ExpatParserDispatcher::XMLConsumer*)nullptr),
      ComparePair
    );

    auto end(consumers_.end());
    if (i != end) {
      for (; i != end && i->first == currentNode_.path; ++i) {
        i->second->EndElement(currentNode_);
      }
    }
    else if (defaultConsumer_) {
      defaultConsumer_->EndElement(currentNode_);
    }

    currentNode_.path.erase(currentNode_.path.length() - strlen(name) - 1);
    currentNode_.depth--;
  }

  void ExpatParserDispatcher::CharacterData(const XML_Char *s, int len) {
    auto i = std::lower_bound(
      consumers_.begin(), consumers_.end(),
      std::make_pair(currentNode_.path, (ExpatParserDispatcher::XMLConsumer*)nullptr),
      ComparePair
    );

    auto end(consumers_.end());
    if (i != end) {
      for (; i != end && i->first == currentNode_.path; ++i) {
        i->second->CharacterData(currentNode_, s, len);
      }
    }
    else if (defaultConsumer_) {
      defaultConsumer_->CharacterData(currentNode_, s, len);
    }
  }


} // james