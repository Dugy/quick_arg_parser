# Quick Arg Parser
Tired of unwieldy tools like _getopt_ or _argp_? Quick Arg Parser is a single header C++ library for parsing command line arguments and options with minimal amount of code. All you have to do is to instantiate a class inheriting from the `MainArguments` type and access its members.

```C++
#include "quick_arg_parser.hpp"

struct Args : MainArguments<Args> {
	std::string folder = argument(0) = ".";
	int efficiency = option("efficiency", 'e', "The intended efficiency") = 5;
	bool verbose = option('v');
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
./a.out . --efficiency 9 -v
```

A longer example of usage is [here](https://github.com/Dugy/quick_arg_parser/blob/main/quick_arg_parser_test_manual.cpp).

## More detailed information
The library requires C++17. I have tested it on GCC and Clang. It should work on Windows, but the command-line arguments will not be Windows-like.

It can parse integer types, floating point types, `std::string`, `std::vector` of already supported types (assuming comma separated lists), `shared_ptr` and `unique_ptr` to already supported types. Because of a technical limitation, I haven't figured out how to support `std::optional` as well.

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

### Automatic help entry
Calling the program with `--help` or `-?` will print the expected number of arguments and all options, also listing their help entries if set.

The description of the program and arguments can be altered by defining a method with signature `static std::string help(const std::string&)`, which gets the program name as argument and is expected to output the first part of help. To replace the help for options, you need to define a method `static std::string options()`.

By default, the program exits after printing help. This behaviour can be changed by defining a method with signature `void onHelp()` and it will be called instead.

## Automatic version entry
If the class has an `inline static` string member called `version` or a method with signature `static std::string version()`, it will react to options `--version` or `-V` by printing the string and exiting. The automatic exit can be overriden by defining a `void onVersion()` method, which will be called instead.

## Gotchas
This isn't exactly the way C++ was expected to be used, so there might be a few traps for those who use it differently than intended. The class inheriting from `MainArguments` can have other members, but its constructor can be dangerous. Using the constructor to initialise members set through `option` or `argument` will cause the assignment to override the parsing behaviour for those members. The constructor also should not have side effects, because it will be called more than once, not always with the parsed values. Neither of this matters if you use it as showcased.
