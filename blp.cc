#include "blp.h"
#include <cstring>

BLPHeader::BLPHeader(const void *const buffer)
{
	const uint8_t *const bytes = (uint8_t *) buffer;
	const uint32_t *const dwords = (uint32_t *) buffer;

	type		= dwords[1];	// Texture type: 0 = JPG, 1 = S3TC
	compression	= bytes[8];		// Compression mode: 1 = raw, 2 = DXTC
	alpha_depth	= bytes[9];		// 0, 1, 4, or 8
	alpha_type	= bytes[10];	// 0, 1, 7, or 8
	has_mips	= bytes[11];	// 0 = no mips levels, 1 = has mips (number of levels determined by image size)
	width		= dwords[3];
	height		= dwords[4];

	std::memcpy(mipmap_offsets, dwords + 5, 64);
	std::memcpy(mipmap_lengths, dwords + 21, 64);
	std::memcpy(palette, dwords + 37, 1024);
}

BLPFile::BLPFile(const void *const buffer) :
	header(buffer)
{
	size_t size = 0;
	for (unsigned int l = 0; header.mipmap_offsets[l]; l++)
		size = header.mipmap_offsets[l] + header.mipmap_lengths[l];

	const size_t data_size = size - sizeof(BLPHeader);
	data = new uint8_t[data_size];
	std::memcpy(data, ((uint8_t *) buffer) + sizeof(BLPHeader), data_size);
}

BLPFile::~BLPFile()
{
	delete []data;
}

const void *BLPFile::GetMipMap(const short level, uint32_t &w, uint32_t &h, uint32_t &length) const
{
	if (!header.mipmap_offsets[level])
	{
		w = 0; h = 0; length = 0;
		return NULL;
	}
	w = header.width >> level ? header.width >> level : 1;
	h = header.height >> level ? header.height >> level : 1;
	length = header.mipmap_lengths[level];

	return data + header.mipmap_offsets[level] - sizeof(BLPHeader);
}

void BLPFile::Resize(uint32_t w, uint32_t h)
{
}
