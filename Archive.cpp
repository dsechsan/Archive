//
//  Archive.cpp
//
//
//
//

#include "Archive.hpp"
#include <memory>
#include <utility>

namespace ECE141 {

  //STUDENT put archive class code here...

    void ArchiveObserver::operator()(ActionType anAction, const std::string& aName, bool status) {
        switch (anAction) {
            case ActionType::added: std::cout << "add "; break;
            case ActionType::extracted: std::cout << "extract "; break;
            case ActionType::removed: std::cout << "remove "; break;
            case ActionType::listed: std::cout << "list "; break;
            case ActionType::dumped: std::cout << "dump "; break;
            case ActionType::compacted: std::cout << "compact "; break;
        }
        std::cout << aName << "\n";
        std::cout << "File: " << aName << ", Status: " << (status ? "Success" : "Failure") << "\n";
    }

    Archive::Archive(std::string aFullPath, ECE141::AccessMode aMode) : archiveFilePath(std::move(aFullPath)),accessMode(aMode) {};

    std::string Archive::getFileName(const std::string &aFullPath) {
        size_t thePos = aFullPath.find_last_of('/');

        if (thePos == std::string::npos) {
            return aFullPath;
        } else {
            auto fileName = aFullPath.substr(thePos + 1);
            return fileName;
        }
    }

    ArchiveStatus<std::shared_ptr<Archive>> Archive::createArchive(const std::string &anArchiveName) {
        std::string theArchiveName =anArchiveName;
        if ((anArchiveName.rfind(".arc") != anArchiveName.length() - 4)) {
            theArchiveName += ".arc";
        }

        std::ofstream theFileStream(theArchiveName, std::ios::binary | std::ios::out | std::ios::trunc);
        if(!theFileStream.is_open()){
            std::cerr << "error opening file\n" ;
            return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileOpenError);
        }

        auto theArchivePtr = std::shared_ptr<Archive>(new Archive(theArchiveName, AccessMode::AsNew));
        theArchivePtr->chunkManager = std::make_shared<ChunkManager>(theArchiveName);
//        theArchivePtr->chunkManager->readChunksFromFile();

        return ArchiveStatus<std::shared_ptr<Archive>>(theArchivePtr);

  }

    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName){
        std::string theArchiveName = anArchiveName;
        if (anArchiveName.length() > 4 && (anArchiveName.rfind(".arc") != anArchiveName.length() - 4)) {
            theArchiveName += ".arc";
        }
        std::ifstream theFileStream(theArchiveName, std::ios::binary | std::ios::in);

        if(!std::filesystem::exists(theArchiveName)){
            std::cerr << "File not found\n" ;
            return ArchiveStatus<std::shared_ptr<Archive>>(ArchiveErrors::fileNotFound);
        }

        auto theArchivePtr = std::shared_ptr<Archive>(new Archive(theArchiveName, AccessMode::AsExisting));

        theArchivePtr->chunkManager = std::make_shared<ChunkManager>(theArchiveName);
        theArchivePtr->chunkManager->getArchiveChunkCount();
        theFileStream.close();

        return ArchiveStatus<std::shared_ptr<Archive>>(theArchivePtr);
    }

    Archive&  Archive::addObserver(std::shared_ptr<ArchiveObserver>& anObserver){
        observers.push_back(anObserver);
        return *this;
    }

    ArchiveStatus<bool>   Archive::add(const std::string &aFullPath){
        /* 1. get list of free chunks
         * 2. add chunks if necessary
         * 3. write data to chunks
         */
        auto theFileName = getFileName(aFullPath);

        if(chunkManager->find(theFileName)){
            std::cerr << "File already exists in the archive\n";
            return ArchiveStatus<bool>(ArchiveErrors::fileExists);
        }else{
            std::ifstream theFileStream(aFullPath, std::ios::binary | std::ios::in);
            if(!std::filesystem::exists(aFullPath)){
                std::cerr << "File doesn't exist in memory\n" ;
                return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);
            }
            chunkManager->setInputFileName(aFullPath);
//            chunkManager->writeDataToChunks(theFileName);
            chunkManager->writeChunksToArchive(theFileName);

            theFileStream.close();
            return ArchiveStatus<bool>(true);
        }
    }

    ArchiveStatus<bool>   Archive::extract(const std::string &aFilename, const std::string &aFullPath) {
        bool theStatus{false};

//        std::ofstream theFileStream(aFullPath, std::ios::binary | std::ios::out);
        if(chunkManager->find(aFilename) ){
            theStatus = chunkManager->retrieve(aFilename,aFullPath);
        }else{
            return ArchiveStatus<bool>(ArchiveErrors::badFilename);
        }
//        theFileStream.close();

        if(theStatus) return ArchiveStatus<bool>(true);
        return ArchiveStatus<bool>(ArchiveErrors::fileWriteError);
    }

    ArchiveStatus<bool>   Archive::remove(const std::string &aFilename){
        bool theStatus{false};
        if(chunkManager->find(aFilename)) {
            theStatus = chunkManager->deleteChunksOfFile(aFilename);
        }
        else return ArchiveStatus<bool>(ArchiveErrors::badArchive);
        if(theStatus) return ArchiveStatus<bool>(true);
    }

    ArchiveStatus<size_t>  Archive::list(std::ostream &aStream){
        size_t theNumOfFiles{0};
        chunkManager->listFiles(aStream, &theNumOfFiles);
        return ArchiveStatus<size_t>(theNumOfFiles);
    }

    ArchiveStatus<size_t>  Archive::debugDump(std::ostream &aStream){
        chunkManager->dump(aStream);
        return ArchiveStatus<size_t>(chunkManager->getArchiveChunkCount());
    }

    ArchiveStatus<size_t>  Archive::compact(){
        chunkManager->Compact();
        auto theNumChunks = chunkManager->getArchiveChunkCount();
        return ArchiveStatus<size_t>(theNumChunks);
    }

    ArchiveStatus<std::string> Archive::getFullPath() const {
        return ArchiveStatus<std::string>(archiveFilePath);
    }


}
