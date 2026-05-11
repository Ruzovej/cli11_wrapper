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

#include "cli11_wrapper/args.hxx"

#include <cstring>

#include <utility>

namespace cli11_wrapper {

args::args(std::string_view const app_name,
           std::vector<std::string> const &args)
    : owned{true} {
  auto const num_remaining_args{args.size()};

  argc_v = static_cast<int>(num_remaining_args);
  argv_v = new char *[argc_v + 2];

  argv_v[0] = strndup(app_name.data(), app_name.size());
  for (std::size_t i{0}; i < num_remaining_args; ++i) {
    argv_v[i + 1] = strndup(args[i].data(), args[i].size());
  }
  argv_v[num_remaining_args + 1] = nullptr;
}

args::args(int const aArgc, char **const aArgv) noexcept
    : owned{false}, argc_v{aArgc}, argv_v{aArgv} {
  // force 2 lines
}

args::args(args &&rhs) noexcept
    : owned{std::exchange(rhs.owned, false)},
      argc_v{std::exchange(rhs.argc_v, 0)}, argv_v{std::exchange(rhs.argv_v,
                                                                 nullptr)} {
  // force 2 lines
}

args &args::operator=(args &&rhs) noexcept {
  using std::swap;

  swap(owned, rhs.owned);
  swap(argc_v, rhs.argc_v);
  swap(argv_v, rhs.argv_v);

  return *this;
}

args::~args() noexcept {
  if (owned && (argv_v != nullptr)) {
    for (int i{0}; i < argc_v + 1; ++i) {
      std::free(argv_v[i]); // due to `strndup`
    }
    delete[] argv_v;
  }
}

} // namespace cli11_wrapper
