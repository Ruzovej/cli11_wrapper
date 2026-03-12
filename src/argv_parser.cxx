/*
  Copyright 2026 Lukáš Růžička

  This file is part of cli11_wrapper.

  cli11_wrapper is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  cli11_wrapper is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License along
  with cli11_wrapper. If not, see <https://www.gnu.org/licenses/>.
*/

#include "cli11_wrapper/argv_parser.hxx"

#include <cstdlib>

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace cli11_wrapper {

namespace {

std::string_view trim(std::string_view const str) {
  auto const first{str.find_first_not_of(" \t\r\n")};

  if (first == std::string::npos) {
    return {};
  }

  auto const last{str.find_last_not_of(" \t\r\n")};

  return str.substr(first, last - first + 1);
}

// ---------------------------------------------------------------------------
// Tiny TOML key-value merger (good enough for flat config files).
// Reads each file line-by-line; non-comment, non-section lines of the form
//   key = value
// are stored in a map. Later files overwrite earlier keys.
// ---------------------------------------------------------------------------
struct toml_merger {
  void load(std::string const &path) {
    if (!std::filesystem::exists(path)) {
      return;
    }

    std::ifstream input_file{path};
    std::string line;

    while (std::getline(input_file, line)) {
      auto const trimmed_line{trim(line)};

      // skip empty, comments, sections
      if (trimmed_line.empty() || trimmed_line[0] == '#' ||
          trimmed_line[0] == ';' || trimmed_line[0] == '[') {
        continue;
      }

      auto const eq_pos{trimmed_line.find('=')};

      if (eq_pos == std::string::npos) {
        continue;
      }

      std::string key{trim(trimmed_line.substr(0, eq_pos))};
      std::string val{trim(trimmed_line.substr(eq_pos + 1))};

      entries[std::move(key)] = std::move(val);
    }
  }

  /// Produce a merged TOML string that CLI11 can parse as a config
  [[nodiscard]] std::stringstream to_toml_ss() const {
    std::stringstream ss;

    for (auto const &[k, v] : entries) {
      ss << k << " = " << v << "\n";
    }

    return ss;
  }

  [[nodiscard]] bool empty() const { return entries.empty(); }

private:
  std::unordered_map<std::string, std::string> entries;
};

} // namespace

argv_parser::argv_parser(std::string &&app_desc, std::string &&app_name,
                         std::vector<std::string_view> &&config_names,
                         int const aArgc, char **const aArgv)
    : app{std::move(app_desc), std::move(app_name)},
      config_names{std::move(config_names)}, argc{aArgc}, argv{aArgv} {
  // explicit --config from CLI (highest-priority config file)
  app.set_config("--config")->expected(0, 1)->check(CLI::ExistingFile);
}

void argv_parser::parse() {
  toml_merger confs;

  for (auto const conf_name : config_names) {
    confs.load(std::string{conf_name});
  }

  if (!confs.empty()) {
    auto ss{confs.to_toml_ss()};

    app.parse_from_stream(ss);
  }

  app.parse(argc, argv);
}

} // namespace cli11_wrapper
