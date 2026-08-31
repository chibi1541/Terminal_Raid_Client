#pragma once

NAME_SPACE_BEGIN(Craft)

class CRAFT_API Image
{
public:
	Image(const char* image, uint16 raw, uint16 col);
	~Image() = default;

	const char* Get() const {return _image.c_str(); }

	uint16 GetRaw() const {return _raw;}
	uint16 GetCol() const {return _col;}

private:
	const std::string _image;

	uint16 _raw = 0;
	uint16 _col = 0;

};

NAME_SPACE_END
