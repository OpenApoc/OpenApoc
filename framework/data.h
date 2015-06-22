#pragma once

#include "includes.h"
#include "image.h"
#include "sound.h"

#include <queue>
#include <vector>
#include <fstream>

namespace OpenApoc {

#define PROGRAM_NAME "OpenApoc"
#define PROGRAM_ORGANISATION "OpenApoc"

class ImageLoader;
class SampleLoader;
class MusicLoader;
class Framework;

class IFileImpl
{
public:
	virtual ~IFileImpl();
};

class IFile : public std::istream
{
private:
	std::unique_ptr<IFileImpl> f;
	IFile();
	friend class Data;
public:
	~IFile();
	size_t size() const;
	bool readule16(uint16_t &val);
	bool readule32(uint32_t &val);
	const UString &fileName() const;
	const UString &systemPath() const;
	IFile(IFile &&other);
};

class Data
{

	private:
		UString writeDir;
		std::map<UString, std::weak_ptr<Image> >imageCache;
		std::map<UString, std::weak_ptr<ImageSet> >imageSetCache;

		std::map<UString, std::weak_ptr<Sample> >sampleCache;
		std::map<UString, std::weak_ptr<MusicTrack> >musicCache;

		//Pin open 'imageCacheSize' images
		std::queue<std::shared_ptr<Image> > pinnedImages;
		//Pin open 'imageSetCacheSize' image sets
		std::queue<std::shared_ptr<ImageSet> > pinnedImageSets;
		std::list<std::unique_ptr<ImageLoader>> imageLoaders;
		std::list<std::unique_ptr<SampleLoader>> sampleLoaders;
		std::list<std::unique_ptr<MusicLoader>> musicLoaders;

	public:
		Data(Framework &fw, std::vector<UString> paths, int imageCacheSize = 10, int imageSetCacheSize = 10);
		~Data();

		enum class FileMode
		{
			Read,
			Write,
			ReadWrite,
		};

		std::shared_ptr<Sample> load_sample(const UString& path);
		std::shared_ptr<MusicTrack> load_music(const UString& path);
		std::shared_ptr<Image> load_image(const UString& path);
		std::shared_ptr<ImageSet> load_image_set(const UString& path);
		std::shared_ptr<Palette> load_palette(const UString& path);
		IFile load_file(const UString& path, FileMode mode = FileMode::Read);

};

}; //namspace OpenApoc
