//usr/bin/g++ --std=c++17 -Wall $0 -o ${o=`mktemp`} && exec $o $*
#include "quick_arg_parser.hpp"

struct Input : MainArguments<Input> {
	bool verbose = option("verbose", 'v');
	bool shorten = option("shorten", 's');
	int port = option("port", 'p');
	float timeout = option("timeout", 't', "Timeout in seconds");
	Optional<std::string> debugLog = option("debug_log", 'd');
	
	std::filesystem::path file = argument(0);
	std::filesystem::path secondaryFile = argument(1) = "aux.out";
	
	inline static std::string version = "1.0";
	static std::string help(const std::string& programName) {
		return "Usage:\n" + programName + " FILE (SECONDARY_FILE)";
	}
};

int main(int argc, char** argv) {
	Input in{{argc, argv}};
	
	std::cout << "Arguments interpreted:" << std::endl;
	std::cout << "Verbose: " << in.verbose << std::endl;
	std::cout << "Shorten: " << in.shorten << std::endl;
	std::cout << "Port: " << in.port << std::endl;
	std::cout << "Timeout: " << in.timeout << std::endl;
	std::optional<std::string> debugLog = in.debugLog;
	if (debugLog)
		std::cout << "DebugLog " << *debugLog << std::endl;
	std::cout << "File: " << in.file << std::endl;
	std::cout << "Secondary file: " << in.secondaryFile << std::endl;
}
