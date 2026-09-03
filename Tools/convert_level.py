# -*- coding: utf-8 -*-
"""
도트 아트 원본을 클라이언트 타일맵 레벨 XML로 변환한다.

    python Tools/convert_level.py <source.txt> <output.xml> [--level-id NAME] [--desc TEXT]

예)
    python Tools/convert_level.py Assets/level-dot-12x12.txt Assets/Cemetery.level.xml

빌드에는 넣지 않는다. 아트를 다시 뽑았을 때만 돌리고 결과 XML을 커밋해서,
런타임이 원본 .txt에 의존하지 않게 한다.

PowerShell로 쓰지 않는 이유:
    Out-File / Set-Content 는 BOM이나 ANSI로 쓴다. BOM은 XmlParser에서 하드 실패다.
    python의 open(encoding='utf-8', newline='\\n')은 BOM 없는 LF가 기본 동작이라
    두 요구사항이 API 차원에서 보장된다(기억에 의존하지 않는다).

입력 형식은 두 가지를 자동으로 알아본다.
    1. 순수 그리드   - 한 줄이 한 행. (level-dot-12x12.txt)
    2. C 배열 리터럴 - static const char* kLevel[] = { "...", ... };
       (cemetery-level-dot-8x8.txt 처럼 코드에서 바로 뽑아온 것)

TODO : 애니메이션 등 다른 애셋 변환기가 필요해지면 아래 네 조각을 나눈다.
       읽기(read_grid) / 검증(check_rectangle, check_symbols) / 쓰기(write_xml) / 확인(verify).
       기호표(SYMBOL_COLORS)와 확인 절차는 그대로 공유할 수 있다.
       지금은 레벨 하나뿐이라 파일 하나로 둔다.
"""

import argparse
import io
import os
import re
import sys
import xml.etree.ElementTree as ElementTree
from collections import Counter

# 엔진의 SymbolPalette(Math/SymbolPalette.h)와 같아야 한다.
# 여기에 없는 기호가 섞이면 LevelMap이 로드 단계에서 거부한다.
SYMBOL_COLORS = {
    'K': 'Black',     'D': 'DarkGray',  'L': 'Gray',      'B': 'White',
    'F': 'DarkGreen', 'G': 'Green',
    'N': 'DarkBrown', 'W': 'Brown',     'T': 'Tan',
    'C': 'DarkRed',   'R': 'Red',
    'O': 'Orange',    'Y': 'Yellow',
    'V': 'DarkBlue',  'U': 'Blue',
    'P': 'Purple',
}

# SymbolPalette::TransparentSymbol. 그리지 않고 건너뛰는 칸이다.
TRANSPARENT = '.'

CR = chr(13)
LF = chr(10)
TAB = chr(9)

C_ARRAY_HEADER = re.compile(r'^static const char[^=]*=[^{]*\{$')
C_ARRAY_FOOTER = re.compile(r'^\};?$')
C_ARRAY_ROW = re.compile(r'^"([^"]*)",?$')

XML_COMMENT = u"""<!--
\t클라이언트 전용 타일맵 레벨 데이터.

\t이 파일은 "보이는 것"만 정의한다. 이동 가능 여부는 서버가 자기
\t레벨 데이터('#' = 막힘)로 판정하며, 서버는 이 파일을 읽지 않는다.
\t그래서 blocked / tileSize / collision 이 없다. 앞으로도 넣지 말 것 -
\t같은 정보가 두 파일에 있으면 반드시 갈라진다.
\t(서버의 tileSize는 길찾기용 묶음이고, 여기의 한 칸은 콘솔 셀 하나다.
\t 서로 다른 개념이니 두 파일을 "통일"하려 들지 말 것)

\tlevelId : 콘텐츠 식별자. 로드 실패 진단 메시지에 찍힌다.
\t          조회 키가 아니다 - 조회는 Assets/LevelData.xml의 name= 으로 한다.
\twidth   : 한 행의 문자 수. 모든 Row가 정확히 이 길이여야 한다.
\theight  : Row 개수. 정확히 일치해야 한다.
\tdesc    : 파서가 읽지 않는다. 코드에 이 속성을 읽는 곳이 없고 앞으로도 두지 않는다.
\t          순수한 사람용 메모다. 여기에 무엇을 쓰든 게임 동작은 바뀌지 않는다.

\tRow : SymbolPalette(Math/SymbolPalette.h)의 기호 한 글자 = 콘솔 셀 한 칸.
\t      '.'(TransparentSymbol)은 그리지 않고 건너뛰는 칸이다.

\t      여는 태그와 내용 사이에 공백이나 줄바꿈을 넣지 말 것.
\t      rapidxml은 Row 태그 바로 뒤에 오는 공백을 그대로 값에 포함시켜서
\t      길이 검사에서 바로 실패한다.

\t이 파일은 Tools/convert_level.py가 생성한다. 손으로 고치지 말 것.
\tBOM 없는 UTF-8 / LF로 저장해야 한다.
-->"""


def fail(message):
    print(u'[convert_level] FAIL : ' + message)
    sys.exit(1)


def read_grid(path):
    """원본을 읽어 (행 목록, 형식 이름)을 돌려준다. 형식은 자동으로 알아본다."""

    # ascii로 읽는다. BOM이나 이상한 바이트가 섞여 있으면 여기서 바로 터진다.
    try:
        with io.open(path, 'r', encoding='ascii', newline='') as source_file:
            text = source_file.read()
    except IOError:
        fail(u'원본을 열 수 없다 : ' + path)
    except UnicodeDecodeError as error:
        fail(u'원본이 순수 ascii가 아니다(BOM이나 비-ascii 문자) : %s' % error)

    if CR in text:
        fail(u'원본에 CR이 있다. LF만 있어야 한다.')

    lines = text.split(LF)

    # 마지막 개행 뒤의 빈 조각은 버린다.
    while lines and lines[-1] == '':
        lines.pop()

    if not lines:
        fail(u'원본이 비어 있다.')

    # --- C 배열 리터럴이면 껍데기를 벗긴다 ---
    if C_ARRAY_HEADER.match(lines[0].strip()):
        if not C_ARRAY_FOOTER.match(lines[-1].strip()):
            fail(u'C 배열인데 마지막 줄이 닫는 중괄호가 아니다 : %r' % lines[-1])

        rows = []

        for index, line in enumerate(lines[1:-1]):
            matched = C_ARRAY_ROW.match(line.strip())

            if matched is None:
                fail(u'%d번 행이 따옴표로 감싼 문자열이 아니다 : %r' % (index, line[:40]))

            rows.append(matched.group(1))

        return rows, 'C array literal'

    # --- 순수 그리드 ---
    return lines, 'plain grid'


def check_rectangle(rows):
    """모든 행의 길이가 같은지 확인하고 (width, height)를 돌려준다."""

    widths = set(len(row) for row in rows)

    if len(widths) != 1:
        # 어느 행이 튀는지 짚어준다.
        # 288줄에서 "길이가 다르다"만 나오면 손으로 못 찾는다.
        common = Counter(len(row) for row in rows).most_common(1)[0][0]

        for index, row in enumerate(rows):
            if len(row) != common:
                fail(u'%d번 행의 길이가 %d다. 나머지는 %d다.' % (index, len(row), common))

    return widths.pop(), len(rows)


def check_symbols(rows):
    symbols = set(''.join(rows))
    legal = set(SYMBOL_COLORS.keys()) | set([TRANSPARENT])
    unknown = symbols - legal

    # 엔진이 실제로 거부하는 조건을 그대로 검사한다.
    # 여기서 통과시키면 LevelMap이 로드 단계에서 되돌려보낸다.
    if unknown:
        fail(u'SymbolPalette에 없는 기호가 있다 : %s (허용 : %s 와 %r)'
             % (sorted(unknown), ''.join(sorted(SYMBOL_COLORS.keys())), TRANSPARENT))

    # XML 이스케이프가 필요 없다는 것을 증명한다.
    # 이게 보장되어야 아래 쓰기가 문자열 이어붙이기 한 줄로 끝난다.
    escapable = symbols & set('<>&"' + chr(39))

    if escapable:
        fail(u'XML에서 이스케이프가 필요한 문자가 있다 : %s' % sorted(escapable))


def write_xml(out_path, rows, width, height, level_id, desc):
    with io.open(out_path, 'w', encoding='utf-8', newline=LF) as out_file:
        out_file.write(u'<?xml version="1.0" encoding="utf-8"?>' + LF)
        out_file.write(XML_COMMENT + LF)
        out_file.write(u'<Level levelId="%s" width="%d" height="%d" desc="%s">%s'
                       % (level_id, width, height, desc, LF))

        for row in rows:
            out_file.write(u'%s<Row>%s</Row>%s' % (TAB, row, LF))

        out_file.write(u'</Level>' + LF)


def verify(out_path, rows, width, height, level_id):
    """세 갈래로 확인한다. 하나라도 어긋나면 변환이 손실된 것이다."""

    # 1. 독립 파서로 되읽는다.
    #
    # 자작 리더로 확인하면 쓰기 쪽 버그와 읽기 쪽 버그가 같이 상쇄되어 통과해버린다.
    # 다른 구현으로 읽어야 이스케이프 실수나 공백 유출이 드러난다.
    root = ElementTree.parse(out_path).getroot()

    if root.tag != 'Level':
        fail(u'루트 태그가 Level이 아니다 : %s' % root.tag)

    if root.get('levelId') != level_id:
        fail(u'levelId가 다르다 : %s' % root.get('levelId'))

    if root.get('width') != str(width) or root.get('height') != str(height):
        fail(u'헤더 속성이 다르다 : width=%s height=%s'
             % (root.get('width'), root.get('height')))

    parsed = [node.text for node in root.findall('Row')]

    if len(parsed) != height:
        fail(u'되읽은 Row가 %d개다. %d개여야 한다.' % (len(parsed), height))

    if parsed != rows:
        for index, (expected, actual) in enumerate(zip(rows, parsed)):
            if expected != actual:
                fail(u'%d번 행이 원본과 다르다.%s  원본 : %r%s  결과 : %r'
                     % (index, LF, expected[:40], LF, (actual or '')[:40]))

        fail(u'행 내용이 원본과 다르다.')

    # 2. 바이트 수준.
    with io.open(out_path, 'rb') as raw_file:
        raw = raw_file.read()

    if raw.startswith(bytes([0xEF, 0xBB, 0xBF])):
        fail(u'결과에 BOM이 있다. XmlParser가 파싱하지 못한다.')

    if bytes([13]) in raw:
        fail(u'결과에 CR이 있다. LF만 있어야 한다.')

    if raw.count(b'<Row>') != height:
        fail(u'Row 태그가 %d개다. %d개여야 한다.' % (raw.count(b'<Row>'), height))

    # 탭 1 + <Row> 5 + 내용 + </Row> 6
    expected_length = 1 + 5 + width + 6

    for line in raw.decode('utf-8').split(LF):
        if line.startswith(TAB + '<Row>') and len(line) != expected_length:
            fail(u'Row 줄 길이가 %d다. %d여야 한다(태그 안에 공백이 섞였을 수 있다).'
                 % (len(line), expected_length))

    # 3. 히스토그램.
    if Counter(''.join(rows)) != Counter(''.join(parsed)):
        fail(u'기호 히스토그램이 원본과 다르다.')

    return Counter(''.join(parsed))


def default_level_id(out_path):
    """Cemetery.level.xml -> Cemetery"""

    name = os.path.basename(out_path)

    for suffix in ('.level.xml', '.xml'):
        if name.endswith(suffix):
            return name[:-len(suffix)]

    return name


def main():
    parser = argparse.ArgumentParser(
        description='도트 아트 원본을 타일맵 레벨 XML로 변환한다.')
    parser.add_argument('source', help='원본 텍스트(순수 그리드 또는 C 배열 리터럴)')
    parser.add_argument('output', help='생성할 레벨 XML 경로')
    parser.add_argument('--level-id', default=None,
                        help='levelId 속성. 기본값은 출력 파일 이름에서 뽑는다.')
    parser.add_argument('--desc', default=None,
                        help='desc 속성(파서가 읽지 않는 메모). 기본값은 원본 경로.')

    args = parser.parse_args()

    level_id = args.level_id or default_level_id(args.output)
    desc = args.desc or (u'원본 : %s' % args.source.replace(chr(92), '/'))

    rows, source_kind = read_grid(args.source)
    width, height = check_rectangle(rows)
    check_symbols(rows)

    write_xml(args.output, rows, width, height, level_id, desc)
    histogram = verify(args.output, rows, width, height, level_id)

    total = sum(histogram.values())

    if total != width * height:
        fail(u'셀 수가 %d다. %d여야 한다.' % (total, width * height))

    print('[convert_level] OK')
    print('  source  : %s (%s)' % (args.source, source_kind))
    print('  output  : %s' % args.output)
    print('  levelId : %s' % level_id)
    print('  size    : %d x %d = %d cells' % (width, height, total))

    for symbol, count in histogram.most_common():
        name = 'transparent' if symbol == TRANSPARENT else ('Color::' + SYMBOL_COLORS[symbol])
        print('  %s  %7d  %5.2f%%  -> %s' % (symbol, count, count * 100.0 / total, name))


if __name__ == '__main__':
    main()
