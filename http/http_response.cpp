#include <sys/socket.h>
#include <unordered_map>

#include "http_response.h"

const std::unordered_map<std::string, std::string> SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

std::string getFileType(const std::string &filePath) {
    size_t pos =  filePath.find_last_not_of(".");
    if(pos == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = filePath.substr(pos);
    if(SUFFIX_TYPE.count(suffix)) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    else {
        return "text/plain";
    }


}


// class method
HttpResponse::HttpResponse() {
    m_pWrite = 0;
    m_state = ResponseState::SEND;
}

HttpResponse::~HttpResponse() {
    // empty
}

void HttpResponse::writeOnce(int fd) {
    size_t bytesToSend = m_writeBuffer.size();
    if(m_pMapFile && m_pMapFile->getFilePointer()) {
        bytesToSend += m_pMapFile->getFileLength();
    }
    while(m_pWrite < bytesToSend) {
        ssize_t bytes;
        if(m_pWrite < m_writeBuffer.size()) {
            // send first buffer
            bytes = send(fd, m_writeBuffer.data()+m_pWrite, m_writeBuffer.size()-m_pWrite, 0);
        }
        else {
            // send second buffer(mapped file buffer)
            bytes = send(fd, m_pMapFile->getFilePointer()+m_pWrite-m_writeBuffer.size(), bytesToSend-m_pWrite, 0);
        }
        if(bytes == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            else {
                // something wrong
                m_state = ResponseState::SEND_ERROR;
            }
        }
        else {
            m_pWrite += bytes;
        }
    }
    m_state = ResponseState::DONE;
}

void HttpResponse::fillResponseBuffer(HttpRequest &request) {
    // request state may be PARSE_ERROR, DONE
    RequestState requestState = request.getState();
    if(requestState == RequestState::PARSE_ERROR) {
        this->dealWith400();
        return;
    }
    // DONE
    

    std::string method = request.getMethod();
    if(method == "GET") {
        this->dealWithGet(request.getFilePath(), request.getkeepAlive());
    }
    else {
        // only support get now
        // fix this to get the welcome
        this->dealWithGet("/", request.getkeepAlive());
    }

}

void HttpResponse::init() {
    m_writeBuffer.clear();
    m_pMapFile = nullptr;
    m_pWrite = 0;
    m_state = ResponseState::SEND;
} 

ResponseState HttpResponse::getState() {
    return m_state;
}


// private
void HttpResponse::add_status_line(int code){
    // example:
    // HTTP/1.1 200 OK\r\n
    static const std::string protocol = "HTTP/1.1";
    static const std::unordered_map<int, std::string> code2title = {
        {200, "OK"}, {400, "Bad Request"}, {403, "Forbidden"},
        {404, "Not Found"}, {500, "Internal Error"}
    };
    // if(!code2title.count(code)) {
    //     // not supported code 
    //     m_state = ResponseState::INTERNAL_ERROR;
    //     code = 500;
    // }

    m_writeBuffer.append(protocol).append(" ");
    m_writeBuffer.append(std::to_string(code)).append(" ");
    m_writeBuffer.append(code2title.find(code)->second).append("\r\n");
}

void HttpResponse::add_headers(const std::string &keepAlive, const std::string &contentType, const std::string &contentLength) {
    m_writeBuffer.append("Connection: ").append(keepAlive).append("\r\n");
    m_writeBuffer.append("Content-type: ").append(contentType).append("\r\n");
    m_writeBuffer.append("Content-length: ").append(contentLength).append("\r\n");
    m_writeBuffer.append("\r\n");  // empty line
}

void HttpResponse::dealWithGet(std::string filePath, bool keepAlive) {
    if(filePath == "/") {
        filePath = "root/welcome.html";
    }
    else {
        filePath = "root" + filePath;
    }
    m_pMapFile = std::make_unique<MapFile>(filePath);
    if(m_pMapFile->getFilePointer()) {
        // 200 ok
        this->add_status_line(200);
        this->add_headers(keepAlive? "keep-alive": "close", getFileType(filePath), std::to_string(m_pMapFile->getFileLength()));
    }
    else {
        int fileCode = m_pMapFile->getFileCode(); 
        this->add_status_line(fileCode);
        this->add_headers("close", "text/plain", "0");
    }
}

void HttpResponse::dealWith400() {
    this->add_status_line(400);
    this->add_headers("close", "text/plain", "0");
}
