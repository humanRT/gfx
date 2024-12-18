#include "utils.hpp"

namespace fmt {
    template <>
    struct formatter<std::tm> {
        constexpr auto parse(format_parse_context& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const std::tm& tm, FormatContext& ctx) {
            return format_to(ctx.out(), "{:02}:{:02}:{:02}", tm.tm_hour, tm.tm_min, tm.tm_sec);
        }
    };
}

void shutdownTimer(int seconds, bool* runIndifeinitely)
{
    while (seconds > 0 || *runIndifeinitely) {
        if (!*runIndifeinitely) {
            std::string title = fmt::format("Shutting down in {} seconds ...", seconds);
            std::cout << "\033[33m" << title << "\033[0m" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            seconds--;
        }
        else return;        
    }
   
    exit(0);
}

void utils::printGLVersion()
{
    const GLubyte* version = glGetString(GL_VERSION);
    if (version) {
        std::cout << "OpenGL Version: " << version << std::endl;
    } else {
        std::cerr << "Failed to get OpenGL version!" << std::endl;
    }
}

float utils::conversion::toRadians(float degrees)
{
    return degrees * M_PI / 180.0;
}

std::string utils::disk::getCurrentDirectory()
{
    return std::filesystem::current_path().string();
}

std::string utils::disk::getDirFromFilename(const std::string& filename)
{
    // Extract the directory part from the file name
    std::string::size_type SlashIndex;

#ifdef _WIN64
    SlashIndex = Filename.find_last_of("\\");

    if (SlashIndex == -1) {
        SlashIndex = Filename.find_last_of("/");
    }
#else
    SlashIndex = filename.find_last_of("/");
#endif

    std::string Dir;

    if (SlashIndex == std::string::npos) {
        Dir = ".";
    }
    else if (SlashIndex == 0) {
        Dir = "/";
    }
    else {
        Dir = filename.substr(0, SlashIndex);
    }

    return Dir;
}

void utils::disk::printCurrentDirectory()
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::cout << "Current Directory: " << currentPath << std::endl;
}

void utils::disk::printFilesInCurrentDirectory()
{
    std::cout << "Files:" << std::endl;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        std::cout << entry.path().filename() << std::endl;
    }
}

bool utils::disk::readFile(const char* pFileName, std::string& outFile)
{
    std::ifstream f(pFileName);

    bool ret = false;

    if (f.is_open()) {
        std::string line;
        while (getline(f, line)) {
            outFile.append(line);
            outFile.append("\n");
        }

        f.close();

        ret = true;
    }
    else {
        std::string title = "File error: " + std::string(pFileName);
        std::cout << "\033[31m" << title << "\033[0m" << std::endl;
    }

    return ret;
}

void utils::format::clearConsole()
{
    system("clear");
}

void utils::format::printBegin()
{
    std::time_t currentTime = std::time(nullptr);
    std::tm localTime = *std::localtime(&currentTime);
    std::string title = fmt::format(".BEGIN [{}]", localTime);
    std::cout << "\033[32m" << title << "\033[0m" << std::endl;
    std::cout << "\033[32m" << std::string(40, '-') << "\033[0m" << std::endl;
}

void utils::format::printEnd()
{
    std::cout << std::string(40, '-') << std::endl;
    std::cout << std::endl << ".END" << std::endl;
}
void utils::timer::shutdown(int seconds, bool* runIndifeinitely)
{
    std::thread shutdownThread([seconds, runIndifeinitely]() {
        shutdownTimer(seconds, runIndifeinitely);
    });
    shutdownThread.detach();
}
