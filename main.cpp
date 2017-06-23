#include <iostream>
#include <fstream>
#include <string>

#include <james/expat-parser.hpp>
#include <james/expat-parser-dispatcher.hpp>
#include <james/expat-facade.hpp>

using namespace james;
using namespace std;

string CollapseWhitespace(const string& src) {
  string result;
  bool inWhitespace = false;
  
  for (char c : src) {
    if (isspace(c)) {
      if (inWhitespace) {
        continue;
      }
      else {
        inWhitespace = true;
        result.push_back(' ');
      }
    }
    else {
      inWhitespace = false;
      result.push_back(c);
    }
  }
  return result;
}

int main() {

  ExpatFacade dispatcher;
  ExpatParser parser(dispatcher.XMLConsumer());

  Tag root;

  dispatcher.ListenFor("/root", Tag()
    .Opened([](const Path& p, const Attributes& atts) {
      cout << "<root>\n";

      for (auto a : atts) {
        cout << "  " << a.name << " = " << a.value << "\n";
      }
    })
    .Closed([](const Path& p) {
      cout << "</root>\n";
    })
    .Text([](const Path& p, const string& txt) {
      cout << "Text of " << p.name << " = " << CollapseWhitespace(txt) << "\n";
    })
  );

  dispatcher.ListenFor("/root/a", Tag()
    .Opened([](const Path& p, const Attributes& a) {
      cout << "  <a> (n = " << p.instance << ")\n";
    })
    .Closed([](const Path& p) {
      cout << "  </a>\n";
    })
    .Text([](const Path& p, const string& txt) {
      cout << "  Text of " << p.name << " = " << CollapseWhitespace(txt) << "\n";
    })
  );

  dispatcher.ListenFor("/root/throw", Tag()
    .Opened([](const Path&, const Attributes&) {
      throw runtime_error("Lookit, we found a <throw/> tag!");
    })
  );

  ifstream src("test.xml", ios::in);

  if (src.good()) {
    cout << "Parsing the XML...\n";
    src.exceptions(src.exceptions() | ios::eofbit);
    
    try {
      ParseStream(parser, src);
    }
    catch (ExpatParser::Exception& e) {
      cout << "XML error occured on line " << e.Line() << ":\n";
      cout << "  " << e.Message() << "\n";
    }
    catch (std::runtime_error& e) {
      cout << "A non-specific error occured:\n";
      cout << e.what() << "\n";
    }
  }
  else {
    cout << "An error occured opening the file.";
  }

  cin.get();
}