#ifndef ENGINEUTILS_H
#define ENGINEUTILS_H

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include <iostream>
#include <algorithm>
#include "lua.hpp"
#include "LuaBridge.h"

static void ReadJsonFile(const std::string& path, rapidjson::Document & out_document)
{
	FILE* file_pointer = nullptr;
#ifdef _WIN32
	fopen_s(&file_pointer, path.c_str(), "rb");
#else
	file_pointer = fopen(path.c_str(), "rb");
#endif
	char buffer[65536];
	rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
	out_document.ParseStream(stream);
	std::fclose(file_pointer);

	if (out_document.HasParseError()) {
		rapidjson::ParseErrorCode errorCode = out_document.GetParseError();
		std::cout << "error parsing json at [" << path << "]" << std::endl;
		exit(0);
	}
}

static void ReportError(const std::string& actor_name, const luabridge::LuaException& e) {
    std::string e_message = e.what();
    std::replace(e_message.begin(), e_message.end(), '\\', '/');
    std::cout << "\033[31m" << actor_name << " : " << e_message << "\033[0m\n";
}

#endif