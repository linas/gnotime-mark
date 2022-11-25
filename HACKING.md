# HACKING

## Platform Support

_GnoTime_ is undergoing a refactoring currently. The last _Ubuntu_ version which
seems to be able to build and run it seems to be _Ubuntu 12.04 "Trusty Tahr"_.
For this reason this version will be treated as the lower boundary for the
versions of dependencies. The longterm goal is to always maintain compatibility
with _Debian oldstable_ - which would be _Debian Buster_ currently.

Currently Linux is supported only. For now no support of further platforms is
planned but not ruled out if newer build systems and dependency versions make
this possible without too much effort.

## Coding Style

The coding style to be applied for this project is defined by the
`.clang-format` file at the root of this project. It is derived from the "GNU"
style template of clang-format version 7.
