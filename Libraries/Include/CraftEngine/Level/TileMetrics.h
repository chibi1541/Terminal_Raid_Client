#pragma once

#include "Utils/EngineMacro.h"

NAME_SPACE_BEGIN(Craft)

// 필드 오브젝트가 차지하는 "타일" 한 칸의 크기(콘솔 셀 개수).
//
// 프롭 스프라이트가 폭 12로 고정이고 타일 영역이 정사각이라 12다.
//
// ★ 서버의 tileSize와 다른 개념이다 ★
// 서버 쪽 tileSize는 길찾기용 묶음 단위이고, 이 값은 클라이언트의 그림/배치 격자다.
// Cemetery.level.xml 주석에 적어둔 것과 같은 이유로 두 값을 "통일"하려 들지 말 것 -
// 같은 정보가 두 곳에 있으면 반드시 갈라진다.
//
// 레벨 격자(LevelMap)의 한 칸은 콘솔 셀 하나이고 이것과도 다르다.
// 즉 프롭 한 타일 = 레벨 격자 12x12칸이다.
constexpr int PropTileSize = 12;

NAME_SPACE_END
