#pragma once
#include "../../config.h"
#include <unordered_map>


namespace df {
namespace detail {

class FileIO {
	using Path_Type = std::filesystem::path;
private:
	FileIO(const FileIO&) = delete;
	FileIO operator=(const FileIO&) = delete;
public:

	FileIO(const Path_Type &path) : _path(path), _dirtyBit(false) { Load(); }

	const std::string& GetContent() const { return _content; }
	void SetContent(const std::string& newContent) { _dirtyBit = true; _content = newContent; }
		
	bool Load();
	bool Save();

private:
	const Path_Type _path;
	std::string _content;
	bool _dirtyBit = false; //https://www.youtube.com/watch?v=JwQZQygg3Lk
};

class FileCache
{
public:
	using Path_Type = std::filesystem::path;
private:
	FileCache(const FileCache&) = delete;
	FileCache operator=(const FileCache&) = delete;
public:
	static Path_Type GetAbsolutePath(const Path_Type& path_);
	static size_t GetHashValue(const Path_Type& path_);

	FileCache() {}

	// If not in the cache already, the file is loaded. Hash is returned
	size_t AddFile(const Path_Type &path_);

	// Get file from cache. Use GetHashValue to compute hash_.
	FileIO& GetFile(size_t hash_);

private:
	std::unordered_map<size_t, FileIO> _cache;
};

}//detail
}//df