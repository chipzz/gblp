#include <stdint.h>
#include <memory>

#pragma pack(push, 1)
struct BLPHeader
{
	uint8_t		fourcc[4];          // "BLP2" magic number
	uint32_t	type;               // 0 = JPG, 1 = BLP / DXTC / Uncompressed
	uint8_t		compression;        // 1 = BLP, 2 = DXTC, 3 = Uncompressed
	uint8_t		alpha_depth;        // 0, 1, 4, or 8
	uint8_t		alpha_type;         // 0, 1, 7, or 8
	uint8_t		has_mips;           // 0 = no mips, 1 = has mips
	uint32_t	width;              // Image width in pixels, usually a power of 2
	uint32_t	height;             // Image height in pixels, usually a power of 2
	uint32_t	mipmap_offsets[16]; // The file offsets of each mipmap, 0 for unused
	uint32_t	mipmap_lengths[16]; // The length of each mipmap data block
	uint32_t	palette[256];       // A set of 256 ARGB values used as a color palette

	BLPHeader(const void *const buffer);
};
#pragma pack(pop)

struct BLPFile
{
	BLPHeader	header;
	uint8_t		*data;

	BLPFile(const void *const buffer);
	~BLPFile();

	const void *GetMipMap(const short level, uint32_t &width, uint32_t &height, uint32_t &length) const;
	void Resize(uint32_t w, uint32_t h);
};
