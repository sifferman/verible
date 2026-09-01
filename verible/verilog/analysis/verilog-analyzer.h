// Copyright 2017-2020 The Verible Authors.
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

#ifndef VERIBLE_VERILOG_ANALYSIS_VERILOG_ANALYZER_H_
#define VERIBLE_VERILOG_ANALYSIS_VERILOG_ANALYZER_H_

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

#include "absl/status/status.h"
#include "verible/common/analysis/file-analyzer.h"
#include "verible/common/strings/mem-block.h"
#include "verible/common/text/token-stream-view.h"
#include "verible/verilog/analysis/verilog-filelist.h"
#include "verible/verilog/preprocessor/verilog-preprocess.h"

namespace verilog {

// VerilogAnalyzer analyzes Verilog and SystemVerilog code syntax.
class VerilogAnalyzer : public verible::FileAnalyzer {
 public:
  VerilogAnalyzer(std::shared_ptr<verible::MemBlock> text,
                  std::string_view name,
                  const VerilogPreprocess::Config &preprocess_config)
      : verible::FileAnalyzer(std::move(text), name),
        preprocess_config_(preprocess_config) {}

  // Legacy constructor.
  VerilogAnalyzer(std::string_view text, std::string_view name,
                  const VerilogPreprocess::Config &preprocess_config)
      : verible::FileAnalyzer(text, name),
        preprocess_config_(preprocess_config) {}

  // Legacy constructor.
  // TODO(hzeller): Remove once every instantiation sets preprocessor config.
  VerilogAnalyzer(std::string_view text, std::string_view name)
      : VerilogAnalyzer(text, name, VerilogPreprocess::Config()) {}

  VerilogAnalyzer(const VerilogAnalyzer &) = delete;
  VerilogAnalyzer(VerilogAnalyzer &&) = delete;

  // Enables full preprocessing: `include resolution using 'file_opener', and
  // the command-line defines and include directories in '*preprocessing_info'.
  // '*preprocessing_info' must outlive this object. Must be called before
  // Analyze().
  void SetPreprocessing(const FileList::PreprocessingInfo *preprocessing_info,
                        VerilogPreprocess::FileOpener file_opener) {
    preprocessing_info_ = preprocessing_info;
    file_opener_ = std::move(file_opener);
  }

  // Lex-es the input text into tokens.
  absl::Status Tokenize() final;

  // Create token stream view without comments and whitespace.
  // The retained tokens will become leaves of a concrete syntax tree.
  void FilterTokensForSyntaxTree();

  // Analyzes the syntax and structure of a source file (lex and parse).
  // Result of parsing is stored in syntax_tree_, which may contain gaps
  // if there are syntax errors.
  absl::Status Analyze();

  absl::Status LexStatus() const { return lex_status_; }

  absl::Status ParseStatus() const { return parse_status_; }

  size_t MaxUsedStackSize() const { return max_used_stack_size_; }

  // Automatically analyze with the correct parsing mode, as detected
  // by parser directive comments.
  static std::unique_ptr<VerilogAnalyzer> AnalyzeAutomaticMode(
      const std::shared_ptr<verible::MemBlock> &text, std::string_view name,
      const VerilogPreprocess::Config &preprocess_config);

  static std::unique_ptr<VerilogAnalyzer> AnalyzeAutomaticMode(
      std::string_view text, std::string_view name,
      const VerilogPreprocess::Config &preprocess_config);

  // Automatically analyze with correct parsing mode like AnalyzeAutomaticMode()
  // but attempt first with preprocessor disabled to get as complete as
  // possible parse tree; if this yields to syntax errors, fall back to
  // enabling preprocess branches.
  static std::unique_ptr<VerilogAnalyzer> AnalyzeAutomaticPreprocessFallback(
      std::string_view text, std::string_view name);

  const VerilogPreprocessData &PreprocessorData() const {
    return preprocessor_data_;
  }

  // Maybe this belongs in a subclass like VerilogFileAnalyzer?
  // TODO(fangism): Retain a copy of the token stream transformer because it
  // may contain tokens backed by generated text.

 protected:
  // Apply context-based disambiguation of tokens.
  void ContextualizeTokens();

  // Scan comments for parsing mode directives.
  // Returns a string that is first argument of the directive, e.g.:
  //     // verilog_syntax: mode-x
  // results in "mode-x".
  static std::string_view ScanParsingModeDirective(
      const verible::TokenSequence &raw_tokens);

  // Special string inside a comment that triggers setting parsing mode.
  static constexpr std::string_view kParseDirectiveName = "verilog_syntax:";

 private:
  // Attempt to parse all macro arguments as expressions.  Where parsing as an
  // expession succeeds, substitute the leaf with a node with the expression's
  // syntax tree.  If parsing fails, leave the MacroArg token unexpanded.
  void ExpandMacroCallArgExpressions();

  // Information about parser internals.

  // True if input text has already been lexed.
  bool tokenized_ = false;

  // Maximum symbol stack depth.
  size_t max_used_stack_size_ = 0;

  // Preprocessor.
  const VerilogPreprocess::Config preprocess_config_;
  VerilogPreprocessData preprocessor_data_;

  // Include directories and defines for the preprocessor, or nullptr to
  // preprocess without them. Not owned; see SetPreprocessing().
  const FileList::PreprocessingInfo *preprocessing_info_ = nullptr;

  // Opens the files named by `include directives, or empty to leave `include
  // unresolved. See SetPreprocessing().
  VerilogPreprocess::FileOpener file_opener_;

  // The parser's token stream when preprocessing `include's produced tokens
  // from more than one file, which therefore cannot live in TextStructure's
  // own single-file token stream view. Empty otherwise.
  verible::TokenStreamView multi_file_token_stream_;

  // Returns the token stream to hand to the parser.
  const verible::TokenStreamView &TokenStreamToParse();

  // Status of lexing.
  absl::Status lex_status_;

  // Status of parsing.
  absl::Status parse_status_;
};

}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_VERILOG_ANALYZER_H_
