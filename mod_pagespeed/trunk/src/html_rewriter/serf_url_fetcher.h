// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HTML_REWRITER_SERF_URL_FETCHER_H_
#define HTML_REWRITER_SERF_URL_FETCHER_H_

#include <string>
#include "html_rewriter/serf_url_async_fetcher.h"
#include "net/instaweb/util/public/url_fetcher.h"

using net_instaweb::MessageHandler;
using net_instaweb::Writer;
using net_instaweb::MetaData;
using net_instaweb::UrlFetcher;


namespace html_rewriter {

class PageSpeedServerContext;

class SerfUrlFetcher : public UrlFetcher {
 public:
  SerfUrlFetcher(PageSpeedServerContext* context,
                 SerfUrlAsyncFetcher* async_fetcher);
  virtual ~SerfUrlFetcher();
  virtual bool StreamingFetchUrl(const std::string& url,
                                 const MetaData& request_headers,
                                 MetaData* response_headers,
                                 Writer* fetched_content_writer,
                                 MessageHandler* message_handler);

 private:
  PageSpeedServerContext* context_;
  SerfUrlAsyncFetcher* async_fetcher_;
};

}  // namespace html_rewriter

#endif  // HTML_REWRITER_SERF_URL_FETCHER_H_