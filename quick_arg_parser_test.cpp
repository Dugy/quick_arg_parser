//usr/bin/g++ --std=c++11 -Wall $0 -o ${o=`mktemp`} && exec $o $*
#include "quick_arg_parser.hpp"

struct Input : MainArguments<Input> {
	using MainArguments<Input>::MainArguments; // Not necessary in C++17
	bool verbose = option("verbose", 'V');
	int port = option("port", 'p');
	int secondaryPort = option("port2", 'P') = 999;
	int parts = argument(0) = 1;
	Optional<int> logPort = option("logPort", 'l');
};

struct Input2 : MainArguments<Input2> {
	using MainArguments<Input2>::MainArguments;
	std::vector<int> ports = option("ports", 'p');
	std::shared_ptr<int> downloads = option("downloads", 'd', "The number of downloads");
	std::unique_ptr<int> uploads = option("uploads", 'u');
	std::string file = argument(0);
	std::string logFile = argument(1) = "log.log";
	std::string debugLogFile = argument(2) = "debug.log";
	Optional<std::string> logAddress = option("logAddress", 'l');
	int legacyOption = nonstandardOption("-line").validator([] (int a) { return a < 10; }) = 0;
	std::string legacyOption2 = nonstandardOption("/tool") = "none";

	static std::string help(const std::string& programName) {
		return "Usage\n" + programName + " FILE LOG DEBUGLOG";
	}
	void onHelp() {
		std::cout << "Help called" << std::endl;
	}
	
	static std::string version() {
		return "3.3.7";
	}
	void onVersion() {}
};

struct Input3 : MainArguments<Input3> {
	using MainArguments<Input3>::MainArguments;
	std::vector<int> ports = option("ports", 'p');
	bool enableHorns = option('h');
	std::string file = argument(0);
	bool enableHooves = option('H');
	bool loud = option("LOUD");
	std::string target = argument(1) = "a.out";

	void onHelp() {}
	static std::string options() {
		return "Don't use the options, they suck\n";
	}
	static std::string version;
	void onVersion() {}
};
std::string Input3::version = "1.0"; // Not necessary in C++17

struct Input4 : MainArguments<Input4> {
	using MainArguments<Input4>::MainArguments;
	std::vector<int> outputConnectors = option("connectors", 'c');
	std::string genre = option("genre", 'g') = "metal";
	float masterVolume = option("master_volume", 'v') = 100;
	std::unordered_map<std::string, float> speakerVolumes = option("speaker_volumes", 's');
	bool muteNeighbours = option("mute_neighbours", 'm');
	bool jamPhones = option("jam_phones", 'j');
	std::string path = argument(0) = ".";
};

struct Input5 : MainArguments<Input5> {
	using MainArguments<Input5>::MainArguments; // Not necessary in C++17
	std::vector<bool> verbose = option("verbose", 'V');
	bool extra = option("extra", 'e');
	int port = option("port", 'p');
	int secondaryPort = option("port2", 'P') = 999;
	int parts = argument(0) = 1;
	Optional<int> logPort = option("logPort", 'l');
};

template <typename T>
T constructFromString(std::string args) {
	std::vector<char*> segments;
	segments.push_back(&args[0]);
	for (int i = 0; i < int(args.size()); i++) {
		if (args[i] == ' ') {
			segments.push_back(&args[i + 1]);
			args[i] = '\0';
		}
	}
	return T{int(segments.size()), &segments[0]};
}

int errors = 0;

template <typename T1, typename T2>
void verify(T1 first, T2 second) {
	if (first != second) {
		std::cout << first << " and " << second << " were supposed to be equal" << std::endl;
		errors++;
	}
};

int main() {

	std::cout << "First input" << std::endl;
	Input t1 = constructFromString<Input>("super_program -V --port 666 -- 3");
	verify(t1.verbose, true);
	verify(t1.port, 666);
	verify(t1.secondaryPort, 999);
	verify(t1.parts, 3);
	verify(bool(t1.logPort), false);

	std::cout << "Second input" << std::endl;
	Input2 t2 = constructFromString<Input2>("mega_program -p 23,80,442 -u 3 -p 778 --help --version -line 2 --logAddress 127.0.0.1 -- -lame_file_name log");
	verify(int(t2.ports.size()), 4);
	verify(t2.file, "-lame_file_name");
	verify(bool(t2.downloads), false);
	verify(bool(t2.uploads), true);
	verify(*t2.uploads, 3);
	verify(t2.logFile, "log");
	verify(t2.debugLogFile, "debug.log");
	verify(bool(t2.logAddress), true);
	if (bool(t2.logAddress))
		verify(*t2.logAddress, "127.0.0.1");
	verify(t2.legacyOption, 2);
	verify(t2.legacyOption2, "none");

	std::cout << "Third input" << std::endl;
	Input3 t3 = constructFromString<Input3>("supreme_program file -hH -? -V --LOUD target");
	verify(int(t3.ports.size()), 0);
	verify(t3.file, "file");
	verify(t3.enableHooves, true);
	verify(t3.enableHorns, true);
	verify(t3.target, "target");
	verify(t3.loud, true);
	
	std::cout << "Fourth input" << std::endl;
	Input4 t4 = constructFromString<Input4>("ultimate_program -v110 -jc=5 --connectors=8 -mc 10 -sleft=110 -sright=105,bottom=115 -gpunk ~/Music");
	verify(int(t4.outputConnectors.size()), 3);
	if (int(t4.outputConnectors.size()) == 3) {
		verify(t4.outputConnectors[0], 5);
		verify(t4.outputConnectors[1], 8);
		verify(t4.outputConnectors[2], 10);
	}
	verify(t4.genre, "punk");
	verify(t4.masterVolume, 110);
	verify(t4.muteNeighbours, true);
	verify(t4.jamPhones, true);
	verify(t4.path, "~/Music");

	std::cout << "Fifth input" << std::endl;
	Input5 t5 = constructFromString<Input5>("super_program -VV -VeVV --port 666 -- 3");
	verify(int(t5.verbose.size()), 5);
	verify(t5.extra, true);
	verify(t5.port, 666);
	verify(t5.secondaryPort, 999);
	verify(t5.parts, 3);
	verify(bool(t5.logPort), false);

	std::cout << "Errors: " << errors << std::endl;
}
