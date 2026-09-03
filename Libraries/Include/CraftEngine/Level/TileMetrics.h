#pragma once

#include "Utils/EngineMacro.h"

NAME_SPACE_BEGIN(Craft)

// 필드 오브젝트가 차지하는 "타일" 한 칸의 크기(콘솔 셀 개수)의 기본값.
//
// ★ 실제 값은 레벨이 가진다 (Level::GetTileSize) ★
// 레벨마다 아트 스케일이 다르다 - 프롭 정규 크기가 8x8인 레벨은 타일도 8이다.
// 이 상수는 레벨이 값을 알려주기 전에 쓰는 기본값일 뿐이다.
// 값의 출처는 레이아웃 XML(<LevelLayout tileSize="..">)이다.
//
// 타일은 정사각이지만 오브젝트의 타일 영역은 정사각이 아니다.
// 오브젝트는 크기에 따라 타일을 여러 개 먹는다(PropSprite::GetTileSpan).
//
// ★ 서버의 tileSize와 다른 개념이다 ★
// 서버 쪽 tileSize는 길찾기용 묶음 단위이고, 이 값은 클라이언트의 그림/배치 격자다.
// Cemetery.level.xml 주석에 적어둔 것과 같은 이유로 두 값을 "통일"하려 들지 말 것.
//
// 레벨 격자(LevelMap)의 한 칸은 콘솔 셀 하나이고 이것과도 다르다.
// 즉 프롭 한 타일 = 레벨 격자 12x12칸이다.
constexpr int DefaultTileSize = 12;

NAME_SPACE_END
