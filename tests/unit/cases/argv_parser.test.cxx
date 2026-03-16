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
#include <optional>
#include <string_view>

#include <CLI/CLI.hpp>

#include <doctest/doctest.h>

namespace {

[[nodiscard]] cli11_wrapper::argv_parser
make_semi_initialized_parser(std::vector<std::string> &&config_names) {
  static std::string_view constexpr app_name{"myapp1"};
  static std::string_view constexpr app_desc{"myapp1 description"};

  return cli11_wrapper::argv_parser{std::string{app_desc},
                                    std::string{app_name},
                                    std::move(config_names), 0, nullptr};
}

[[nodiscard]] cli11_wrapper::argv_parser
make_parser(std::vector<std::string> &&config_names,
            std::string &error_msg_buffer) {
  auto parser{make_semi_initialized_parser(std::move(config_names))};

  parser.failure_message(
      [&error_msg_buffer]([[maybe_unused]] CLI::App const *const app,
                          CLI::Error const &e) -> std::string {
        error_msg_buffer = e.what();
        return {};
      });

  return parser;
}

struct scoped_env_var {
  explicit scoped_env_var(std::string &&aName, std::string const &val)
      : name{std::move(aName)} {
    auto const cur{std::getenv(name.c_str())};

    if (cur != nullptr) {
      former_val.emplace(cur);
    }

    if (setenv(name.c_str(), val.c_str(), 1) != 0) {
      throw std::runtime_error{"Failed to set env var: " + name};
    }
  }

  ~scoped_env_var() {
    if (former_val) {
      setenv(name.c_str(), former_val->c_str(), 1);
    } else {
      unsetenv(name.c_str());
    }
  }

private:
  scoped_env_var(scoped_env_var const &) = delete;
  scoped_env_var &operator=(scoped_env_var const &) = delete;
  scoped_env_var(scoped_env_var &&) = delete;
  scoped_env_var &operator=(scoped_env_var &&) = delete;

  std::string name;
  std::optional<std::string> former_val;
};

struct tmp_file {
  explicit tmp_file(std::filesystem::path &&aPath,
                    std::string_view const content)
      : path{std::move(aPath)} {
    if (path.empty()) {
      throw std::runtime_error{"Empty path"};
    }

    if (std::filesystem::exists(path)) {
      throw std::runtime_error{"File already exists: " + path.string()};
    }

    std::ofstream{path} << content;
  }

  ~tmp_file() {
    // force 2 lines
    std::filesystem::remove(path);
  }

private:
  tmp_file(tmp_file const &) = delete;
  tmp_file &operator=(tmp_file const &) = delete;
  tmp_file(tmp_file &&) = delete;
  tmp_file &operator=(tmp_file &&) = delete;

  std::filesystem::path path;
};

// TODO maybe move this in the class as its method?!
[[nodiscard]] int call_parse_like_in_main(cli11_wrapper::argv_parser &parser) {
  CLI11_PARSE(parser);

  return EXIT_SUCCESS;
}

struct my_cstr_arr_deleter {
  void operator()(char *null_terminated_arr[]) const {
    if (null_terminated_arr != nullptr) {
      for (char **ptr = null_terminated_arr; *ptr != nullptr; ++ptr) {
        std::free(*ptr); // due to `strndup`
      }
      delete[] null_terminated_arr;
    }
  }
};

[[nodiscard]] std::pair<int, std::unique_ptr<char *[], my_cstr_arr_deleter>>
build_argc_argv(std::string_view const path,
                std::vector<std::string_view> const &args) {
  auto const num_args{args.size()};

  std::unique_ptr<char *[], my_cstr_arr_deleter> args_cstr_arr {
    new char *[num_args + 2], {}
  };

  // https://man7.org/linux/man-pages/man3/exec.3.html -> "The first
  // argument, by convention, should point to the filename associated with
  // the file being executed"
  args_cstr_arr[0] = strndup(path.data(), path.size());
  for (std::size_t i{0}; i < num_args; ++i) {
    args_cstr_arr[i + 1] = strndup(args[i].data(), args[i].size());
  }
  args_cstr_arr[num_args + 1] = nullptr;

  return {static_cast<int>(num_args + 1), std::move(args_cstr_arr)};
}

TEST_CASE("argv_parser") {
  static std::string_view constexpr app_name{"myapp1"};
  static std::string_view constexpr app_desc{"myapp1"};

  SUBCASE("no configs") {
    std::string error_msg_buffer;
    auto parser{make_parser({}, error_msg_buffer)};

    SUBCASE("no flags, etc.") {
      SUBCASE("empty") {
        auto const [argc, argv]{build_argc_argv(app_name, {})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);
      }

      SUBCASE("some unknown arg(s), error") {
        auto const [argc, argv]{build_argc_argv(app_name, {"--foo"})};

        parser.reset_argc_argv(argc, argv.get());

        // highlighting: `..._NE...` ~ not equal:
        REQUIRE_NE(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE_EQ(parser.get_parsed_extras(),
                   std::vector<std::string>{"--foo"});

        REQUIRE_EQ(
            error_msg_buffer,
            std::string_view{"The following argument was not expected: --foo"});
      }

      SUBCASE("some unknown arg(s), allowed") {
        parser.set_allow_extras(true);

        auto const [argc, argv]{build_argc_argv(app_name, {"--foo"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE_EQ(parser.get_parsed_extras(),
                   std::vector<std::string>{"--foo"});
      }
    }

    SUBCASE("flags") {
      bool flag = false;
      parser.add_flag("-f,--flag,!--no-flag", flag, "flag desc.");

      bool option = false;
      parser.add_flag("-o,--option,!--no-option", option, "option desc.");

      SUBCASE("no args") {
        auto const [argc, argv]{build_argc_argv(app_name, {})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE_FALSE(flag);
        REQUIRE_FALSE(option);
      }

      SUBCASE("--flag") {
        auto const [argc, argv]{build_argc_argv(app_name, {"--flag"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE(flag);
        REQUIRE_FALSE(option);
      }

      SUBCASE("--option") {
        auto const [argc, argv]{build_argc_argv(app_name, {"--option"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE_FALSE(flag);
        REQUIRE(option);
      }

      SUBCASE("--no-flag") {
        auto const [argc, argv]{build_argc_argv(app_name, {"--no-flag"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE_FALSE(flag);
        REQUIRE_FALSE(option);
      }

      SUBCASE("--no-option") {
        auto const [argc, argv]{build_argc_argv(app_name, {"--no-option"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE_FALSE(flag);
        REQUIRE_FALSE(option);
      }

      SUBCASE("--flag --option") {
        auto const [argc,
                    argv]{build_argc_argv(app_name, {"--flag", "--option"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE(flag);
        REQUIRE(option);
      }

      SUBCASE("-fo ... abbreviated flags") {
        auto const [argc, argv]{build_argc_argv(app_name, {"-fo"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE(flag);
        REQUIRE(option);
      }
    }
  }
}

} // namespace
