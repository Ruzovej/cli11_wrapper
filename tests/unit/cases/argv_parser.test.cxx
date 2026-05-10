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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

#include <doctest/doctest.h>

namespace {

std::string_view constexpr app_name{"myapp1"};
std::string_view constexpr app_desc{"myapp1 description"};

[[nodiscard]] cli11_wrapper::argv_parser
make_semi_initialized_parser(std::vector<std::string> &&config_names) {
  return cli11_wrapper::argv_parser{std::string{app_desc},
                                    std::string{app_name},
                                    std::move(config_names), 0, nullptr};
}

[[nodiscard]] cli11_wrapper::argv_parser
make_parser(std::vector<std::string> &&config_names,
            std::ostringstream &err_msg_sink) {
  auto parser{make_semi_initialized_parser(std::move(config_names))};

  parser.set_error_message_sink(err_msg_sink);

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

  explicit tmp_file(std::string_view const content)
      : tmp_file{std::tmpnam(nullptr), content} {
    // force 2 lines
  }

  ~tmp_file() {
    // force 2 lines
    std::filesystem::remove(path);
  }

  std::filesystem::path const &get_path() const {
    // force 2 lines
    return path;
  }

private:
  tmp_file(tmp_file const &) = delete;
  tmp_file &operator=(tmp_file const &) = delete;
  tmp_file(tmp_file &&) = delete;
  tmp_file &operator=(tmp_file &&) = delete;

  std::filesystem::path path;
};

[[nodiscard]] int call_parse_like_in_main(cli11_wrapper::argv_parser &parser) {
  // TBH, `return parser.do_parse();` would be exactly the same ...:
  CLI11_WRAPPER_PARSE(parser);

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
  std::ostringstream err_msg_sink;

  SUBCASE("flags") {
    SUBCASE("no configs") {
      auto parser{make_parser({}, err_msg_sink)};

      SUBCASE("nothing specified, etc.") {
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

          REQUIRE_EQ(err_msg_sink.str(),
                     std::string_view{
                         "The following argument was not expected: --foo\n"});

          err_msg_sink.str("");
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

      SUBCASE("no env.") {
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

        SUBCASE("--flag --flag ... repeated flags") {
          auto const [argc,
                      argv]{build_argc_argv(app_name, {"--flag", "--flag"})};

          parser.reset_argc_argv(argc, argv.get());

          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE(flag);
          REQUIRE_FALSE(option);
        }

        SUBCASE("--flag --no-flag ... flag countering itself") {
          auto const [argc,
                      argv]{build_argc_argv(app_name, {"--flag", "--no-flag"})};

          parser.reset_argc_argv(argc, argv.get());

          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_FALSE(flag);
          REQUIRE_FALSE(option);
        }

        SUBCASE("--no-flag --flag ... flag overwriting itself") {
          auto const [argc,
                      argv]{build_argc_argv(app_name, {"--no-flag", "--flag"})};

          parser.reset_argc_argv(argc, argv.get());

          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE(flag);
          REQUIRE_FALSE(option);
        }
      }

      SUBCASE("with env.") {
        bool flag = false;
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FLAG"},
            "-f,--flag,!--no-flag", flag, "flag desc.");

        SUBCASE("no args ... using value 1 from env.") {
          auto const [argc, argv]{build_argc_argv(app_name, {})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_FLAG", "1"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE(flag);
        }

        SUBCASE("no args ... using value 0 from env.") {
          auto const [argc, argv]{build_argc_argv(app_name, {})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_FLAG", "0"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_FALSE(flag);
        }

        SUBCASE("--flag ... flag overwriting env.") {
          auto const [argc, argv]{build_argc_argv(app_name, {"--flag"})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_FLAG", "0"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE(flag);
        }

        SUBCASE("--no-flag ... overwriting env.") {
          auto const [argc, argv]{build_argc_argv(app_name, {"--no-flag"})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_FLAG", "1"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_FALSE(flag);
        }
      }
    }

    SUBCASE("with configs") {
      SUBCASE("single config") {
        static std::string_view constexpr config_content{R"(
        # comment line
        flag = true
        option = false
        whatever_else = 42
        )"};
        tmp_file const config{config_content};

        auto parser{make_parser({config.get_path().string()}, err_msg_sink)};

        bool flag = false;
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FLAG"},
            "-f,--flag,!--no-flag", flag, "flag desc.");

        bool option = false;
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_OPTION"},
            "-o,--option,!--no-option", option, "option desc.");

        SUBCASE("forbidden config extras") {
          auto const [argc, argv]{build_argc_argv(app_name, {})};

          parser.reset_argc_argv(argc, argv.get());
          parser.set_allow_config_extras(false);

          // highlighting: `..._NE...` ~ not equal:
          REQUIRE_NE(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(
              err_msg_sink.str(),
              std::string_view{"INI was not able to parse whatever_else\n"});

          err_msg_sink.str("");
        }

        SUBCASE("no args") {
          SUBCASE("no env.") {
            auto const [argc, argv]{build_argc_argv(app_name, {})};

            parser.reset_argc_argv(argc, argv.get());

            REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

            REQUIRE(flag);
            REQUIRE_FALSE(option);
          }

          SUBCASE("with env.") {
            auto const [argc, argv]{build_argc_argv(app_name, {})};

            parser.reset_argc_argv(argc, argv.get());

            scoped_env_var env_var1{"CLI11WRAPPERUNITTEST_FLAG", "0"};
            scoped_env_var env_var2{"CLI11WRAPPERUNITTEST_OPTION", "1"};

            REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

            REQUIRE_FALSE(flag);
            REQUIRE(option);
          }
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

          REQUIRE(flag);
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

          REQUIRE(flag);
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
      }

      SUBCASE("more configs") {
        static std::string_view constexpr config_content1{R"(
        # comment line
        conf1 = true
        conf2 = false
        conf3 = false
        conf4 = false
        conf5 = false
        )"};
        tmp_file const config1{config_content1};

        static std::string_view constexpr config_content2{R"(
        # comment line
        conf2 = true
        conf3 = false
        conf4 = false
        conf5 = false
        )"};
        tmp_file const config2{config_content2};

        static std::string_view constexpr config_content3{R"(
        # comment line
        conf3 = true
        conf4 = false
        conf5 = false
        )"};
        tmp_file const config3{config_content3};

        scoped_env_var env_var{"CLI11WRAPPERUNITTEST_CONF4", "1"};

        auto parser{make_parser({config1.get_path().string(),
                                 config2.get_path().string(),
                                 config3.get_path().string()},
                                err_msg_sink)};

        parser.set_allow_config_extras(false);

        std::array<bool, 5> flags{false, false, false, false, false};
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_CONF1"},
            "--conf1,!--no-conf1", flags[0], "conf1 desc.");
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_CONF2"},
            "--conf2,!--no-conf2", flags[1], "conf2 desc.");
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_CONF3"},
            "--conf3,!--no-conf3", flags[2], "conf3 desc.");
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_CONF4"},
            "--conf4,!--no-conf4", flags[3], "conf4 desc.");
        parser.add_flag(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_CONF5"},
            "--conf5,!--no-conf5", flags[4], "conf5 desc.");

        auto const [argc, argv]{build_argc_argv(app_name, {"--conf5"})};

        parser.reset_argc_argv(argc, argv.get());

        REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

        REQUIRE(flags[0]);
        REQUIRE(flags[1]);
        REQUIRE(flags[2]);
        REQUIRE(flags[3]);
        REQUIRE(flags[4]);
      }
    }
  }

  SUBCASE("options") {
    SUBCASE("int") {
      SUBCASE("no configs") {
        auto parser{make_parser({}, err_msg_sink)};

        int option = -1;
        parser.add_option(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_OPTION"},
            "-o,--option", option, "option desc.");

        SUBCASE("default value") {
          auto const [argc, argv]{build_argc_argv(app_name, {})};

          parser.reset_argc_argv(argc, argv.get());

          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(option, -1);
        }

        SUBCASE("--option 42") {
          auto const [argc,
                      argv]{build_argc_argv(app_name, {"--option", "42"})};

          parser.reset_argc_argv(argc, argv.get());

          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(option, 42);
        }

        SUBCASE("env. alone") {
          auto const [argc, argv]{build_argc_argv(app_name, {})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_OPTION", "42"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(option, 42);
        }

        SUBCASE("--option 42, overwriting env.") {
          auto const [argc,
                      argv]{build_argc_argv(app_name, {"--option", "42"})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_OPTION", "666"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(option, 42);
        }
      }

      SUBCASE("with configs") {
        static std::string_view constexpr config_content{R"(
        # comment line
        option = 1
        )"};
        tmp_file const config{config_content};

        auto parser{make_parser({config.get_path().string()}, err_msg_sink)};

        int option = -1;
        parser.add_option(
            cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_OPTION"},
            "-o,--option", option, "option desc.");

        SUBCASE("value from config") {
          auto const [argc, argv]{build_argc_argv(app_name, {})};

          parser.reset_argc_argv(argc, argv.get());

          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(option, 1);
        }

        SUBCASE("value from env. overwriting config") {
          auto const [argc, argv]{build_argc_argv(app_name, {})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_OPTION", "2"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(option, 2);
        }

        SUBCASE("value from argv") {
          auto const [argc, argv]{build_argc_argv(app_name, {"--option", "3"})};

          parser.reset_argc_argv(argc, argv.get());

          scoped_env_var env_var{"CLI11WRAPPERUNITTEST_OPTION", "2"};
          REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

          REQUIRE_EQ(option, 3);
        }
      }
    }

    // ... all is similar in the end, so only briefly:

    SUBCASE("long long") {
      auto parser{make_parser({}, err_msg_sink)};

      long long option_ll = -1;
      parser.add_option("-l,--option-ll", option_ll, "option desc.");

      auto const [argc, argv]{build_argc_argv(
          app_name, {"--option-ll", "8000000000"})}; // `int` would overflow

      parser.reset_argc_argv(argc, argv.get());

      REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

      REQUIRE_EQ(option_ll, 8'000'000'000LL);
    }

    SUBCASE("double") {
      auto parser{make_parser({}, err_msg_sink)};

      double option_dp = -1;
      parser.add_option("-d,--option-dp", option_dp, "option desc.");

      auto const [argc,
                  argv]{build_argc_argv(app_name, {"--option-dp", "123.456"})};

      parser.reset_argc_argv(argc, argv.get());

      REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

      REQUIRE_EQ(option_dp, 123.456);
    }

    SUBCASE("string") {
      auto parser{make_parser({}, err_msg_sink)};

      std::string option_str;
      parser.add_option("-s,--option-str", option_str, "option desc.");

      static std::string_view constexpr expected_str{
          "Hello, how are You? I'm fine, thanks for asking."};

      auto const [argc, argv]{build_argc_argv(
          app_name, {"--option-str", std::string{expected_str}})};

      parser.reset_argc_argv(argc, argv.get());

      REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

      REQUIRE_EQ(option_str, expected_str);
    }

    SUBCASE("strings") {
      auto parser{make_parser({}, err_msg_sink)};

      std::vector<std::string> option_strs;
      parser.add_option("-s,--strs", option_strs, "option desc.");

      static std::string_view constexpr expected_str{
          "Hello, how are You? I'm fine, thanks for asking."};

      auto const [argc, argv]{
          build_argc_argv(app_name, {"--strs", std::string{expected_str},
                                     "some", "extra", "args"})};

      parser.reset_argc_argv(argc, argv.get());

      REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

      REQUIRE_EQ(option_strs.size(), 4);
      REQUIRE_EQ(option_strs[0], expected_str);
      REQUIRE_EQ(option_strs[1], "some");
      REQUIRE_EQ(option_strs[2], "extra");
      REQUIRE_EQ(option_strs[3], "args");
    }
  }

  SUBCASE("all at once") {
    static std::string_view constexpr config_content1{R"(
      # comment line
      str2 = conf1_string2
      flag1 = true
      flag2 = false
      str1 = "Hello world!"
      fp2 = 5.1
      flag3 = false
      )"};
    tmp_file const config1{config_content1};

    static std::string_view constexpr config_content2{R"(
      # comment line
      flag2 = true
      int2 = 666
      fp1 = 2.718281828
      fp2 = 5.2
      int1 = 42
      )"};
    tmp_file const config2{config_content2};

    static std::string_view constexpr config_content3{R"(
      # comment line
      flag1 = false
      # `int` would overflow, `long long` is needed:
      ll1 = 8000000000
      fp2 = 5.3
      fp1 = 3.141529
      str2 = conf3_string2
      array1 = ["a1", "a2", "a3"]
      )"};
    tmp_file const config3{config_content3};

    auto parser{
        make_parser({config1.get_path().string(), config2.get_path().string(),
                     config3.get_path().string()},
                    err_msg_sink)};

    bool flag1 = false;
    parser.add_flag(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FLAG1"},
                    "--flag1,!--no-flag1", flag1, "flag1 desc.");

    bool flag2 = false;
    parser.add_flag(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FLAG2"},
                    "--flag2,!--no-flag2", flag2, "flag2 desc.");

    bool flag3 = true;
    parser.add_flag(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FLAG3"},
                    "--flag3,!--no-flag3", flag3, "flag3 desc.");

    bool flag4 = true;
    parser.add_flag(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FLAG4"},
                    "--flag4,!--no-flag4", flag4, "flag4 desc.");

    int int1 = -1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_INT1"},
                      "--int1", int1, "int1 desc.");

    int int2 = -1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_INT2"},
                      "--int2", int2, "int2 desc.");

    long long ll1 = -1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_LL1"},
                      "--ll1", ll1, "ll1 desc.");

    long long ll2 = -1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_LL2"},
                      "--ll2", ll2, "ll2 desc.");

    double fp1 = -1.1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FP1"},
                      "--fp1", fp1, "fp1 desc.");

    double fp2 = -1.1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FP2"},
                      "--fp2", fp2, "fp2 desc.");

    double fp3 = -1.1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_FP3"},
                      "--fp3", fp3, "fp3 desc.");

    std::string str1;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_STR1"},
                      "--str1", str1, "str1 desc.");

    std::string str2;
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_STR2"},
                      "--str2", str2, "str2 desc.");

    std::string str3{"default str3"};
    parser.add_option(cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_STR3"},
                      "--str3", str3, "str3 desc.");

    std::vector<std::string> array1;
    parser.add_option(
        cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_ARRAY1"}, "--array1",
        array1, "array1 desc.");

    std::vector<std::string> array2;
    parser.add_option(
        cli11_wrapper::env_var_name{"CLI11WRAPPERUNITTEST_ARRAY2"}, "--array2",
        array2, "array2 desc.");

    parser.set_allow_config_extras(true);

    auto const [argc, argv]{build_argc_argv(
        app_name,
        {"--array2", "Hello, how are You? I'm fine, thanks for asking.", "some",
         "extra", "args", "--fp3", "18.26", "--no-flag2", "--flag4", "--fp2",
         "5.5", "tail_extra_1", "tail_extra_2", "...", "tail_extra_n"})};

    parser.reset_argc_argv(argc, argv.get());

    scoped_env_var env_var1{"CLI11WRAPPERUNITTEST_FLAG4", "0"};
    scoped_env_var env_var2{"CLI11WRAPPERUNITTEST_INT2", "-100"};
    scoped_env_var env_var3{"CLI11WRAPPERUNITTEST_LL2", "9000000000"};
    scoped_env_var env_var4{"CLI11WRAPPERUNITTEST_FP2", "5.4"};
    scoped_env_var env_var5{"CLI11WRAPPERUNITTEST_STR1",
                            "Hello amazing world ...!"};

    REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

    REQUIRE_FALSE(flag1); // conf1 < conf3
    REQUIRE_FALSE(flag2); // conf1 < conf2 < cli
    REQUIRE_FALSE(flag3); // conf1
    REQUIRE(flag4);       // env. < cli

    REQUIRE_EQ(int1, 42);   // conf2
    REQUIRE_EQ(int2, -100); // conf2 < env.

    REQUIRE_EQ(ll1, 8'000'000'000LL); // conf3
    REQUIRE_EQ(ll2, 9'000'000'000LL); // env.

    REQUIRE_EQ(fp1, 3.141529); // conf2 < conf3
    REQUIRE_EQ(fp2, 5.5);      // conf1 < conf2 < conf3 < env. < cli
    REQUIRE_EQ(fp3, 18.26);    // cli

    REQUIRE_EQ(str1, "Hello amazing world ...!"); // conf1 < env.
    REQUIRE_EQ(str2, "conf3_string2");            // conf1 < conf3
    REQUIRE_EQ(str3, "default str3");             // default value

    REQUIRE_EQ(array1, std::vector<std::string>{"a1", "a2", "a3"}); // conf3
    REQUIRE_EQ(array2, std::vector<std::string>{
                           "Hello, how are You? I'm fine, thanks for asking.",
                           "some", "extra", "args"}); // cli

    REQUIRE_EQ(parser.get_parsed_extras(),
               std::vector<std::string>{"tail_extra_1", "tail_extra_2", "...",
                                        "tail_extra_n"}); // cli
  }

  SUBCASE("parsed extras") {
    auto parser{make_parser({}, err_msg_sink)};

    std::string foo{"Ignored, whatever ..."};
    parser.add_option("-f,--foo", foo, "foo desc.");

    parser.set_allow_extras(true);

    SUBCASE("none") {
      auto const [argc, argv]{build_argc_argv(app_name, {"--foo", "bar"})};

      parser.reset_argc_argv(argc, argv.get());

      REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

      REQUIRE_EQ(parser.get_parsed_extras(), std::vector<std::string>{});

      REQUIRE_EQ(foo, "bar");

      auto const extras{parser.get_parsed_extras_c_like()};

      REQUIRE_EQ(extras.argc(), 0);
      REQUIRE_EQ(extras.argv()[0], app_name);
      REQUIRE_EQ(extras.argv()[1], nullptr);
    }

    SUBCASE("some") {
      auto const [argc, argv]{
          build_argc_argv(app_name, {"--foo", "bar", "baz", "qux"})};

      parser.reset_argc_argv(argc, argv.get());

      REQUIRE_EQ(call_parse_like_in_main(parser), EXIT_SUCCESS);

      REQUIRE_EQ(parser.get_parsed_extras(),
                 std::vector<std::string>{"baz", "qux"});

      REQUIRE_EQ(foo, "bar");

      auto const extras{parser.get_parsed_extras_c_like()};

      REQUIRE_EQ(extras.argc(), 2);
      REQUIRE_EQ(extras.argv()[0], app_name);
      REQUIRE_EQ(extras.argv()[1], std::string_view{"baz"});
      REQUIRE_EQ(extras.argv()[2], std::string_view{"qux"});
      REQUIRE_EQ(extras.argv()[3], nullptr);
    }
  }

  REQUIRE(err_msg_sink.str().empty());
}

} // namespace
