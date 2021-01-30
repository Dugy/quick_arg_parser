#include <iostream>
#include <vector>
#include <sstream>
#include <memory>
#include <array>
#if __cplusplus > 201703L
#include <optional>
#endif

namespace QuickArgParserInternals {

struct ArgumentError : std::runtime_error {
	using std::runtime_error::runtime_error;
};

template <typename T, typename SFINAE = void>
struct ArgConverter {
	constexpr static bool canDo = false;
};

template <typename T>
struct ArgConverter<T, std::enable_if_t<std::is_integral_v<T>>> {
	static T makeDefault() {
		return 0;
	}
	static T deserialise(const std::string& from) {
		return std::stoi(from);
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<T, std::enable_if_t<std::is_floating_point_v<T>>> {
	static T makeDefault() {
		return 0;
	}
	static T deserialise(const std::string& from) {
		return std::stof(from);
	}
	constexpr static bool canDo = true;
};

template <>
struct ArgConverter<std::string, void> {
	static std::string makeDefault() {
		return "";
	}
	static std::string deserialise(const std::string& from) {
		return from;
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<std::shared_ptr<T>, void> {
	static std::shared_ptr<T> makeDefault() {
		return nullptr;
	}
	static std::shared_ptr<T> deserialise(const std::string& from) {
		return std::make_shared<T>(ArgConverter<T>::deserialise(from));
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<std::unique_ptr<T>, void> {
	static std::unique_ptr<T> makeDefault() {
		return nullptr;
	}
	static std::unique_ptr<T> deserialise(const std::string& from) {
		return std::make_unique<T>(ArgConverter<T>::deserialise(from));
	}
	constexpr static bool canDo = true;
};

template <typename T>
struct ArgConverter<std::vector<T>, std::enable_if_t<ArgConverter<T>::canDo>> {
	static std::vector<T> makeDefault() {
		return {};
	}
	static std::vector<T> deserialise(const std::string& from) {
		std::vector<T> made;
		int lastPosition = 0;
		for (int i = 0; i < int(from.size()) + 1; i++) {
			if (from[i] == ',' || i == int(from.size())) {
				made.push_back(ArgConverter<T>::deserialise(
						std::string(from.begin() + lastPosition, from.begin() + i)));
				lastPosition = i + 1;
			}
		}
		return made;
	}
	constexpr static bool canDo = true;
};

template <typename T>
class Optional {
	alignas(T) std::array<int8_t, sizeof(T)> _contents;
	bool _exists = false;
	void clear() {
		if (_exists)
			operator*().~T();
	}
public:
	Optional() = default;
	Optional(std::nullptr_t) {}
#if __cplusplus > 201703L
	Optional(std::nullopt_t) {}
#endif
	Optional(const Optional& other) {
		if (_exists)
			new (operator->()) T(*other);
	}
	Optional(Optional&& other) {
		if (_exists)
			new (operator->()) T(*other); 
	}
	T& operator=(const T& other) {
		clear();
		if (_exists) {
			operator*() = other;
		} else
			new (_contents.data()) T(other);
		_exists = true;
		return operator*();
	}
	T& operator=(T&& other) {
		clear();
		if (_exists)
			operator*() = other;
		else
			new (_contents.data()) T(other);
		_exists = true;
		return operator*();
	}
	void operator=(std::nullptr_t) {
		clear();
		_exists = false;
	}
#if __cplusplus > 201703L
	void operator=(std::nullopt_t) {
		operator=(nullptr);
	}
#endif
	T& operator*() {
		return *reinterpret_cast<T*>(_contents.data());
	}
	const T& operator*() const {
		return *reinterpret_cast<const T*>(_contents.data());
	}
	T* operator->() {
		return reinterpret_cast<T*>(_contents.data());
	}
	const T* operator->() const {
		return reinterpret_cast<const T*>(_contents.data());
	}
	operator bool() const {
		return _exists;
	}
#if __cplusplus > 201703L
	operator std::optional<T>() {
		if (_exists)
			return std::optional<T>(operator*());
		else
			return std::nullopt;
	}
#endif
	~Optional() {
		clear();
	}
};

template <typename T>
struct ArgConverter<Optional<T>, void> {
	static Optional<T> makeDefault() {
		return nullptr;
	}
	static Optional<T> deserialise(const std::string& from) {
		Optional<T> made;
		made = ArgConverter<T>::deserialise(from);
		return made;
	}
	constexpr static bool canDo = true;
};


template <typename T, typename SFINAE = void>
struct HasHelpProvider : std::false_type {};

template <typename T>
struct HasHelpProvider<T, std::enable_if_t<
		!std::is_void_v<decltype(T::help(std::declval<std::string>()))>>>
		: std::true_type {};

template <typename T, typename SFINAE = void>
struct HasHelpCallback : std::false_type {};

template <typename T>
struct HasHelpCallback<T, std::enable_if_t<
		std::is_void_v<decltype(std::declval<T>().onHelp())>>>
		: std::true_type {};

template <typename T, typename SFINAE = void>
struct HasHelpOptionsProvider : std::false_type {};

template <typename T>
struct HasHelpOptionsProvider<T, std::enable_if_t<
		!std::is_void_v<decltype(T::options())>>>
		: std::true_type {};


template <typename T, typename SFINAE = void>
struct HasVersionConstant : std::false_type {};

template <typename T>
struct HasVersionConstant<T, std::enable_if_t<
		!std::is_void_v<decltype(std::string(T::version))>>>
		: std::true_type {};

template <typename T, typename SFINAE = void>
struct HasVersionGetter : std::false_type {};

template <typename T>
struct HasVersionGetter<T, std::enable_if_t<
		!std::is_void_v<decltype(std::string(T::version()))>>>
		: std::true_type {};

template <typename T, typename SFINAE = void>
struct HasVersionCallback : std::false_type {};

template <typename T>
struct HasVersionCallback<T, std::enable_if_t<
		std::is_void_v<decltype(std::declval<T>().onVersion())>>>
		: std::true_type {};


} // namespace

template <typename Child>
class MainArguments {
	std::string _programName;
	std::vector<std::string> _argv;
	inline static std::stringstream _helpPreface;
	inline static std::stringstream _help;

	constexpr static int NOT_FOUND = -1;
	inline static std::vector<std::pair<std::string, char>> _nullarySwitches;
	inline static std::vector<std::pair<std::string, char>> _unarySwitches;
	inline static int _argumentCountMin = 0;
	inline static int _argumentCountMax = 0;
	enum InitialisationStep {
		UNINITIALISED,
		INITIALISING,
		INITIALISED
	};
	inline static InitialisationStep _initialisationState = UNINITIALISED;

public:
	template <typename T> using Optional = QuickArgParserInternals::Optional<T>;
	MainArguments() = default;
	MainArguments(int argc, char** argv) : _programName(argv[0]), _argv(argv + 1, argv + argc) {
		using namespace QuickArgParserInternals;
		if (_initialisationState == UNINITIALISED) {
			// When first created, create temporarily another instance to explore what are the members
			_initialisationState = INITIALISING;

			Child investigator;
			// This will fill the static variables
			if constexpr(QuickArgParserInternals::HasHelpProvider<Child>::value) {
				_helpPreface << Child::help(_programName);
			} else {
				_helpPreface << _programName << " takes between " << _argumentCountMin << " and " << _argumentCountMax <<
						" arguments, plus these options:";
			}
			if constexpr(QuickArgParserInternals::HasHelpOptionsProvider<Child>::value) {
				_help << Child::options();
			}

			investigator._initialisationState = INITIALISED;
		}
		if (_initialisationState == INITIALISED) {
			bool switchesEnabled = true;
			auto isListedAs = [] (const auto& arg, const std::vector<std::pair<std::string, char>>& switches) {
				for (const auto& it : switches) {
					if constexpr(std::is_same_v<std::remove_const_t<std::decay_t<decltype(arg)>>, char>) {
						if (it.second == arg)
							return true;
					} else {
						if (it.first == arg)
							return true;
					}
				}
				return false;
			};
			auto printHelp = [this] () {
				std::cout << _helpPreface.str() << std::endl;
				std::cout << _help.str() << std::endl;
				if constexpr(QuickArgParserInternals::HasHelpCallback<Child>::value) {
					static_cast<Child*>(this)->onHelp();
				} else {
					std::exit(0);
				}
			};
			auto printVersion = [this] () {
				if constexpr(QuickArgParserInternals::HasVersionConstant<Child>::value) {
					std::cout << Child::version << std::endl;
				} else if constexpr(QuickArgParserInternals::HasVersionGetter<Child>::value) {
					std::cout << Child::version() << std::endl;
				} else return false; // Returns false if the version is not known, leading to no action if found
				
				if constexpr(QuickArgParserInternals::HasVersionCallback<Child>::value) {
					static_cast<Child*>(this)->onVersion();
					return true;
				} else {
					std::exit(0);
					return true;
				}
			};

			// Collect program arguments (as opposed to switches) and validate everything
			for (int i = 0; i < int(_argv.size()); i++) {
				if (switchesEnabled && _argv[i][0] == '-') {
					if (_argv[i] == "--help") {
						printHelp();
						continue;
					}
					if (_argv[i] == "--version") {
						if (printVersion())
							continue;
					}
					
					if (_argv[i][1] == '-') {
						// Starts with --
						if (_argv[i].size() == 2) {
							switchesEnabled = false;
							continue; // the -- marks an end of switches
						}
						if (isListedAs(_argv[i], _unarySwitches)) {
							i++; // The next argument is part of the switch
							continue;
						} else if (!isListedAs(_argv[i], _nullarySwitches)) {
							throw ArgumentError("Unknown switch " + _argv[i]);
						}
					} else {
						// Starts with -
						if (_argv[i].size() == 2) {
							// Is an argument of type -x
							if (_argv[i][1] == '?') {
								printHelp();
								continue;
							}
							if (_argv[i][1] == 'V') {
								if (printVersion())
									continue;
							}
							
							if (isListedAs(_argv[i][1], _unarySwitches)) {
								i++; // The next argument is part of the switch
								continue;
							} else if (!isListedAs(_argv[i][1], _nullarySwitches)) {
								throw ArgumentError(std::string("Unknown switch ") + _argv[i][1]);
							}
							// Otherwise it's a bool switch that can be ignored
						} else {
							// Some validations that all massed single letter switches don't have arguments
							for (int j = 1; j < int(_argv[i].size()); j++) {
								if (!isListedAs(_argv[i][j], _nullarySwitches)) {
									if (isListedAs(_argv[i][j], _unarySwitches))
										throw ArgumentError("Switch group " + _argv[i]
												+ " contains a switch that needs an argument");
									else
										throw ArgumentError(std::string("Unknown switch ") + _argv[i][j]);
								}
							}
						}
					}
				} else {
					// Is not a switch
					arguments.push_back(_argv[i]);
				}
			}

			if (int(arguments.size()) < _argumentCountMin)
				throw ArgumentError("Expected at least " + std::to_string(_argumentCountMin)
						+ " arguments, got " + std::to_string(arguments.size()));
			if (int(arguments.size()) > _argumentCountMax)
				throw ArgumentError("Expected at most " + std::to_string(_argumentCountMax)
						+ " arguments, got " + std::to_string(arguments.size()));
		}
	}
	std::vector<std::string> arguments;

private:
	
	int findArgument(const std::string& argument, char shortcut) const {
		// Look for shortcut, end of string means no shortcut
		if (shortcut != '\0') {
			for (int i = 0; i < int(_argv.size()); i++) {
				if (_argv[i][0] == '-' && _argv[i][1] != '-') {
					for (int j = 1; _argv[i][j] != '\0'; j++) {
						if (_argv[i][j] == shortcut) {
							return i + 1;
						}
					}
				}
				if (_argv[i] == "--") {
					break;
				}
			}
		}
		
		// Look for full argument name, empty means no full argument name
		if (!argument.empty()) {
			for (int i = 0; i < int(_argv.size()); i++) {
				if (_argv[i][0] == '-' && _argv[i][1] == '-') {
					if (_argv[i] == argument) {
						return i + 1;
					}
				}
			}
		}
		
		return NOT_FOUND;
	}
	
protected:
	class GrabberBase {
	protected:
		const std::string name;
		const MainArguments* parent;
		const char shortcut;
		const std::string help;
		GrabberBase(const MainArguments* parent, const std::string& name, char shortcut, const std::string& help)
				: name(name), parent(parent), shortcut(shortcut), help(help) {}

		void addHelpEntry() const {
			if constexpr(QuickArgParserInternals::HasHelpOptionsProvider<Child>::value)
				return;

			if (shortcut != '\0')
				parent->_help << '-' << shortcut;
			parent->_help << '\t';
			if (!name.empty())
				parent->_help << name;
			parent->_help << "\t " << help << std::endl;
		}
	public:
		operator bool() const {
			if (parent->_initialisationState == INITIALISING) {
				parent->_nullarySwitches.push_back(std::make_pair(name, shortcut));
				addHelpEntry();
				return false;
			}
			return parent->findArgument(name, shortcut) != NOT_FOUND;
		}

		template <typename T>
		T getOption(T defaultValue) const {
			if (parent->_initialisationState == INITIALISING) {
				parent->_unarySwitches.push_back(std::make_pair(name, shortcut));
				addHelpEntry();
				return defaultValue;
			}
			int position = parent->findArgument(name, shortcut);
			if (position != NOT_FOUND) {
				return QuickArgParserInternals::ArgConverter<T>::deserialise(parent->_argv[position]);
			}
			return defaultValue;
		}
	};

	template <typename Default>
	class GrabberDefaulted : public GrabberBase {
		Default defaultValue;
	public:
		GrabberDefaulted(const MainArguments* parent, const std::string& name, char shortcut,
				const std::string& help, Default defaultValue)
				: GrabberBase(parent, name, shortcut, help), defaultValue(defaultValue) {}
		template <typename T, std::enable_if_t<QuickArgParserInternals::ArgConverter<T>::canDo && !std::is_same_v<T, bool>>* = nullptr>
		operator T() const {
			return GrabberBase::getOption(defaultValue);
		}
	};
	
	class Grabber : public GrabberBase {
		friend class MainArguments;
	public:
		using GrabberBase::GrabberBase;
		template <typename Default>
		auto operator=(Default defaultValue) {
			return GrabberDefaulted<Default>(GrabberBase::parent, GrabberBase::name,
					GrabberBase::shortcut, GrabberBase::help, defaultValue);
		}
		
		template <typename T, std::enable_if_t<QuickArgParserInternals::ArgConverter<T>::canDo>* = nullptr>
		operator T() const {
			return GrabberBase::getOption(QuickArgParserInternals::ArgConverter<T>::makeDefault());
		}
	};

	Grabber option(const std::string& name, char shortcut = '\0', const std::string& help = "") {
		return Grabber(this, "--" + name, shortcut, help);
	}
	Grabber option(char shortcut = '\0', const std::string& help = "") {
		return Grabber(this, "", shortcut, help);
	}

	class ArgGrabberBase {
	protected:
		const MainArguments* parent;
		const int index;
	public:
		ArgGrabberBase(const MainArguments* parent, int index) : parent(parent), index(index) {}
	};

	template <typename Default>
	class ArgGrabberDefaulted : public ArgGrabberBase {
		Default defaultValue;
	public:
		ArgGrabberDefaulted(const MainArguments* parent, int index, Default defaultValue) :
				ArgGrabberBase(parent, index), defaultValue(defaultValue) {}

		template <typename T, std::enable_if_t<QuickArgParserInternals::ArgConverter<T>::canDo>* = nullptr>
		operator T() const {
			if (ArgGrabberBase::parent->_initialisationState == INITIALISING) {
				ArgGrabberBase::parent->_argumentCountMax =
						std::max(ArgGrabberBase::parent->_argumentCountMax, ArgGrabberBase::index + 1);
				return QuickArgParserInternals::ArgConverter<T>::makeDefault();
			}
			if (ArgGrabberBase::index >= int(ArgGrabberBase::parent->arguments.size()))
				return defaultValue;
			return QuickArgParserInternals::ArgConverter<T>::deserialise(
					ArgGrabberBase::parent->arguments[ArgGrabberBase::index]);
		}
	};
	
	struct ArgGrabber : public ArgGrabberBase {
		using ArgGrabberBase::ArgGrabberBase;
		template <typename Default>
		auto operator=(Default defaultValue) const {
			return ArgGrabberDefaulted<Default>(ArgGrabberBase::parent, ArgGrabberBase::index, defaultValue);
		}
		template <typename T, std::enable_if_t<QuickArgParserInternals::ArgConverter<T>::canDo>* = nullptr>
		operator T() const {
			if (ArgGrabberBase::parent->_initialisationState == INITIALISING) {
				ArgGrabberBase::parent->_argumentCountMin =
						std::max(ArgGrabberBase::parent->_argumentCountMin, ArgGrabberBase::index + 1);
				ArgGrabberBase::parent->_argumentCountMax =
						std::max(ArgGrabberBase::parent->_argumentCountMax, ArgGrabberBase::index + 1);
				return QuickArgParserInternals::ArgConverter<T>::makeDefault();
			}
			return QuickArgParserInternals::ArgConverter<T>::deserialise(ArgGrabberBase::parent->arguments[ArgGrabberBase::index]);
		}
	};
	
	ArgGrabber argument(int index) {
		return ArgGrabber(this, index);
	}
};
