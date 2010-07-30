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

#ifndef HTML_REWRITER_PAGESPEED_SERVER_CONTEXT_H_
#define HTML_REWRITER_PAGESPEED_SERVER_CONTEXT_H_

#include "base/scoped_ptr.h"
#include "html_rewriter/apache_rewrite_driver_factory.h"

// Forward declaration.
struct apr_pool_t;

namespace html_rewriter {

class PageSpeedServerContext;
struct PageSpeedConfig {
  PageSpeedServerContext* context;
  bool pagespeed_enable;
  const char* rewrite_url_prefix;
  const char* fetch_proxy;
  const char* generated_file_prefix;
  const char* file_cache_path;
  int64 fetcher_timeout_ms;
  int64 resource_timeout_ms;
  int num_shards;
  bool use_http_cache;
  bool use_threadsafe_cache;
  bool combine_css;
  bool outline_css;
  bool outline_javascript;
  bool rewrite_images;
  bool extend_cache;
  bool add_head;
  bool add_base_tag;
  bool remove_quotes;
  bool force_caching;
};

class PageSpeedServerContext {
 public:
  explicit PageSpeedServerContext(apr_pool_t* pool, PageSpeedConfig* config);
  ~PageSpeedServerContext();
  void set_rewrite_driver_factory(
      net_instaweb::ApacheRewriteDriverFactory* factory) {
    rewrite_driver_factory_.reset(factory);
  }
  net_instaweb::ApacheRewriteDriverFactory* rewrite_driver_factory() {
    return rewrite_driver_factory_.get();
  }
  apr_pool_t* pool() { return pool_; }
  const PageSpeedConfig* config() const { return config_; }

 private:
  apr_pool_t* pool_;
  PageSpeedConfig* config_;
  scoped_ptr<net_instaweb::ApacheRewriteDriverFactory> rewrite_driver_factory_;
};

bool CreatePageSpeedServerContext(apr_pool_t* pool, PageSpeedConfig* config);

}  // namespace html_rewriter

#endif  // HTML_REWRITER_PAGESPEED_SERVER_CONTEXT_H_