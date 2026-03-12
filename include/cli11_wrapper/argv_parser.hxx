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

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <CLI/CLI.hpp>

namespace cli11_wrapper {

struct argv_parser {
  explicit argv_parser(std::string &&app_desc, std::string &&app_name,
                       std::vector<std::string_view> &&config_names,
                       int const aArgc, char **const aArgv);

  template <typename... Args> auto add_option(Args &&...args) {
    return app.add_option(std::forward<Args>(args)...);
  }

  template <typename... Args> auto add_flag(Args &&...args) {
    return app.add_flag(std::forward<Args>(args)...);
  }

  // call it this way (in `main`, etc.): 
  // ```
  // CLI11_PARSE(argv_parser_instance);
  // ```
  void parse();

private:
  argv_parser(const argv_parser &) = delete;
  argv_parser &operator=(const argv_parser &) = delete;
  argv_parser(argv_parser &&) = delete;
  argv_parser &operator=(argv_parser &&) = delete;

  CLI::App app;
  std::vector<std::string_view> config_names;
  int argc;
  char **argv;
};

} // namespace cli11_wrapper
