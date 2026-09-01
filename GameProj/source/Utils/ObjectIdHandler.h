#pragma once
#include "Protocol/Enum.pb.h"

/*---------------------
	ObjectIdHandler
----------------------*/

class ObjectIdHandler
{
	enum : uint64
	{
		OBJECT_TYPE_SHIFT = 48,
		OBJECT_TYPE_MASK = 0xFFFF'0000'0000'0000,
		OBJECT_COUNT_MASK = 0x0000'FFFF'FFFF'FFFF,
	};

public:
	inline static Protocol::ObjectType GetObjectType(uint64 objectId) {
		return static_cast<Protocol::ObjectType>((objectId & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT); }
};
