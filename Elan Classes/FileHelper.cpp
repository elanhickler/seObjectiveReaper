#include "FileHelper.h"
#include <map>
#include "StringHelper.h"
#include "salt_exceptions.h"

String illegal_filename_chars = "\\/:*?|<>\"";
String illegal_directory_chars = "*?|<>\"";

enum class FileErrorCode
{
    empty_string,
    illegal_char,
    path_is_dir,
    begins_with_space,
    space_before_ext,
    ends_with_space,    
    ends_with_dot,
    ends_or_begins_with_space,
    file_already_exists,
    file_already_exists_as_folder,
    directory_with_multi_slash,
    unknown_error_create_dir,
    cannot_create_file,
    unknown_error_file_move,
    cannot_overwrite_file,
    unknown_error_file_copy,
};

std::map<FileErrorCode, std::string> fileErrorMessages = {
	{ FileErrorCode::empty_string, "Input string is empty" },
	{ FileErrorCode::illegal_char, "Illegal character found" },
	{ FileErrorCode::path_is_dir, "File path cannot be a directory" },
	{ FileErrorCode::begins_with_space, "Begins with space" },
	{ FileErrorCode::space_before_ext, "Space before extension" },
	{ FileErrorCode::ends_with_space, "Ends with space" },
	{ FileErrorCode::ends_with_dot, "Ends with dot" },
	{ FileErrorCode::ends_or_begins_with_space, "Ends or begins with space" },
	{ FileErrorCode::file_already_exists, "File already exists" },
	{ FileErrorCode::file_already_exists_as_folder, "File already exists as a folder" },
    { FileErrorCode::directory_with_multi_slash, "Directory path has multiple slashes in a row"},
	{ FileErrorCode::unknown_error_create_dir, "Unknown error creating directory" },
	{ FileErrorCode::unknown_error_file_move, "Unknown error moving or renaming file" },
	{ FileErrorCode::cannot_create_file, "Unknown error creating file" },
	{ FileErrorCode::cannot_overwrite_file, "Could not overwrite file" },
	{ FileErrorCode::unknown_error_file_copy, "Unknown error copying file" },
};

void throwError(FileErrorCode errorCode)
{
	throw SaltException(fileErrorMessages[errorCode]); 
}

bool FileHelper::isIllegalChar(const juce_wchar & c)
{
    return c <= 31 || CHAr::isAnyOf(c, illegal_filename_chars);
}

void FileHelper::isValidDir(const String& directory)
{
    if (directory.isEmpty()) throwError(FileErrorCode::empty_string);

    if (directory.trim() != directory) throwError(FileErrorCode::ends_or_begins_with_space);

    if (directory.removeCharacters(illegal_directory_chars) != directory) throwError(FileErrorCode::illegal_char);

    if (directory.contains("\\\\") || directory.contains("//")) throwError(FileErrorCode::directory_with_multi_slash);
}

void FileHelper::isValidName(const String& filename)
{
    if (filename.isEmpty()) throwError(FileErrorCode::empty_string);

    enum MODE { begin, middle, end };
    MODE mode = begin;

    int last_dot_pos = 0;

    StringIterator it(filename);

    while (!it.atEnd())
    {
        switch (mode)
        {
        case begin:
            if (FileHelper::isIllegalChar(it)) throwError(FileErrorCode::illegal_char);
            if (it == ' ') throwError(FileErrorCode::begins_with_space);
            if (it == '.') last_dot_pos = it;
            if (it.atLast()) { mode = end; continue; }
            mode = middle;
            ++it;
            continue;
        case middle:
            if (FileHelper::isIllegalChar(it)) throwError(FileErrorCode::illegal_char);
            if (it == '.') last_dot_pos = it;
            if (it.atLast()) { mode = end; continue; }
            ++it;
            continue;
        case end:
            if (FileHelper::isIllegalChar(it)) throwError(FileErrorCode::illegal_char);
            if (it == ' ') throwError(FileErrorCode::ends_with_space);
            if (it == '.') throwError(FileErrorCode::ends_with_dot);
            ++it;
            continue;
        }
    }

    // make sure no space before extension
    if (it[std::max(last_dot_pos - 1, 0)] == ' ')
        throwError(FileErrorCode::space_before_ext);
}

void FileHelper::isValidFilePath(const String& path)
{
    if (CHAr::isAnyOf(path.getLastCharacter(), "\\/"))
        throwError(FileErrorCode::path_is_dir);

    String Dir = File(path).getParentDirectory().getFullPathName();
    isValidDir(Dir);

    String Name = File(path).getFileName();
    isValidName(Name);
}

String FileHelper::getFileName(const File& file) { return file.getFileName(); }
String FileHelper::getFileNameNoExt(const File& file) { return file.getFileNameWithoutExtension(); }
String FileHelper::getFileDir(const File& file) { return file.getParentDirectory().getFullPathName(); }
String FileHelper::getFilePath(const File& file) {return file.getFullPathName(); }

File FileHelper::setFilePath(const File & originalFile, const String & filePath, bool overwrite)
{
    if (originalFile == filePath)
        return originalFile;

    isValidFilePath(filePath);

    File newFile = filePath;

    if (newFile.existsAsFile())
    {
        if (!overwrite)
            throwError(FileErrorCode::file_already_exists);
        else if (!newFile.deleteFile())
            throwError(FileErrorCode::cannot_overwrite_file);
    }

    if (!newFile.getParentDirectory().createDirectory())
        throwError(FileErrorCode::unknown_error_create_dir);

    if (!originalFile.moveFileTo(newFile))
        throwError(FileErrorCode::unknown_error_file_move);

    return newFile;
}

File FileHelper::setFileDir(const File & file, const String & fileDir, bool overwrite)
{
    if (file.getParentDirectory() == fileDir)
        return fileDir;

    isValidDir(fileDir);

    if (!File(fileDir).createDirectory())
        throwError(FileErrorCode::unknown_error_create_dir);

    File newFile = fileDir + file.getFileName();

    if (newFile.existsAsFile())
    {
        if (!overwrite)
            throwError(FileErrorCode::file_already_exists);
        else if (!newFile.deleteFile())
            throwError(FileErrorCode::cannot_overwrite_file);
    }

    if (!file.moveFileTo(newFile))
        throwError(FileErrorCode::unknown_error_file_move);

    return newFile;
}

File FileHelper::setFileName(const File & file, const String & fileName, bool overwrite)
{
    if (file.getFileName() == fileName) 
        return fileName;

    isValidName(fileName);

    File newFile = file.getParentDirectory().getChildFile(fileName);

    if (newFile.existsAsFile())
    {
        if (!overwrite)
            throwError(FileErrorCode::file_already_exists);
        else if (!newFile.deleteFile())
            throwError(FileErrorCode::cannot_overwrite_file);
    }

    if(!file.moveFileTo(newFile))
        throwError(FileErrorCode::unknown_error_file_move);

    return newFile;
}

void FileHelper::createFile(const String& filePath, bool overwrite)
{
    isValidFilePath(filePath);

    File newFile = filePath;

     if (newFile.existsAsFile())
     {
        if (!overwrite)
            throwError(FileErrorCode::file_already_exists);
        else if (!newFile.deleteFile())
            throwError(FileErrorCode::cannot_overwrite_file);
     }

    else if (newFile.exists())
        throwError(FileErrorCode::file_already_exists_as_folder);

    if (!newFile.create())
        throwError(FileErrorCode::cannot_create_file);
}

void FileHelper::createFolder(const String& fileDir)
{
    isValidDir(fileDir);

    if (!File(fileDir).createDirectory())
        throwError(FileErrorCode::unknown_error_create_dir);
}

File FileHelper::copyFile(const File& origFile, const String& filePath, bool overwrite)
{
    isValidFilePath(filePath);

    File newFile = filePath;

    if (newFile.existsAsFile())
    {
        if (!overwrite)
            throwError(FileErrorCode::file_already_exists);
        else if (!newFile.deleteFile())
            throwError(FileErrorCode::cannot_overwrite_file);
    }

    if (!newFile.getParentDirectory().createDirectory())
        throwError(FileErrorCode::unknown_error_create_dir);

    if (!origFile.copyFileTo(newFile))
        throwError(FileErrorCode::unknown_error_file_copy);

    return File();
}
