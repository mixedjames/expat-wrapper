#pragma once

#include <james/expat-parser.hpp>
#include <functional>
#include <map>

namespace james {

  struct Path {
    std::string name;
    std::string path;
    int depth;
    int instance;

    Path() : depth(0), instance(0) {}
  };

  struct Attribute {
    const char* name;
    const char* value;

    explicit Attribute(const char** data) : name(data[0]), value(data[1]) {}
    Attribute(const char* name, const char* value) : name(name), value(value) {}
  };

  struct Attributes {

    struct Iterator {
      explicit Iterator(const char** data) : data_(data) {}
      
      bool operator ==(const Iterator& b) { return data_ == b.data_; }
      bool operator !=(const Iterator& b) { return data_ != b.data_; }

      Iterator& operator ++() { data_ += 2; return *this; }
      Iterator& operator --() { data_ -= 2; return *this; }

      Attribute operator *() { return Attribute(data_); }

    private:
      const char** data_;
    };

    explicit Attributes(const char** data);

    const char* operator[] (const char* name) const {
      for (std::size_t i = 0; data_[i]; i += 2) {
        if (strcmp(data_[i], name) == 0) {
          return data_[i + 1];
        }
      }
      return "";
    }

    bool Has(const char* name) const {
      for (std::size_t i = 0; data_[i]; i += 2) {
        if (strcmp(data_[i], name) == 0) {
          return true;
        }
      }
      return false;
    }

    std::size_t Length() const {
      std::size_t i = 0;
      while (data_[i]) {
        i += 2;
      }
      return i >> 1;
    }

    friend Iterator begin(const Attributes&);
    friend Iterator end(const Attributes&);

  private:
    const char** data_;
  };

  struct ExpatFacade;

  struct Tag {
    typedef std::function<void(const Path&, const Attributes&)> TagOpenedFunc;
    typedef std::function<void(const Path&)> TagClosedFunc;
    typedef std::function<void(const Path&, const std::string&)> TextContentFunc;

    TagOpenedFunc TagOpened;
    TagClosedFunc TagClosed;
    TextContentFunc TextContent;

    Tag& Opened(TagOpenedFunc f) { TagOpened = f; return *this; }
    Tag& Closed(TagClosedFunc f) { TagClosed = f; return *this; }
    Tag& Text(TextContentFunc f) { TextContent = f; return *this; }
  };

  struct ExpatFacade
    : private ExpatParser::XMLConsumer
  {

    ExpatFacade();

    ExpatParser::XMLConsumer& XMLConsumer() { return *this; }

    void ListenFor(const std::string&, const Tag&);

  private:
    struct TagData {
      Tag tag;
      std::string textContent;
      int instanceCount;

      TagData(const Tag& tag) : tag(tag), instanceCount(0) {}
    };

    typedef std::multimap<std::string, TagData> TagMap;

    TagMap tags_;
    std::pair<TagMap::iterator, TagMap::iterator> matchedTags_;
    Path currentPath_;

    void StartElement(const char *name, const char **atts) override;
    void EndElement(const char *name) override;
    void CharacterData(const XML_Char *s, int len) override;
  };

} // james