#ifndef UTILS_HPP
#define UTILS_HPP

#include <fmt/format.h>
#include <GL/glew.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#define RED_TEXT "\033[31m"
#define RESET_TEXT "\033[0m"

namespace utils
{
    void printGLVersion();
    
    namespace conversion
    {
        float toRadians(float degrees);
    }
    namespace disk
    {
        std::string getCurrentDirectory();
        std::string getDirFromFilename(const std::string& filename);
        void printCurrentDirectory();
        void printFilesInCurrentDirectory();
        bool readFile(const char* pFileName, std::string& outFile);
    }

    namespace format
    {
        void clearConsole();
        void printBegin();
        void printEnd();
    }

    namespace timer
    {
        void shutdown(int);
    }
}

#endif // UTILS_HPP