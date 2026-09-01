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

#ifndef VERIBLE_VERILOG_ANALYSIS_VERILOG_LINTER_H_
#define VERIBLE_VERILOG_ANALYSIS_VERILOG_LINTER_H_

#include <iosfwd>
#include <set>
#include <string_view>
#include <vector>

#include "absl/flags/declare.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "verible/common/analysis/line-linter.h"
#include "verible/common/analysis/lint-rule-status.h"
#include "verible/common/analysis/lint-waiver.h"
#include "verible/common/analysis/syntax-tree-linter.h"
#include "verible/common/analysis/text-structure-linter.h"
#include "verible/common/analysis/token-stream-linter.h"
#include "verible/common/analysis/violation-handler.h"
#include "verible/common/strings/line-column-map.h"
#include "verible/common/text/text-structure.h"
#include "verible/verilog/analysis/lint-rule-registry.h"
#include "verible/verilog/analysis/verilog-linter-configuration.h"

// Flag is declared for testing purposes (used e.g. in
// verilog/tools/ls/verilog-language-server_test.cc)
ABSL_DECLARE_FLAG(bool, rules_config_search);

namespace verilog {

// Returns violations from multiple `LintRuleStatus`es sorted by position
// of their occurrence in source code.
std::set<verible::LintViolationWithStatus> GetSortedViolations(
    const std::vector<verible::LintRuleStatus> &statuses);

// Options controlling how LintOneFile() analyzes one file and reports on it.
struct LintOneFileOptions {
  // Report lexical and syntax errors found while analyzing the file.
  bool check_syntax = true;

  // Stop after syntax errors rather than linting the salvaged syntax tree.
  bool parse_fatal = true;

  // Return nonzero when lint violations are found.
  bool lint_fatal = true;

  // Quote the offending source line underneath each diagnostic.
  bool show_context = false;
};

// Checks a single file for Verilog style lint violations.
// This is suitable for calling from main().
// 'stream' is used for printing potential syntax errors.
// 'filename' is the path to the file to analyze.
// 'config' controls lint rules for analysis.
// 'violation_handler' controls what to do with violations.
// 'options' selects syntax checking and fatality; see LintOneFileOptions.
// Returns an exit_code like status where 0 means success, 1 means some
// errors were found (syntax, lint), and anything else is a fatal error.
int LintOneFile(std::ostream *stream, std::string_view filename,
                const LinterConfiguration &config,
                verible::ViolationHandler *violation_handler,
                const LintOneFileOptions &options);

// VerilogLinter analyzes a TextStructureView of Verilog source code.
// This uses syntax-tree based analyses and lexical token-stream analyses.
class VerilogLinter {
 public:
  VerilogLinter();

  // Configures the internal linters, enabling select rules.
  absl::Status Configure(const LinterConfiguration &configuration,
                         std::string_view lintee_filename);

  // Analyzes text structure.
  void Lint(const verible::TextStructureView &text_structure,
            std::string_view filename);

  // Reports lint findings.
  std::vector<verible::LintRuleStatus> ReportStatus(
      const verible::LineColumnMap &, std::string_view text_base);

 private:
  // Line based linter.
  verible::LineLinter line_linter_;

  // Token-based linter.
  verible::TokenStreamLinter token_stream_linter_;

  // Syntax-tree based linter.
  verible::SyntaxTreeLinter syntax_tree_linter_;

  // TextStructure-based linter.
  verible::TextStructureLinter text_structure_linter_;

  // Tracks the set of waived lines per rule.
  verible::LintWaiverBuilder lint_waiver_;
};

// Creates a linter configuration from global flags.
// If --rules_config_search is configured, uses the given
// start file to look up the directory chain.
absl::StatusOr<LinterConfiguration> LinterConfigurationFromFlags(
    std::string_view linting_start_file = ".");

// Expands linter configuration from a text file
absl::Status AppendLinterConfigurationFromFile(
    LinterConfiguration *config, std::string_view config_filename);

// VerilogLintTextStructure analyzes Verilog syntax tree for style violations
// and syntactically detectable pitfalls.
//
// The configuration of this function is controlled by flags:
//   FLAGS_ruleset, FLAGS_rules
//
// Args:
//   stream: the output stream where diagnostics are captured.
//     Writing anything to this stream means that the input contains
//     some lint violation.
//   filename: (optional) name of input file, that can appear in logs.
//   text_structure: contains the syntax tree that will be lint-analyzed.
//   show_context: print additional line with vulnerable code
//
// Returns:
//   Vector of LintRuleStatuses on success, otherwise error code.
absl::StatusOr<std::vector<verible::LintRuleStatus>> VerilogLintTextStructure(
    std::string_view filename, const LinterConfiguration &config,
    const verible::TextStructureView &text_structure);

// Prints the rule, description and default_enabled.
absl::Status PrintRuleInfo(std::ostream *,
                           const analysis::LintRuleDescriptionsMap &,
                           std::string_view);

// Outputs the descriptions for every rule for the --help_rules flag.
// TODO(sconwayaus): These are really printers and not getters. Consider
// renaming
void GetLintRuleDescriptionsHelpFlag(std::ostream *, std::string_view);

// Outputs the descriptions for every rule, formatted for markdown.
// TODO(sconwayaus): These are really printers and not getters. Consider
// renaming
void GetLintRuleDescriptionsMarkdown(std::ostream *);

// Outputs the default linting rules in a format suitable to produce a
// .rules.verible_lint file
// TODO(sconwayaus): These are really printers and not getters. Consider
// renaming
void GetLintRuleFile(std::ostream *os, const LinterConfiguration &config);

}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_VERILOG_LINTER_H_
