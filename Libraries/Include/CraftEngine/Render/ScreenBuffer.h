#pragma once

#include "Math/Vector2.h"

NAME_SPACE_BEGIN(Craft)

class ScreenBuffer
{
public:
	ScreenBuffer(const Vector2& screenSize);
	~ScreenBuffer();

	// 콘솔 초기화 - 화면 지우기
	void Clear() const;

	// Palette의 16색을 이 콘솔 버퍼의 ColorTable에 적용한다.
	void ApplyPalette() const;

	void Draw(const CHAR_INFO* const charInfo) const;

	// Getter
	inline HANDLE GetBuffer() const { return buffer; }

	// 실제로 잡힌 화면 크기.
	// 요청한 크기가 물리적 화면에 안 들어가면 생성자가 줄여서 잡으므로,
	// 요청값과 다를 수 있다.
	inline Vector2 GetSize() const { return size; }

	// 콘솔 화면 펴버 핸들
	HANDLE buffer = nullptr;

	// 화면 크기
	Vector2 size;
};

NAME_SPACE_END