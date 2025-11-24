#include "file_includes.h"

FileIncludes::FileIncludes(){
    
}

void FileIncludes::clearIncludes(Filename fileName){
    if (includesMap.contains(fileName)){
        includesMap.erase(fileName);
    }
}

void FileIncludes::addInclude(Filename fileName, Filename includedFile){
    includesMap[fileName].insert(includedFile);
}

std::unordered_set<Filename> FileIncludes::getChildren(Filename fileName){
    return includesMap[fileName];
}