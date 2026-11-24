#include "input.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool readFile(const std::string& filepath, std::string& outContent, std::string& outError) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        outError = "Error: No se pudo abrir el archivo '" + filepath + "'. Verifique que la ruta sea correcta.";
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    outContent = buffer.str();
    return true;
}

std::string readStandardInput() {
    std::stringstream buffer;
    buffer << std::cin.rdbuf();
    return buffer.str();
}