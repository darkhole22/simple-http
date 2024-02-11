#pragma once
#include <SimpleHTTP/http.h>

#include <filesystem>

namespace simpleHTTP {

class ContentType
{
public:
    ContentType() = default;
    ContentType(MediaType type);

    void setMediaType(MediaType type);
    void addParameter(std::string_view parameter);

    explicit operator bool() const;

    std::string toString() const;
private:
    MediaType m_MediaType = MediaTypes::UNKNOWN;
    std::vector<std::string> m_Parameters;
};

class Resource
{
public:
    virtual inline StatusCodeType getStatusCode() const {
        return static_cast<StatusCodeType>(StatusCode::NOT_IMPLEMENTED);
    }

    virtual inline ContentType getContentType() const {
        return ContentType();
    }

    virtual inline u64 getContentLength() const {
        return 0;
    }

    virtual inline void sendCallback(ClientSocket* socket) {}

    virtual inline ~Resource() {}
private:
};

class FileResource : public Resource
{
public:
    FileResource(std::filesystem::path path);

    static std::unique_ptr<FileResource> make(std::filesystem::path path);

    virtual StatusCodeType getStatusCode() const override;

    virtual ContentType getContentType() const override;

    virtual u64 getContentLength() const override;

    virtual void sendCallback(ClientSocket* socket) override;

    virtual inline ~FileResource() override {}
private:
    std::filesystem::path m_Path;
};

class HeaderFileResource : public FileResource
{
public:
    HeaderFileResource(std::filesystem::path path);

    static std::unique_ptr<HeaderFileResource> make(std::filesystem::path path);

    virtual void sendCallback(ClientSocket* socket) override;

    virtual inline ~HeaderFileResource() override {}
private:
};

}
