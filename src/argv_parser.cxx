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

#include <cassert>
#include <cstdlib>

#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <CLI/CLI.hpp>

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
                         std::vector<std::string> &&config_names,
                         int const aArgc, char **const aArgv)
    : app{std::make_unique<CLI::App>(std::move(app_desc), std::move(app_name))},
      config_names{std::move(config_names)}, argc{aArgc}, argv{aArgv} {
  assert(app != nullptr);
  // explicit --config from CLI (highest-priority config file)
  app->set_config("--config")->expected(0, 1)->check(CLI::ExistingFile);
}

argv_parser::argv_parser(argv_parser &&) noexcept = default;

argv_parser &argv_parser::operator=(argv_parser &&) noexcept = default;

argv_parser::~argv_parser() = default;

CLI::Option *argv_parser::add_option(std::string &&name, int &value,
                                     std::string &&desc) {
  return app->add_option(std::move(name), value, std::move(desc));
}

CLI::Option *argv_parser::add_option(std::string &&name, long long &value,
                                     std::string &&desc) {
  return app->add_option(std::move(name), value, std::move(desc));
}

CLI::Option *argv_parser::add_option(std::string &&name, double &value,
                                     std::string &&desc) {
  return app->add_option(std::move(name), value, std::move(desc));
}

CLI::Option *argv_parser::add_option(std::string &&name, std::string &value,
                                     std::string &&desc) {
  return app->add_option(std::move(name), value, std::move(desc));
}

CLI::Option *argv_parser::add_option(std::string &&name,
                                     std::vector<std::string> &value,
                                     std::string &&desc) {
  return app->add_option(std::move(name), value, std::move(desc));
}

CLI::Option *argv_parser::add_flag(std::string &&name, bool &flag,
                                   std::string &&desc) {
  return app->add_flag(std::move(name), flag, std::move(desc));
}

void argv_parser::failure_message(
    std::function<std::string(const CLI::App *, const CLI::Error &e)>
        &&msg_sink_fn) {
  app->failure_message(std::move(msg_sink_fn));
}

void argv_parser::reset_argc_argv(int const aArgc, char **const aArgv) {
  argc = aArgc;
  argv = aArgv;
}

void argv_parser::set_allow_extras(bool const allow) {
  // force 2 lines
  app->allow_extras(allow);
}

void argv_parser::set_allow_config_extras(bool const allow) {
  // force 2 lines
  app->allow_config_extras(allow);
}

std::vector<std::string> argv_parser::get_parsed_extras() const {
  // force 2 lines
  return app->remaining();
}

void argv_parser::parse() {
  toml_merger confs;

  for (auto const &conf_name : config_names) {
    confs.load(conf_name);
  }

  if (!confs.empty()) {
    auto ss{confs.to_toml_ss()};

    app->parse_from_stream(ss);
  }

  app->parse(argc, argv);
}

int argv_parser::exit(CLI::Error const &e, std::ostream &out,
                      std::ostream &err) {
  return app->exit(e, out, err);
}

} // namespace cli11_wrapper
