#pragma once
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <fcntl.h>

// file object by ummap
class MapFile {
private:
    char *m_pFile;
    struct stat m_fileState;
    int m_fileCode;  // 404 403 200 500
public:
    MapFile(const std::string &filePath) {
        m_pFile = nullptr;
        memset(&m_fileState, 0, sizeof(m_fileState));
        int ret = stat(filePath.c_str(), &m_fileState);
        if(ret == -1 || S_ISDIR(m_fileState.st_mode)) {
            // not found
            m_fileCode = 404;
        }
        else if(!(m_fileState.st_mode & S_IROTH)) {
            // forbidden
            m_fileCode = 403;
        }
        else {
            // file ok
            m_fileCode = 200;
            int fd = open(filePath.c_str(), O_RDONLY);
            m_pFile = (char*)mmap(0, m_fileState.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            close(fd);

            if((void*)m_pFile == MAP_FAILED) {
                // error map
                m_fileCode = 500;
                m_pFile = nullptr;
            }
        }
 
    }
    ~MapFile() {
        if(m_pFile) {
            munmap(m_pFile, m_fileState.st_size);
            m_pFile = nullptr;
        }
    }
    char* getFilePointer() {
        return m_pFile;
    }
    size_t getFileLength() {
        return m_fileState.st_size;
    }
    int getFileCode() {
        return m_fileCode;
    }
};