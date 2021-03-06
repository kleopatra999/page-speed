// Copyright 2009 Google Inc. All Rights Reserved.
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

// Command line utility that runs lint rules on the provided input set.

#include <stdio.h>

#include <fstream>

#include "base/logging.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/formatters/json_formatter.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/formatters/text_formatter.h"
#include "pagespeed/har/http_archive.h"
#include "pagespeed/proto/pagespeed_input.pb.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/proto_resource_utils.h"
#include "pagespeed/rules/rule_provider.h"

namespace {

/**
 * Formatter that prints the binary ResultText protobuf output.
 */
class PrintProtoFormatter : public pagespeed::formatters::ProtoFormatter {
 public:
  PrintProtoFormatter() : ProtoFormatter(&results_) {}
  virtual ~PrintProtoFormatter() {
    STLDeleteContainerPointers(results_.begin(), results_.end());
  }

 protected:
  // Formatter interface
  virtual void DoneAddingChildren() {
    ProtoFormatter::DoneAddingChildren();

    ::google::protobuf::io::OstreamOutputStream out_stream(&std::cout);
    for (std::vector<pagespeed::ResultText*>::const_iterator
             it = results_.begin(),
             end = results_.end();
         it != end;
         ++it) {
      (*it)->SerializeToZeroCopyStream(&out_stream);
    }
  }

 private:
  std::vector<pagespeed::ResultText*> results_;
};

pagespeed::PagespeedInput* ParseProtoInput(const std::string& file_contents) {
  pagespeed::ProtoInput input_proto;
  bool success = ::google::protobuf::TextFormat::ParseFromString(
      file_contents, &input_proto);
  CHECK(success);

  pagespeed::PagespeedInput *input = new pagespeed::PagespeedInput;
  pagespeed::proto::PopulatePagespeedInput(input_proto, input);
  return input;
}

void PrintUsage() {
  fprintf(stderr, "Usage: pagespeed <output_format> <input_format> <input>\n");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    PrintUsage();
    return 1;
  }

  const std::string out_format = argv[1];
  const std::string in_format = argv[2];
  const std::string filename = argv[3];
  std::ifstream in(filename.c_str());
  if (!in) {
    fprintf(stderr, "Could not read input from %s\n", filename.c_str());
    PrintUsage();
    return 1;
  }

  std::string file_contents;
  std::string line;
  while (std::getline(in, line)) {
    file_contents += line;
    file_contents += '\n';
  }

  scoped_ptr<pagespeed::RuleFormatter> formatter;
  if (out_format == "json") {
    formatter.reset(new pagespeed::formatters::JsonFormatter(&std::cout,
                                                             NULL));
  } else if (out_format == "proto") {
    formatter.reset(new PrintProtoFormatter);
  } else if (out_format == "text") {
    formatter.reset(new pagespeed::formatters::TextFormatter(&std::cout));
  } else {
    fprintf(stderr, "Invalid output format %s\n", out_format.c_str());
    PrintUsage();
    return 1;
  }
  CHECK(formatter.get() != NULL);

  scoped_ptr<pagespeed::PagespeedInput> input;
  if (in_format == "har") {
    input.reset(pagespeed::ParseHttpArchive(file_contents));
  } else if (in_format == "proto") {
    input.reset(ParseProtoInput(file_contents));
  } else {
    fprintf(stderr, "Invalid input format %s\n", in_format.c_str());
    PrintUsage();
    return 1;
  }
  CHECK(input.get() != NULL);

  std::vector<pagespeed::Rule*> rules;
  bool save_optimized_content = true;
  pagespeed::rule_provider::AppendAllRules(save_optimized_content, &rules);

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(rules);
  engine.Init();

  engine.ComputeAndFormatResults(*input.get(), formatter.get());

  return 0;
}
