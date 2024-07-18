#pragma once

struct MipMappedSize
{
	unsigned int width = 0, height = 0;

	void get_size_for_mipmap(const unsigned int mipmap, float &w, float &h) const
	{
		w = width << mipmap ? width << mipmap : 1;
		h = height << mipmap ? height << mipmap : 1;
	}

	void get_size_for_level(const unsigned int level, float &w, float &h) const
	{
		const unsigned int max_dim = 1 << level;

		w = 2*float(max_dim);
		h = 2*float(max_dim) / 8;
	}
};