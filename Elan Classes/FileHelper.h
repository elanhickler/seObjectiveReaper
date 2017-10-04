#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

using juce::juce_wchar;
using juce::String;
using juce::File;

namespace FileHelper 
{
bool isIllegalChar(const juce_wchar& c);
void isValidFilePath(const String & path);
void isValidDir(const String& directory);
void isValidName(const String& fileName);

String getFileName(const File& file);
String getFileNameNoExt(const File& file);
String getFileDir(const File& file);
String getFilePath(const File& file);

File setFileDir(const File& file, const String& fileDir, bool overwrite);
File setFileName(const File& file, const String& fileName, bool overwrite);
File setFilePath(const File& file, const String& filePath, bool overwrite);

void createFile(const String& filePath, bool overwrite);
void createFolder(const String& fileDir);

File copyFile(const File& origFile, const String& filePath, bool overwrite);
}