#ifndef INPUT_H
#define INPUT_H

#include <string>


bool readFile(const std::string& filepath, std::string& outContent, std::string& outError);


std::string readStandardInput();

#endif 