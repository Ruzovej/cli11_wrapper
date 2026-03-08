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

#include <string_view>

namespace cli11_wrapper::os_wrapper {

[[nodiscard]] int current_errno() noexcept;

// returns: if successful (e.g. `0 <= syscall_ret`);
// throws: `std::runtime_error` on failure with detailed message
void check_syscall_ret_val(std::string_view const file, int const line,
                           int const syscall_ret);

// NOTE: use the syscall directly, without this wrapper, if failure is allowed
// (see implementation & above)
template <typename ret_t>
ret_t syscall_helper(std::string_view const file, int const line,
                     ret_t const syscall_ret) {
  check_syscall_ret_val(file, line, static_cast<int>(syscall_ret));
  return syscall_ret;
}

} // namespace cli11_wrapper::os_wrapper

#define EXEC_PATH_ARGS_SYSCALL_HELPER(fn)                                      \
  ::cli11_wrapper::os_wrapper::syscall_helper<decltype(fn)>(__FILE__,         \
                                                             __LINE__, (fn))
