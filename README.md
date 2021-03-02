<a href="https://godbolt.org/z/4dc3E8">![Try it online](https://img.shields.io/badge/try%20it-online-blue.svg)</a>
# Quick Arg Parser
Tired of unwieldy tools like _getopt_ or _argp_? Quick Arg Parser is a single header C++ library for parsing command line arguments and options with minimal amount of code. All you have to do is to instantiate a class inheriting from the `MainArguments` type and access its members.

```C++
#include "quick_arg_parser.hpp"

struct Args : MainArguments<Args> {
	std::string folder = argument(0) = ".";
	int efficiency = option("efficiency", 'e', "The intended efficiency") = 5;
	bool verbose = option('v');
	std::vector<int> ports = option("port", p);
};

int main(int argc, char** argv) {
	Args args{{argc, argv}};
	if (args.verbose) {
		std::cout << "Folder " << args.folder << std::endl;
		std::cout << "Efficiency " << args.efficiency << std::endl;
	}
	// ...
```

And it can deal with the following call:
```
./a.out . --efficiency 9 -v -p 4242,6824
```

A longer example of usage is [here](https://github.com/Dugy/quick_arg_parser/blob/main/quick_arg_parser_test_manual.cpp).

## More detailed information
The library requires C++11. I have tested it on GCC and Clang. C++11 does not allow aggregate-initialising parent classes, so the child class of `MainArguments` will have to inherit its constructor `using MainArguments<Args>::MainArguments`, allowing to create instances without double braces.

It should work on Windows, but the command-line arguments will be Unix-like (unless explicitly made different, see [below](https://github.com/Dugy/quick_arg_parser#legacy-options)).

It can parse:
* integer types
* floating point types
* `std::string`
* `std::filesystem::path` (if C++17 is available)
* `std::vector` containing types that it can parse, expecting them to set multiple times (options only) or comma-separated
* `std::unordered_map` indexed by `std::string` and containing types it can parse, expecting to be set as `-pjob=3,work=5 -ptask=7`
* `std::shared_ptr` to types it can parse
* `std::unique_ptr` to types it can parse
* `Optional` (a clone of `std::optional` that can be implicitly converted to it if C++17 is available) of types it can parse
* custom types if a parser for them is added (see [below](https://github.com/Dugy/quick_arg_parser#custom-types))

A class called `Optional` has to be used instead of `std::optional` (its usage is similar to `std::optional` and can be implicitly converted to it). If the option is missing, it will be empty; it won't compile with default arguments (except `nullptr` and `std::nullopt`).

Options are declared as follows:
```C++
TypeName varName = option("long_name", 'l', "Help entry") = "default value";
```
The long name of the option or the short name of the option can be omitted. No option is mandatory, if the option is not listed, it's zero, an empty string, an empty vector or a null pointer. The value when it's missing can be set by assigning into the `option` call. The help entry will be printed when calling the program with the `--help` option. It can be omitted.

Boolean options are true when the option is listed and false by default. Groups of boolean arguments can be written together, for example you can write `-qrc` instead of  `-q -r -c` in the options. Other options expect a value to follow them.

Mandatory arguments are declared as follows:
```C++
TypeName varName = argument(0);
```
The number is the index of the argument, indexed from zero. The program name is not argument zero.

Optional arguments are declared as follows:
```C++
TypeName varName = argument(0) = "default value";
```

Anything behind a ` -- ` separator will be considered an argument, no matter how closely it resembles an option.

If you expect an unlimited number of arguments, you can access them all through `MainArguments`' public variable named `arguments`. The first one in the vector is the first argument after the program name.

To implement a behaviour where the first argument is actually a command, like with `git`, the arguments have to be parsed separately for each command. Quick Arg Parser does not facilitate this, but it can be used with it by dealing with the first argument through a usual `if`/`else if` group, then constructing `MainArguments` instantiations with `{argc - 1, argv + 1}`.

### Automatic help entry
Calling the program with `--help` or `-?` will print the expected number of arguments and all options, also listing their help entries if set.

The description of the program and arguments can be altered by defining a method with signature `static std::string help(const std::string&)`, which gets the program name as argument and is expected to output the first part of help. To replace the help for options, you need to define a method `static std::string options()`.

By default, the program exits after printing help. This behaviour can be changed by defining a method with signature `void onHelp()` and it will be called instead.

## Automatic version entry
If the class has an `inline static` string member called `version` or a method with signature `static std::string version()`, it will react to options `--version` or `-V` by printing the string and exiting. The automatic exit can be overriden by defining a `void onVersion()` method, which will be called instead.

## Validation
You can add a lambda (or a class with overloaded function call operator) that takes the value and returns either a bool indicating if the value is valid or throws an exception if the value is invalid.

```C++
	int port = option("port", 'p').validator([] (int port) { return port > 1023; });
```

## C++17
If C++17 is available, then the `Optional` type can be converted into `std::optional`. Because of a technical limitation, `std::optional` cannot be used as an argument type. Also, arguments can be deserialised into `std::filesystem::path`.

## Legacy options
Sometimes, it's necessary to support options like `-something` or `/something`. This can be done using:
```C++
	int speed = nonstandardOption("-efficiency", 'e');
```
If this is done, the long option will not be expected to be exactly as listed in the first argument, not preceded by a double dash. If it starts with a single dash, it will not be considered an aggregate of short options.

For compatibility with atypical command line interfaces, setting an argument `-p` to `1024` can be done not only as `-p 1024`, but also as `-p=1024` or `-p1024`. Also, if it's a long argument named `--port`, it can be written as `--port=1024`. A vector type argument can be alternatively written as multiple settings of the same option, for example `-p 1024 -p1025`.

## Custom types
To support your custom class (called `MyType` here), define this somewhere before the definition of the parsing class:
```C++
namespace QuickArgParserInternals {
template <>
struct ArgConverter<MyType, void> {
	static MyType makeDefault() {
		return {}; // Do something else if it doesn't have a default constructor
	}
	static MyType deserialise(const std::string& from) {
		return MyType::fromString(from); // assuming this is how it's deserialised
	}
	constexpr static bool canDo = true;
};
} // namespace
```

## Gotchas
This isn't exactly the way C++ was expected to be used, so there might be a few traps for those who use it differently than intended. The class inheriting from `MainArguments` can have other members, but its constructor can be dangerous. Using the constructor to initialise members set through `option` or `argument` will cause the assignment to override the parsing behaviour for those members. The constructor also should not have side effects, because it will be called more than once, not always with the parsed values. Neither of this matters if you use it as showcased.

Because of consistency, using `Optional` as an argument type does not make that argument optional, you need to set `nullptr` or `std::nullopt` (C++17) as default argument to make it optional.
