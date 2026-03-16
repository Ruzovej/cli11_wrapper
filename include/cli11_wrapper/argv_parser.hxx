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

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace CLI {
class App;
class Option;
class Error;
} // namespace CLI

namespace cli11_wrapper {

struct argv_parser {
  explicit argv_parser(std::string &&app_desc, std::string &&app_name,
                       std::vector<std::string> &&config_names, int const aArgc,
                       char **const aArgv);

  argv_parser(argv_parser &&) noexcept;
  argv_parser &operator=(argv_parser &&) noexcept;

  ~argv_parser();

  CLI::Option *add_option(std::string &&name, int &value,
                          std::string &&desc = "");
  CLI::Option *add_option(std::string &&name, long long &value,
                          std::string &&desc = "");
  CLI::Option *add_option(std::string &&name, double &value,
                          std::string &&desc = "");
  CLI::Option *add_option(std::string &&name, std::string &value,
                          std::string &&desc = "");
  CLI::Option *add_option(std::string &&name, std::vector<std::string> &value,
                          std::string &&desc = "");

  CLI::Option *add_flag(std::string &&name, bool &flag,
                        std::string &&desc = "");

  void failure_message(
      std::function<std::string(const CLI::App *, const CLI::Error &e)>
          &&msg_sink_fn);

  void reset_argc_argv(int const aArgc, char **const aArgv);

  void set_allow_extras(bool const allow);

  void set_allow_config_extras(bool const allow);

  [[nodiscard]] std::vector<std::string> get_parsed_extras() const;

  // TODO wrap those 2 below properly, so it's not needed to inlcude CLI/CLI.hpp
  // to use them, etc.:

  // call it this way (in `main`, etc.):
  // ```
  // CLI11_PARSE(argv_parser_instance);
  // ```
  void parse();

  [[nodiscard]] int exit(CLI::Error const &e, std::ostream &out = std::cout,
                         std::ostream &err = std::cerr);

private:
  argv_parser(const argv_parser &) = delete;
  argv_parser &operator=(const argv_parser &) = delete;

  std::unique_ptr<CLI::App> app;
  std::vector<std::string> config_names;
  int argc;
  char **argv; // non-owned
};

} // namespace cli11_wrapper
