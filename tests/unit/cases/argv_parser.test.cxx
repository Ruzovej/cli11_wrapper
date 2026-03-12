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

#include <doctest/doctest.h>

namespace {

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

[[nodiscard]] std::unique_ptr<char *[], my_cstr_arr_deleter>
build_args_cstr(std::string_view const path,
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

  return args_cstr_arr;
}

TEST_CASE("argv_parser") {
  SUBCASE("empty") {
    auto const argv{build_args_cstr("myapp1", {})};

    cli11_wrapper::argv_parser parser{"app desc.", "myapp1", {}, 1, argv.get()};

    REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);
  }
}

} // namespace
