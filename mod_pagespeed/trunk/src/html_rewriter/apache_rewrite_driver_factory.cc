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

#include "html_rewriter/apache_rewrite_driver_factory.h"

#include "html_rewriter/apr_file_system.h"
#include "html_rewriter/apr_mutex.h"
#include "html_rewriter/apr_timer.h"
#include "html_rewriter/html_parser_message_handler.h"
#include "html_rewriter/html_rewriter_config.h"
#include "html_rewriter/md5_hasher.h"
#include "html_rewriter/pagespeed_server_context.h"
#include "html_rewriter/serf_url_async_fetcher.h"
#include "html_rewriter/serf_url_fetcher.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/file_cache.h"
#include "third_party/apache/apr/src/include/apr_pools.h"

using html_rewriter::SerfUrlAsyncFetcher;

namespace net_instaweb {

ApacheRewriteDriverFactory::ApacheRewriteDriverFactory(
    html_rewriter::PageSpeedServerContext* context)
  : context_(context) {
  apr_pool_create(&pool_, context->pool());
  set_filename_prefix(html_rewriter::GetCachePrefix(context_));
  set_url_prefix(html_rewriter::GetUrlPrefix(context_));
  cache_mutex_.reset(NewMutex());
  rewrite_drivers_mutex_.reset(NewMutex());
}

ApacheRewriteDriverFactory::~ApacheRewriteDriverFactory() {
  // We free all the resources before destroy the pool, because some of the
  // resource uses the sub-pool and will destroy them on destruction.
  ShutDown();
  apr_pool_destroy(pool_);
}

FileSystem* ApacheRewriteDriverFactory::DefaultFileSystem() {
  return new html_rewriter::AprFileSystem(pool_);
}

Hasher* ApacheRewriteDriverFactory::NewHasher() {
  return new html_rewriter::Md5Hasher();
}

Timer* ApacheRewriteDriverFactory::DefaultTimer() {
  return new html_rewriter::AprTimer();
}

MessageHandler* ApacheRewriteDriverFactory::DefaultHtmlParseMessageHandler() {
  return new html_rewriter::HtmlParserMessageHandler();
}

CacheInterface* ApacheRewriteDriverFactory::DefaultCacheInterface() {
  return new FileCache(html_rewriter::GetFileCachePath(context_),
                       file_system(),
                       filename_encoder(),
                       html_parse_message_handler());
}

UrlFetcher* ApacheRewriteDriverFactory::DefaultUrlFetcher() {
  SerfUrlAsyncFetcher* async_fetcher =
      reinterpret_cast<SerfUrlAsyncFetcher*>(url_async_fetcher());
  return new html_rewriter::SerfUrlFetcher(context_, async_fetcher);
}
UrlAsyncFetcher* ApacheRewriteDriverFactory::DefaultAsyncUrlFetcher() {
  return new SerfUrlAsyncFetcher(html_rewriter::GetFetcherProxy(context_),
                                 pool_);
}


HtmlParse* ApacheRewriteDriverFactory::DefaultHtmlParse() {
  return new HtmlParse(html_parse_message_handler());
}

AbstractMutex* ApacheRewriteDriverFactory::NewMutex() {
  return new html_rewriter::AprMutex(pool_);
}

RewriteDriver* ApacheRewriteDriverFactory::GetRewriteDriver() {
  RewriteDriver* rewrite_driver = NULL;
  if (!available_rewrite_drivers_.empty()) {
    rewrite_driver = available_rewrite_drivers_.back();
    available_rewrite_drivers_.pop_back();
  } else {
    // Create a RewriteDriver using base class.
    rewrite_driver = NewRewriteDriver();
  }
  active_rewrite_drivers_.insert(rewrite_driver);
  return rewrite_driver;
}

void ApacheRewriteDriverFactory::ReleaseRewriteDriver(
    RewriteDriver* rewrite_driver) {
  int count = active_rewrite_drivers_.erase(rewrite_driver);
  if (count != 1) {
    LOG(ERROR) << "Remove rewrite driver from the active list.";
  } else {
    available_rewrite_drivers_.push_back(rewrite_driver);
  }
}

void ApacheRewriteDriverFactory::ShutDown() {
  cache_mutex_.reset(NULL);
  rewrite_drivers_mutex_.reset(NULL);
  RewriteDriverFactory::ShutDown();
}

}  // namespace net_instaweb