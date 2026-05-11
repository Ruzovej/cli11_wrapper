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

namespace cli11_wrapper {

struct args {
  explicit args(std::string_view const app_name,
                std::vector<std::string> const &args);

  explicit args(int const aArgc, char **const aArgv) noexcept;

  args(args &&) noexcept;
  args &operator=(args &&) noexcept;

  ~args() noexcept;

  [[nodiscard]] int argc() const {
    // force 2 lines
    return argc_v;
  }
  [[nodiscard]] char **argv() const {
    // force 2 lines
    return argv_v;
  }

private:
  args(args const &) = delete;
  args &operator=(args const &) = delete;

  bool owned{true};
  int argc_v{};
  char **argv_v{nullptr};
};

} // namespace cli11_wrapper
