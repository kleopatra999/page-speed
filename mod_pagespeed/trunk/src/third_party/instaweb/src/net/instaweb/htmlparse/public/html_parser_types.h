// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSER_TYPES_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSER_TYPES_H_

#include <list>

namespace net_instaweb {

class FileSystem;
class HtmlCdataNode;
class HtmlCharactersNode;
class HtmlCommentNode;
class HtmlDirectiveNode;
class HtmlElement;
class HtmlEvent;
class HtmlFilter;
class HtmlLeafNode;
class HtmlLexer;
class HtmlNode;
class HtmlParse;
class HtmlStartElementEvent;
class HtmlWriterFilter;
class LibxmlAdapter;
class MessageHandler;
class Writer;

typedef std::list<HtmlEvent*> HtmlEventList;
typedef HtmlEventList::iterator HtmlEventListIterator;

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_PARSER_TYPES_H_
