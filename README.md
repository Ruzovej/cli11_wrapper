# `cli11_wrapper`

Small wrapper to ease use of the [CLI11 library](https://github.com/CLIUtils/CLI11).

## Usage

### `C++`

Most probably the easiest way is to get inspired in [unit test cases](tests/unit/cases/argv_parser.test.cxx) (especially in the last comprehensive case `SUBCASE("all at once")`).

TODO "real" example.

### Build system

In Your `CMakeLists.txt`:

```cmake
...

include(FetchContent)

FetchContent_Declare(
    cli11_wrapper
    GIT_REPOSITORY https://github.com/Ruzovej/cli11_wrapper
    GIT_TAG vX.Y.Z # update this ...
)
FetchContent_MakeAvailable(cli11_wrapper)

...

target_link_libraries(
    your_target
        PRIVATE # or PUBLIC?
            cli11_wrapper::cli11_wrapper
            ...
)

...
```

## License

![LGPLv3 image](doc/lgplv3-with-text-154x68.png)

[`LGPLv3`](https://www.gnu.org/licenses/lgpl-3.0.html) -> [COPYING](COPYING) & [COPYING.lesser](COPYING.LESSER)
