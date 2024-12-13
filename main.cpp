#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <fmt/core.h>
#include <fmt/color.h>
#include <sys/utsname.h>
#include <type_traits>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <vector>
#include <regex>
#include <array>
#include <memory>
#include <stdexcept>

class SystemInfo {
public:
    std::string os;
    std::string host;
    std::string kernel;
    std::string uptime;
    std::string packages;
    std::string shell;
    std::string terminal;
    std::string cpu;
    std::string memory;
    std::string portage;
    std::string profile;
    std::string gcc;

    static SystemInfo collect();

private:
    static std::string readFile(const std::string& path);
    static std::string getUptime();
    static std::string getCpuInfo();
    static std::string getMemoryInfo();
    static std::string getPackagesCount();
    static std::string execCommand(const std::string& cmd);
};

std::string SystemInfo::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "N/A";
    }
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

std::string SystemInfo::execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    auto pipe = std::unique_ptr<FILE, decltype(&pclose)>(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("critical-invalid: failed to run: " + cmd);
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string SystemInfo::getUptime() {
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        long uptime_seconds = sys_info.uptime;
        long days = uptime_seconds / (24 * 3600);
        uptime_seconds %= (24 * 3600);
        long hours = uptime_seconds / 3600;
        uptime_seconds %= 3600;
        long minutes = uptime_seconds / 60;

        std::ostringstream uptime_stream;
        if (days > 0) uptime_stream << days << "d ";
        if (hours > 0 || days > 0) uptime_stream << hours << "h ";
        uptime_stream << minutes << "m";
        return uptime_stream.str();
    }
    return "N/A";
}

std::string SystemInfo::getCpuInfo() {
    std::string cpu_info = readFile("/proc/cpuinfo");
    std::regex model_regex("model name\\s+:\\s+(.+)");
    std::smatch match;
    if (std::regex_search(cpu_info, match, model_regex) && match.size() > 1) {
        return match.str(1);
    }
    return "N/A";
}

std::string SystemInfo::getMemoryInfo() {
    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        std::ostringstream mem_stream;
        mem_stream << (sys_info.totalram - sys_info.freeram) / (1024 * 1024) << "M / "
                   << sys_info.totalram / (1024 * 1024) << "M";
        return mem_stream.str();
    }
    return "N/A";
}

SystemInfo SystemInfo::collect() {
    SystemInfo info;

    std::string os_info = readFile("/etc/os-release");
    std::regex pretty_name_regex("PRETTY_NAME=\\\"(.+?)\\\"");
    std::smatch match;
    if (std::regex_search(os_info, match, pretty_name_regex) && match.size() > 1) {
        info.os = match.str(1);
    } else {
        info.os = "Gentoo";
    }

    struct utsname uname_info;
    if (uname(&uname_info) == 0) {
        info.host = uname_info.nodename;
        info.kernel = uname_info.release;
    }

    info.uptime = getUptime();
    info.packages = execCommand("qlist -I | wc -l");
    info.shell = getenv("SHELL") ? getenv("SHELL") : "N/A";
    info.terminal = execCommand("tty");
    info.cpu = getCpuInfo();
    info.memory = getMemoryInfo();
    info.portage = execCommand("portageq --version");
    info.profile = execCommand("readlink /etc/portage/make.profile");
    info.gcc = execCommand("gcc --version | grep gcc");

    return info;
}

void printLogo() {
    fmt::print(fg(fmt::color::pink), R"(
    .vir.                                d$b
  .d$$$$$$b.    .cd$$b.     .d$$b.   d$$$$$$$$$$$b  .d$$b.      .d$$b.
  $$$$( )$$$b d$$$()$$$.   d$$$$$$$b Q$$$$$$$P$$$P.$$$$$$$b.  .$$$$$$$b.
  Q$$$$$$$$$$B$$$$$$$$P"  d$$$PQ$$$$b.   $$$$.   .$$$P' `$$$ .$$$P' `$$$
    "$$$$$$$P Q$$$$$$$b  d$$$P   Q$$$$b  $$$$b   $$$$b..d$$$ $$$$b..d$$$
   d$$$$$$P"   "$$$$$$$$ Q$$$     Q$$$$  $$$$$   `Q$$$$$$$P  `Q$$$$$$$P
  $$$$$$$P       `"""""   ""        ""   Q$$$P     "Q$$$P"     "Q$$$P"
  `Q$$P"                                  """
    )");
    fmt::print("\n");
}

void printSystemInfo(const SystemInfo& info) {
    auto printField = [](const std::string& label, const std::string& value, bool addNewLine = true) {
        fmt::print(fg(fmt::color::pink), "  {:<9}: ", label);
        fmt::print("{}", value.empty() ? "N/A" : value);
        if (addNewLine) {
            fmt::print("\n");
        }
    };

    printField("OS", info.os);
    printField("Host", info.host);
    printField("Kernel", info.kernel);
    printField("Uptime", info.uptime);
    printField("Packages", info.packages, false);
    printField("Shell", info.shell);
    printField("Terminal", info.terminal, false); // false Это типо чтоб не было лишних пропусков строки
    printField("CPU", info.cpu);
    printField("Memory", info.memory);
    printField("Portage", info.portage, false);
    printField("Profile", info.profile, false);
    printField("gcc", info.gcc);

    fmt::print("\n");  
}

int main() {
    try {
        printLogo();
        auto systemInfo = SystemInfo::collect();
        printSystemInfo(systemInfo);
    } catch (const std::exception& e) {
        std::cerr << fmt::format(fg(fmt::color::red), "critical-invalid: {}\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

