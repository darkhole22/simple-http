#include <SimpleHTTP/handler/Resource.h>

#include <iostream>
#include <fstream>
#include <array>

namespace simpleHTTP {

ContentType::ContentType(MediaType type)
    : m_MediaType(type) {}

void ContentType::setMediaType(MediaType type) {
    m_MediaType = type;
}

void ContentType::addParameter(std::string_view parameter) {
    m_Parameters.emplace_back(parameter);
}

ContentType::operator bool() const {
    return std::strlen(m_MediaType) > 0;
}

std::string ContentType::toString() const {
    std::string result(m_MediaType);

    for (auto& parameter : m_Parameters) {
        result += "; ";
        result += parameter;
    }

    return result;
}

FileResource::FileResource(std::filesystem::path path)
    :m_Path(path) {}

std::unique_ptr<FileResource> FileResource::make(std::filesystem::path path) {
    return std::make_unique<FileResource>(path);
}

StatusCodeType FileResource::getStatusCode() const {
    StatusCode code = std::filesystem::exists(m_Path) ? StatusCode::OK : StatusCode::NOT_FOUND;
    return static_cast<StatusCodeType>(code);
}

ContentType FileResource::getContentType() const {
    ContentType type{};

    auto extension = m_Path.extension();

    if (extension.empty()) {
        return type;
    }

    if (extension == ".html") {
        type.setMediaType(MediaTypes::TEXT_HTML);
        type.addParameter("charset=utf-8");
    }
    else if (extension == ".js" || extension == ".mjs") {
        type.setMediaType(MediaTypes::TEXT_JAVASCRIPT);
        type.addParameter("charset=utf-8");
    }
    else if (extension == ".css") {
        type.setMediaType(MediaTypes::TEXT_CSS);
        type.addParameter("charset=utf-8");
    }
    else if (extension == ".json") {
        type.setMediaType(MediaTypes::APPLICATION_JSON);
        type.addParameter("charset=utf-8");
    }
    else if (extension == ".txt") {
        type.setMediaType(MediaTypes::TEXT_PLAIN);
        type.addParameter("charset=utf-8");
    }
    else if (extension == ".mp4") {
        type.setMediaType(MediaTypes::VIDEO_MP4);
    }
    else if (extension == ".ico") {
        type.setMediaType(MediaTypes::IMAGE_VND_MICROSOFT_ICON);
    }

    return type;
}

u64 FileResource::getContentLength() const {
    try {
        return std::filesystem::file_size(m_Path);
    } catch (...) {
        return 0;
    }
}

constexpr u64 FILE_INPUT_BUFFER_SIZE = 0x100000;

void FileResource::sendCallback(ClientSocket* socket) {
    std::ifstream file(m_Path, std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open File: " << m_Path << std::endl;
        return;
    }

    std::vector<char> buffer(FILE_INPUT_BUFFER_SIZE, 0);

    while (!file.eof()) {
        file.read(buffer.data(), buffer.size());
        std::streamsize size = file.gcount();

        if (size == 0)
            break;

        socket->send(buffer.data(), size);
    }
}

HeaderFileResource::HeaderFileResource(std::filesystem::path path) : FileResource(path) {}

std::unique_ptr<HeaderFileResource> HeaderFileResource::make(std::filesystem::path path) {
    return std::make_unique<HeaderFileResource>(path);
}

void HeaderFileResource::sendCallback(ClientSocket* socket) {}

} // namespace simpleHTTP
