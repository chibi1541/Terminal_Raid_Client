# -*- coding: utf-8 -*-
"""
정적 프롭 스프라이트 원본을 클라이언트 프롭 XML로 변환한다.

    python Tools/convert_props.py <source.txt> <output.xml> [--desc TEXT]

예)
    python Tools/convert_props.py Assets/graveyard_sprites.txt Assets/GraveyardProps.prop.xml

convert_level.py와 같은 방침이다.
    - 빌드에 넣지 않는다. 아트를 다시 뽑았을 때만 돌리고 결과 XML을 커밋한다.
    - BOM 없는 UTF-8 / LF. io.open(encoding='utf-8', newline='\\n')이 API로 보장한다.
    - 기호표(SYMBOL_COLORS)와 검증 절차를 그대로 공유한다.

입력 형식 (Assets/graveyard_sprites.txt)

    -----------------------------------------      <- 구분선
    [TOMB_A_FRONT]  12 x 14                        <- 이름, 너비 x 높이
    # 묘비 A 둥근 비석 - 정면                        <- 사람용 주석. 무시한다.
    ...KKKKKK...                                   <- 픽셀 행 x 높이
    ..KLLLLLLK..
    ...
                                                   <- 빈 줄로 블록 종료

[PALETTE] 블록은 이름 뒤에 " W x H"가 없어서 헤더 정규식에 안 걸린다.
그래서 따로 건너뛰는 처리를 하지 않아도 자연히 무시된다.

이름의 접미사가 방향 슬롯이 된다.
    _FRONT -> Up    (정면. 위/아래에서 볼 때)
    _SIDE  -> Left  (측면. 좌/우에서 볼 때)
접미사가 없으면 프롭 이름 그대로에 Up 슬롯 하나로 넣는다.

_BACK / _RIGHT 접미사도 받는다. 지금 데이터에는 없지만, 앞뒤가 다른
오브젝트(문, 조각상)가 생기면 원본에 블록만 추가하면 되고 이 파일은 안 바뀐다.
빠진 슬롯은 엔진 로더(PropSpriteLoader)가 반대편으로 폴백해서 채운다.
"""

import argparse
import io
import os
import re
import sys
import xml.etree.ElementTree as ElementTree
from collections import Counter, OrderedDict

# 엔진의 SymbolPalette(Math/SymbolPalette.h)와 같아야 한다.
# convert_level.py와 같은 표다. 어느 한쪽만 고치면 안 된다.
SYMBOL_COLORS = {
    'K': 'Black',     'D': 'DarkGray',  'L': 'Gray',      'B': 'White',
    'F': 'DarkGreen', 'G': 'Green',
    'N': 'DarkBrown', 'W': 'Brown',     'T': 'Tan',
    'C': 'DarkRed',   'R': 'Red',
    'O': 'Orange',    'Y': 'Yellow',
    'V': 'DarkBlue',  'U': 'Blue',
    'P': 'Purple',
}

# SymbolPalette::TransparentSymbol.
TRANSPARENT = '.'

CR = chr(13)
LF = chr(10)
TAB = chr(9)

# [NAME]  W x H  - 이름과 크기 사이는 공백 두 칸이다.
# 크기가 없는 [PALETTE] 같은 블록은 여기 안 걸리고 그대로 무시된다.
SPRITE_HEADER = re.compile(r'^\[([A-Z0-9_]+)\]\s+(\d+)\s*x\s*(\d+)$')

# 원본 이름 접미사 -> 엔진의 EFacing 슬롯 이름.
#
# _FRONT가 Up인 이유 - 카메라 기본 회전(k=0)에서 위를 보고 선 오브젝트가
# 화면에 정면으로 보인다. 엔진에서 표시 슬롯은 RotateFacing(facing, k)이라
# 같은 정면 그림이 k=2(180도)에서 Down 슬롯으로도 쓰인다(로더 폴백).
DIRECTION_SUFFIX = OrderedDict([
    ('_FRONT', 'Up'),
    ('_BACK',  'Down'),
    ('_SIDE',  'Left'),
    ('_RIGHT', 'Right'),
])

SLOT_ORDER = ['Up', 'Down', 'Left', 'Right']

# 스프라이트 블록 안의 선택적 지시자.
#
#   @pivot 5.5,9   기준점에 놓일 이미지 안의 점. 생략하면 기본값이 계산된다.
#
# 파일 전체에 한 번 나오는 지시자.
#
#   @tilesize 4    이 아트가 그려진 타일 격자 크기. tileSpan 계산과 기본 피벗에 쓴다.
SPRITE_PIVOT = re.compile(r'^@pivot\s+(-?[\d.]+)\s*,\s*(-?[\d.]+)$')

# @mirror x | xy | none  - Left <-> Right 폴백에서 뒤집을 축.
#
# 좌우 슬롯은 거울이 아니라 수직축 180도 회전이라 월드 두 축이 모두 뒤집힌다.
# 이미지에서 어느 축이 뒤집히는지는 그 그림의 세로축이 무엇을 뜻하느냐에 달렸다.
#
#   x    : 세로축이 "높이"인 그림(서 있는 울타리/기둥/묘비). 가로만 뒤집는다. 기본값.
#   xy   : 세로축이 "벽 방향"인 그림(문처럼 개구부 전체를 담은 것). 둘 다 뒤집는다.
#   none : 뒤집지 않는다.
SPRITE_MIRROR = re.compile(r'^@mirror\s+(x|xy|none)$')
FILE_TILESIZE = re.compile(r'^@tilesize\s+(\d+)$')


def format_pivot_value(value):
    """5.5 -> "5.5", 9.0 -> "9". XML에 9.0으로 적히면 읽는 사람이 헷갈린다."""

    if value == int(value):
        return u'%d' % int(value)

    return (u'%g' % value)

XML_COMMENT = u"""<!--
\t정적 프롭(필드 오브젝트) 스프라이트 정의.

\t한 파일이 애셋 캐시 한 단위다(PropSpriteSet). 프롭 하나가 방향 슬롯을 갖는다.

\tProp name : 조회 키. StaticPropActor 생성자에 넘기는 그 이름이다.
\t          Assets/PropData.xml의 name= 과는 다르다. 그쪽은 이 파일 자체의 이름이고,
\t          여기는 그 파일 안의 항목 이름이다.
\ttileSpan  : 타일 영역의 "벽 방향" 길이(타일 개수). 깊이는 언제나 1타일이다.
\t          정사각인 것은 타일 한 칸의 정의(8x8 / 12x12 / 16x16)이지 이 영역이 아니다.
\t          손으로 적지 않는다 - 정면 스프라이트 폭 / 타일 크기로 계산한다.
\t          스팬이 걸리는 축은 facing이 정한다(Up/Down이면 가로, Left/Right면 세로).
\t          타일 영역은 카메라가 돌아도 회전하지 않는다.

\tSprite dir : Up / Down / Left / Right. 액터가 "어디를 보는가"이지
\t          카메라 방향이 아니다. 화면에 그릴 슬롯은 엔진이
\t          RotateFacing(facing, 카메라 회전)으로 매 회전마다 다시 고른다.
\t          빠진 슬롯은 반대편으로 폴백한다(Down 없으면 Up, Right 없으면 Left).
\twidth/height : 선택. 픽셀맵과 대조해서 다르면 크래시한다.
\t          진실의 원천은 픽셀맵이고 이건 선언일 뿐이다.
\t          줄이 통째로 빠져 12x14가 12x13이 되는 실수는 이 선언이 있어야 걸린다.
\tpivot     : 선택. "x,y" 형태로 기준점에 놓일 스프라이트 안의 점(셀 단위).
\t          생략하면 정면 기준점(하단 중앙)을 90도씩 돌린 값이 쓰인다 -
\t            Up / Down : 하단 중앙  ((width-1)*0.5, height-1)
\t            Right     : 좌변 중앙  (0,             (height-1)*0.5)
\t            Left      : 우변 중앙  (width-1,       (height-1)*0.5)
\t          측면이 거울인 이유 - k=1에서 월드 아래쪽이 화면 왼쪽으로,
\t          k=3에서는 화면 오른쪽으로 간다. 한쪽으로 고정하면 회전 방향에 따라
\t          프롭이 한 타일씩 어긋난다.

\t기호는 SymbolPalette(Math/SymbolPalette.h)의 16개 대문자 + 투명('.')만 쓸 수 있다.
\t줄 앞뒤 공백은 로더가 버리므로 들여쓰기를 자유롭게 써도 된다.

\t이 파일은 Tools/convert_props.py가 생성한다. 손으로 고치지 말 것.
\tBOM 없는 UTF-8 / LF로 저장해야 한다.
-->"""


def fail(message):
    print(u'[convert_props] FAIL : ' + message)
    sys.exit(1)


def split_name(raw_name):
    """TOMB_A_FRONT -> ('TOMB_A', 'Up'). 접미사가 없으면 ('NAME', 'Up')."""

    for suffix, slot in DIRECTION_SUFFIX.items():
        if raw_name.endswith(suffix) and len(raw_name) > len(suffix):
            return raw_name[:-len(suffix)], slot

    return raw_name, 'Up'


def read_sprites(path):
    """원본을 읽어 (프롭 목록, 피벗 표, 파일 타일 크기)를 돌려준다.

    프롭 목록 : OrderedDict{프롭 이름: OrderedDict{슬롯: 행 목록}}
    피벗 표   : {(프롭 이름, 슬롯): (x, y)}  - @pivot을 적은 것만 들어간다
    타일 크기 : @tilesize 값. 없으면 None."""

    # 주석 줄에 한글이 있으므로 utf-8로 읽는다.
    # 픽셀 행이 ascii인지는 check_symbols가 따로 증명한다.
    try:
        with io.open(path, 'r', encoding='utf-8', newline='') as source_file:
            text = source_file.read()
    except IOError:
        fail(u'원본을 열 수 없다 : ' + path)
    except UnicodeDecodeError as error:
        fail(u'원본이 UTF-8이 아니다 : %s' % error)

    if text.startswith(u'﻿'):
        fail(u'원본에 BOM이 있다. BOM 없이 저장할 것.')

    if CR in text:
        fail(u'원본에 CR이 있다. LF만 있어야 한다.')

    lines = text.split(LF)

    props = OrderedDict()
    pivots = {}
    mirrors = {}
    file_tile_size = None

    index = 0
    total_lines = len(lines)

    while index < total_lines:
        stripped = lines[index].strip()

        # 파일 수준 지시자. 스프라이트 블록 밖 아무데나 둬도 된다.
        tile_matched = FILE_TILESIZE.match(stripped)

        if tile_matched is not None:
            if file_tile_size is not None:
                fail(u'@tilesize가 두 번 나온다 (%d번째 줄).' % (index + 1))

            file_tile_size = int(tile_matched.group(1))

            if file_tile_size <= 0:
                fail(u'@tilesize는 양수여야 한다 : %d' % file_tile_size)

            index += 1
            continue

        matched = SPRITE_HEADER.match(stripped)

        if matched is None:
            index += 1
            continue

        raw_name = matched.group(1)
        declared_width = int(matched.group(2))
        declared_height = int(matched.group(3))
        header_line = index + 1

        index += 1

        # 픽셀 행이 나올 때까지 주석/빈 줄/지시자를 건너뛴다.
        # 주석이 여러 줄이거나 없어도 견딘다.
        pivot = None
        mirror = None

        while index < total_lines:
            stripped = lines[index].strip()

            mirror_matched = SPRITE_MIRROR.match(stripped)

            if mirror_matched is not None:
                if mirror is not None:
                    fail(u'[%s] 에 @mirror가 두 번 나온다.' % raw_name)

                mirror = mirror_matched.group(1)
                index += 1
                continue

            pivot_matched = SPRITE_PIVOT.match(stripped)

            if pivot_matched is not None:
                if pivot is not None:
                    fail(u'[%s] 에 @pivot이 두 번 나온다.' % raw_name)

                pivot = (float(pivot_matched.group(1)), float(pivot_matched.group(2)))
                index += 1
                continue

            if stripped == '' or stripped.startswith('#'):
                index += 1
                continue

            break

        rows = []

        # 다음 블록 경계(구분선, 다음 헤더, 빈 줄)까지가 픽셀 행이다.
        while index < total_lines:
            stripped = lines[index].strip()

            if stripped == '' or stripped.startswith('-') or stripped.startswith('['):
                break

            rows.append(stripped)
            index += 1

        if not rows:
            fail(u'%d번째 줄 [%s] 블록에 픽셀 행이 없다.' % (header_line, raw_name))

        check_declared_size(raw_name, header_line, rows, declared_width, declared_height)

        prop_name, slot = split_name(raw_name)

        slots = props.setdefault(prop_name, OrderedDict())

        if slot in slots:
            fail(u'[%s] 의 %s 슬롯이 두 번 정의됐다.' % (prop_name, slot))

        slots[slot] = rows

        if pivot is not None:
            # 범위를 벗어난 피벗은 프롭을 엉뚱한 곳에 붙인다.
            # 화면만 봐서는 데이터 탓인지 알 수 없으므로 여기서 막는다.
            if not (0 <= pivot[0] <= declared_width - 1):
                fail(u'[%s] 의 @pivot x가 %g다. 0 ~ %d 안이어야 한다.'
                     % (raw_name, pivot[0], declared_width - 1))

            if not (0 <= pivot[1] <= declared_height - 1):
                fail(u'[%s] 의 @pivot y가 %g다. 0 ~ %d 안이어야 한다.'
                     % (raw_name, pivot[1], declared_height - 1))

            pivots[(prop_name, slot)] = pivot

        if mirror is not None:
            mirrors[(prop_name, slot)] = mirror

    if not props:
        fail(u'원본에서 스프라이트를 하나도 찾지 못했다. 헤더 형식이 "[NAME]  W x H" 인지 확인할 것.')

    return props, pivots, mirrors, file_tile_size


def check_declared_size(raw_name, header_line, rows, declared_width, declared_height):
    """선언한 W x H가 실제 픽셀맵과 맞는지 확인한다."""

    if len(rows) != declared_height:
        fail(u'[%s] (%d번째 줄) 이 %d행이라고 선언했는데 실제로는 %d행이다.'
             % (raw_name, header_line, declared_height, len(rows)))

    for row_index, row in enumerate(rows):
        if len(row) != declared_width:
            fail(u'[%s] %d번 행의 길이가 %d다. 선언은 %d다.'
                 % (raw_name, row_index, len(row), declared_width))


def check_symbols(props):
    """모든 픽셀맵의 기호가 SymbolPalette 안에 있는지 확인한다."""

    legal = set(SYMBOL_COLORS.keys()) | set([TRANSPARENT])

    for prop_name, slots in props.items():
        for slot, rows in slots.items():
            symbols = set(''.join(rows))
            unknown = symbols - legal

            # 엔진이 실제로 거부하는 조건을 그대로 검사한다.
            if unknown:
                fail(u'[%s/%s] 에 SymbolPalette에 없는 기호가 있다 : %s (허용 : %s 와 %r)'
                     % (prop_name, slot, sorted(unknown),
                        ''.join(sorted(SYMBOL_COLORS.keys())), TRANSPARENT))

            # XML 이스케이프가 필요 없다는 것을 증명한다.
            # 이게 보장되어야 쓰기가 문자열 이어붙이기 한 줄로 끝난다.
            escapable = symbols & set('<>&"' + chr(39))

            if escapable:
                fail(u'[%s/%s] 에 XML 이스케이프가 필요한 문자가 있다 : %s'
                     % (prop_name, slot, sorted(escapable)))


def compute_tile_span(prop_name, slots, tile_size):
    """정면 스프라이트 폭 / 타일 크기 = 벽 방향 타일 개수.

    손으로 적으면 그림과 데이터가 갈라진다. 폭이 곧 진실이므로 여기서 뽑는다."""

    # 정면이 없으면(측면만 있는 프롭) 있는 것 중 첫 슬롯의 폭을 쓴다.
    front = slots.get('Up') or slots.get('Down')
    rows = front if front is not None else next(iter(slots.values()))

    width = len(rows[0])

    if width % tile_size != 0:
        fail(u'[%s] 정면 폭이 %d다. 타일 크기 %d로 나눠떨어져야 한다.'
             % (prop_name, width, tile_size))

    return width // tile_size


def sorted_slots(slots):
    """슬롯을 Up/Down/Left/Right 순으로 돌려준다. 읽는 사람을 위한 순서일 뿐이다."""

    return [(slot, slots[slot]) for slot in SLOT_ORDER if slot in slots]


def write_xml(out_path, props, pivots, mirrors, desc, tile_size):
    with io.open(out_path, 'w', encoding='utf-8', newline=LF) as out_file:
        out_file.write(u'<?xml version="1.0" encoding="utf-8"?>' + LF)
        out_file.write(XML_COMMENT + LF)
        out_file.write(u'<PropSprites tileSize="%d" desc="%s">%s' % (tile_size, desc, LF))

        for prop_name, slots in props.items():
            span = compute_tile_span(prop_name, slots, tile_size)

            out_file.write(u'%s<Prop name="%s" tileSpan="%d">%s'
                           % (LF + TAB, prop_name, span, LF))

            for slot, rows in sorted_slots(slots):
                pivot = pivots.get((prop_name, slot))

                pivot_attr = u''

                if pivot is not None:
                    pivot_attr = u' pivot="%s,%s"' % (format_pivot_value(pivot[0]),
                                                      format_pivot_value(pivot[1]))

                mirror = mirrors.get((prop_name, slot))
                mirror_attr = u'' if mirror is None else (u' mirror="%s"' % mirror)

                out_file.write(u'%s%s<Sprite dir="%s" width="%d" height="%d"%s%s>%s'
                               % (TAB, TAB, slot, len(rows[0]), len(rows),
                                  pivot_attr, mirror_attr, LF))

                for row in rows:
                    out_file.write(u'%s%s' % (TAB * 3, row + LF))

                out_file.write(u'%s%s</Sprite>%s' % (TAB, TAB, LF))

            out_file.write(u'%s</Prop>%s' % (TAB, LF))

        out_file.write(LF + u'</PropSprites>' + LF)


def normalize_pixel_map(text):
    """엔진 로더(SpriteAnimationLoader::NormalizePixelMap)와 같은 처리.

    줄 앞뒤 공백 제거 + 빈 줄 제거. 되읽은 값을 원본과 비교하려면
    엔진이 할 처리를 여기서도 똑같이 해야 한다."""

    return [line.strip() for line in (text or '').split(LF) if line.strip() != '']


def verify(out_path, props, pivots, tile_size):
    """세 갈래로 확인한다. 하나라도 어긋나면 변환이 손실된 것이다."""

    # 1. 독립 파서로 되읽는다.
    #
    # 자작 리더로 확인하면 쓰기 버그와 읽기 버그가 서로 상쇄되어 통과해버린다.
    root = ElementTree.parse(out_path).getroot()

    if root.tag != 'PropSprites':
        fail(u'루트 태그가 PropSprites가 아니다 : %s' % root.tag)

    if root.get('tileSize') != str(tile_size):
        fail(u'되읽은 tileSize가 %s다. %d여야 한다.' % (root.get('tileSize'), tile_size))

    parsed = OrderedDict()

    for prop_node in root.findall('Prop'):
        name = prop_node.get('name')

        if not name:
            fail(u'name이 없는 Prop이 있다.')

        slots = OrderedDict()

        for sprite_node in prop_node.findall('Sprite'):
            slot = sprite_node.get('dir')

            if slot not in SLOT_ORDER:
                fail(u'[%s] 의 dir이 %r이다. Up/Down/Left/Right 중 하나여야 한다.' % (name, slot))

            rows = normalize_pixel_map(sprite_node.text)

            # 선언한 크기가 되읽은 픽셀맵과 맞는지. 엔진이 크래시하는 조건이다.
            declared_width = int(sprite_node.get('width', '0'))
            declared_height = int(sprite_node.get('height', '0'))

            if len(rows) != declared_height:
                fail(u'되읽은 [%s/%s] 가 %d행인데 선언은 %d다.'
                     % (name, slot, len(rows), declared_height))

            for row in rows:
                if len(row) != declared_width:
                    fail(u'되읽은 [%s/%s] 에 길이 %d인 행이 있다. 선언은 %d다.'
                         % (name, slot, len(row), declared_width))

            slots[slot] = rows

            # 피벗도 왕복시킨다. 속성 하나 빠뜨리면 그림이 통째로 밀린다.
            parsed_pivot = sprite_node.get('pivot')
            source_pivot = pivots.get((name, slot))

            if source_pivot is None:
                if parsed_pivot is not None:
                    fail(u'[%s/%s] 에 없어야 할 pivot이 있다 : %s' % (name, slot, parsed_pivot))
            else:
                expected = u'%s,%s' % (format_pivot_value(source_pivot[0]),
                                       format_pivot_value(source_pivot[1]))

                if parsed_pivot != expected:
                    fail(u'[%s/%s] 의 pivot이 %s다. %s여야 한다.'
                         % (name, slot, parsed_pivot, expected))

        parsed[name] = slots

    if list(parsed.keys()) != list(props.keys()):
        fail(u'프롭 목록이 원본과 다르다.%s  원본 : %s%s  결과 : %s'
             % (LF, list(props.keys()), LF, list(parsed.keys())))

    for prop_name, slots in props.items():
        parsed_slots = parsed[prop_name]

        if sorted(parsed_slots.keys()) != sorted(slots.keys()):
            fail(u'[%s] 의 슬롯이 다르다. 원본 : %s / 결과 : %s'
                 % (prop_name, sorted(slots.keys()), sorted(parsed_slots.keys())))

        for slot, rows in slots.items():
            if parsed_slots[slot] != rows:
                for index, (expected, actual) in enumerate(zip(rows, parsed_slots[slot])):
                    if expected != actual:
                        fail(u'[%s/%s] %d번 행이 원본과 다르다.%s  원본 : %r%s  결과 : %r'
                             % (prop_name, slot, index, LF, expected, LF, actual))

                fail(u'[%s/%s] 픽셀맵이 원본과 다르다.' % (prop_name, slot))

    # 2. 바이트 수준.
    with io.open(out_path, 'rb') as raw_file:
        raw = raw_file.read()

    if raw.startswith(bytes([0xEF, 0xBB, 0xBF])):
        fail(u'결과에 BOM이 있다. XmlParser가 파싱하지 못한다.')

    if bytes([13]) in raw:
        fail(u'결과에 CR이 있다. LF만 있어야 한다.')

    sprite_count = sum(len(slots) for slots in props.values())

    if raw.count(b'</Sprite>') != sprite_count:
        fail(u'Sprite 태그가 %d개다. %d개여야 한다.'
             % (raw.count(b'</Sprite>'), sprite_count))

    # 3. 히스토그램.
    source_histogram = Counter()
    parsed_histogram = Counter()

    for prop_name, slots in props.items():
        for slot, rows in slots.items():
            source_histogram.update(''.join(rows))
            parsed_histogram.update(''.join(parsed[prop_name][slot]))

    if source_histogram != parsed_histogram:
        fail(u'기호 히스토그램이 원본과 다르다.')

    return parsed_histogram


def main():
    parser = argparse.ArgumentParser(
        description='정적 프롭 스프라이트 원본을 프롭 XML로 변환한다.')
    parser.add_argument('source', help='원본 텍스트(graveyard_sprites.txt 형식)')
    parser.add_argument('output', help='생성할 프롭 XML 경로')
    parser.add_argument('--tile-size', type=int, default=None,
                        help='타일 한 칸의 셀 수. 원본에 @tilesize가 있으면 그쪽이 이긴다. 기본 12.')
    parser.add_argument('--desc', default=None,
                        help='desc 속성(파서가 읽지 않는 메모). 기본값은 원본 경로.')

    args = parser.parse_args()

    desc = args.desc or (u'원본 : %s' % args.source.replace(chr(92), '/'))

    props, pivots, mirrors, file_tile_size = read_sprites(args.source)
    check_symbols(props)

    # 원본에 적힌 값이 이긴다. 아트가 그려진 스케일이 곧 진실이고,
    # CLI는 @tilesize가 없는 옛 원본을 위한 보조 수단이다.
    tile_size = file_tile_size

    if tile_size is None:
        tile_size = args.tile_size if args.tile_size is not None else 12

    if tile_size <= 0:
        fail(u'타일 크기는 양수여야 한다 : %d' % tile_size)

    if file_tile_size is not None and args.tile_size is not None \
            and args.tile_size != file_tile_size:
        print('[convert_props] NOTE : --tile-size %d 는 무시한다. 원본의 @tilesize %d 를 쓴다.'
              % (args.tile_size, file_tile_size))

    write_xml(args.output, props, pivots, mirrors, desc, tile_size)
    histogram = verify(args.output, props, pivots, tile_size)

    for (prop_name, slot), mode in sorted(mirrors.items()):
        print('  mirror  : %s/%s -> %s' % (prop_name, slot, mode))

    sprite_count = sum(len(slots) for slots in props.values())
    total = sum(histogram.values())

    print('[convert_props] OK')
    print('  source  : %s' % args.source)
    print('  output  : %s' % args.output)
    print('  tile    : %d (%s)' % (tile_size, 'from @tilesize' if file_tile_size else 'default'))
    print('  props   : %d (sprites %d, cells %d)' % (len(props), sprite_count, total))

    for prop_name, slots in props.items():
        shapes = ', '.join('%s %dx%d' % (slot, len(rows[0]), len(rows))
                           for slot, rows in sorted_slots(slots))
        span = compute_tile_span(prop_name, slots, tile_size)
        marks = ' '.join('%s@%s,%s' % (slot,
                                       format_pivot_value(pivots[(prop_name, slot)][0]),
                                       format_pivot_value(pivots[(prop_name, slot)][1]))
                         for slot, _ in sorted_slots(slots)
                         if (prop_name, slot) in pivots)

        print('    %-16s span=%d  %s%s' % (prop_name, span, shapes,
                                           ('   pivot ' + marks) if marks else ''))

    print('  symbols :')

    for symbol, count in histogram.most_common():
        name = 'transparent' if symbol == TRANSPARENT else ('Color::' + SYMBOL_COLORS[symbol])
        print('    %s  %7d  %5.2f%%  -> %s' % (symbol, count, count * 100.0 / total, name))


if __name__ == '__main__':
    main()
