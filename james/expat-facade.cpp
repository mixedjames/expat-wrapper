#include "expat-facade.hpp"

using namespace std;

namespace james {

  Attributes::Attributes(const char** data)
    : data_(data)
  {
  }

  Attributes::Iterator begin(const Attributes& a) {
    return Attributes::Iterator(a.data_);
  }

  Attributes::Iterator end(const Attributes& a) {
    const char** i = a.data_;
    while (*(i += 2)) {}
    return Attributes::Iterator(i);
  }

  ExpatFacade::ExpatFacade()
  {
  }

  void ExpatFacade::ListenFor(const std::string& path, const Tag& t) {
    tags_.insert(make_pair(path, TagData(t)));
  }

  void ExpatFacade::StartElement(const char *name, const char **atts) {
    // Step 1: Flush any accumulated text content for the parent tag
    //
    for (auto i = matchedTags_.first; i != matchedTags_.second; ++i) {
      if (i->second.tag.TextContent && i->second.textContent.length() > 0) {
        i->second.tag.TextContent(currentPath_, i->second.textContent);
      }
      i->second.textContent.clear();
    }

    // Step 2: update the current Path to reflect the new tage
    //
    currentPath_.name = name;
    currentPath_.path += std::string("/") + name;
    currentPath_.depth++;

    // Step 3: find the interested tag listeners
    //
    matchedTags_ = tags_.equal_range(currentPath_.path);

    // Step 4: dispatch the TagOpened event
    //
    Attributes attributes(atts);

    for (auto i = matchedTags_.first; i != matchedTags_.second; ++ i) {
      currentPath_.instance = ++ i->second.instanceCount;

      if (i->second.tag.TagOpened) {
        i->second.tag.TagOpened(currentPath_, attributes);
      }
    }
  }

  void ExpatFacade::EndElement(const char *name) {
    // Step 1: update the current path tag name
    //         (in the case of stacked closing tags - i.e. </b></a>
    //         this will reflect the previous tag name otherwise)
    //
    currentPath_.name = name;

    // Step 2: find interested tag listeners & dispatch events
    //
    matchedTags_ = tags_.equal_range(currentPath_.path);
    
    for (auto i = matchedTags_.first; i != matchedTags_.second; ++i) {
      currentPath_.instance = i->second.instanceCount;

      // Step 2a: dispatch any pending text content & clear the buffer
      //
      if (i->second.tag.TextContent && i->second.textContent.length() > 0) {
        i->second.tag.TextContent(currentPath_, i->second.textContent);
        i->second.textContent.clear();
      }

      // Step 2b: dispatch the closed event
      //
      if (i->second.tag.TagClosed) {
        i->second.tag.TagClosed(currentPath_);
      }
    }

    // Step 3: calculate the parent path details
    //
    currentPath_.path.erase(currentPath_.path.length() - strlen(name) - 1);
    currentPath_.depth--;

    // 'If' Required because when EndElement is called for the root element,
    // the path will be empty as the root tag will just have been erased.
    if (currentPath_.path.size() > 0) {
      currentPath_.name = currentPath_.path.substr(currentPath_.path.find_last_of('/')+1);
    }

    // Step 4: find any tag listeners interested in the new path
    //
    matchedTags_ = tags_.equal_range(currentPath_.path);
  }

  void ExpatFacade::CharacterData(const XML_Char *s, int len) {
    for (auto i = matchedTags_.first; i != matchedTags_.second; ++i) {
      // Only store text if the tag listener is interested in it...
      if (i->second.tag.TextContent) {
        i->second.textContent.append(s, len);
      }
    }
  }

} // james