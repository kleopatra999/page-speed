// Copyright 2010 Google Inc. All Rights Reserved.
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
// Author: aoates@google.com (Andrew Oates)

#ifndef PAGESPEED_L10N_LOCALE_REGISTER_H_
#define PAGESPEED_L10N_LOCALE_REGISTER_H_

#include <map>
#include <string>
#include <vector>

#include "pagespeed/core/string_util.h"

namespace pagespeed {

namespace l10n {

/**
 * Class for registering locales with GettextLocalizer at link-time.
 *
 * To add a locale, instantiate a static RegisterLocale object with the name of
 * the locale and its string table (as generated by poc).  This allows the
 * RegisterLocale class to build a map at process start-up of available locales.
 *
 * RegisterLocale is thread-safe for reading, but not writing.  To prevent
 * writing after static initialization, call RegisterLocale::Freeze before
 * creating any threads (i.e. in pagespeed::Init()).
 */
class RegisterLocale {
 public:
  // Registers the given locale/string table.  If locale == NULL, then
  // string_table is taken to be the master (native locale) string table.
  RegisterLocale(const char* locale, const char** string_table);
  ~RegisterLocale();

  // Freezes the string tables, so that any subsequent modification causes a
  // fatal error.  Should be called after static initialization, but before any
  // threads are created.
  static void Freeze();

  // Returns the string table for the given locale, or NULL if it doesn't exist.
  static const char** GetStringTable(const std::string& locale);

  // Fills the given vector with all available locales.
  static void GetAllLocales(std::vector<std::string>* out);

  // Returns the master string map (mapping from canonical/native string to
  // table index)
  static const std::map<std::string, size_t>* GetMasterStringMap() {
    return master_string_map_;
  }

 private:
  // flag that indicates any future writing to the string table maps is an error
  static bool frozen_;

  typedef std::map<std::string, const char**,
      ::pagespeed::string_util::CaseInsensitiveStringComparator> StringTableMap;

  // map from locale name to string table --- instantiated in the first
  // RegisterLocale constructor called at process startup.  Deleted in the first
  // RegisterLocale destructor called at process shutdown.
  static StringTableMap* string_table_map_;

  // map from string constant to table index, generated from the "master" string
  // table.  Instantiated when RegisterLocale(NULL, ...) is called.
  static std::map<std::string, size_t>* master_string_map_;
};

} // namespace l10n

} // namespace pagespeed


#endif  // PAGESPEED_L10N_LOCALE_REGISTER_H_