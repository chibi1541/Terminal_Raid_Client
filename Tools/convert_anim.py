# -*- coding: utf-8 -*-
"""
캐릭터 스프라이트 원본을 클라이언트 애니메이션 XML로 변환한다.

    python Tools/convert_anim.py <source.txt> <output.xml> [--inherit FILE#CLIP[=x,y]]

예)
    python Tools/convert_anim.py Assets/Knight_sprite.txt Assets/Knight.anim.xml
        --inherit Assets/TestActor.anim.xml#Rolling=4,7

convert_props.py / convert_level.py와 같은 방침이다.
    - 빌드에 넣지 않는다. 아트를 다시 뽑았을 때만 돌리고 결과 XML을 커밋한다.
    - BOM 없는 UTF-8 / LF. io.open(encoding='utf-8', newline='\\n')이 API로 보장한다.
    - ★ 원본의 이름 규칙이 전부를 정한다 ★ 옆에 끼고 다녀야 하는 설정 파일이 없다.
      convert_props.py가 "_FRONT / _SIDE" 접미사로 방향 슬롯을 정하는 것과 같은 방식이다.

입력 형식 (Assets/Knight_sprite.txt)

    ################################################################
    # CLIP 1 / 8 - ROW 1 · A - 앞모습 대기        <- 클립 헤더. 마지막 칸이 이름이다.
    # frames: 1   frame size: 8 x 8 px
    ################################################################

    -- frame 1/1  (sheet id #00)  8x8px          <- 프레임 헤더

       [
         "LLLLL.L.",                             <- 따옴표로 감싼 픽셀 행
         "BBBBB.L.",
       ]

    프레임 픽셀은 두 가지 표기를 모두 받는다.

        "LLLLL.L.",     따옴표 배열 (지금 시트)
        0 | LLLLL.L.    "행번호 | 픽셀" (예전 시트)

    한 프레임에 둘 다 있으면 같은 그림을 두 번 적은 것이므로 배열 쪽만 쓴다.
    파일 앞머리의 CLIP INDEX 표는 무시한다 - 거기 적힌 클립 개수가 실제와 어긋나 있는
    경우가 있어서, 진실의 원천은 언제나 본문의 "# CLIP" 헤더다.

클립 이름 규칙

    "앞모습 대기" 처럼 [방향][동작] 두 낱말이다. 순서는 상관없다.

        방향   앞모습/정면 -> Down     뒷모습/후면 -> Up     측면 -> Side
        동작   대기 -> Idle    이동 -> Walk    공격 -> Attack    구르기 -> Rolling

    "측면"이 Left/Right가 아니라 Side인 이유 - 아트가 오른쪽을 보게 그려져 있고
    반대편은 엔진이 좌우 반전으로 채운다(Asset/SpriteAnimationLoader.h 주석 참고).

    알아보지 못하는 이름이 있으면 그 자리에서 멈춘다. 조용히 건너뛰면 아트 한 벌이
    통째로 빠진 채로 결과가 나오고, 화면을 보기 전까지 아무도 모른다.

    fps와 loop는 동작이 정한다(아래 ACTION_TIMING). 원본에 적을 자리가 없기도 하고,
    같은 동작이 캐릭터마다 다른 속도로 도는 편이 오히려 실수에 가깝다.

pivot 규칙

    ★ 방향마다 하나씩 정하고, 그 방향의 모든 클립이 공유한다 ★

    액터 위치에 놓이는 점이라 대기와 이동이 서로 다른 값을 쓰면 걷기 시작할 때마다
    캐릭터가 옆으로 한 칸 튄다. 걸음이 좌우로 흔들리는 것보다 그쪽이 훨씬 눈에 띈다.

    값은 발 위치에서 뽑는다. 그 방향에 대기(Idle) 클립이 있으면 그 첫 프레임의 발을
    쓰고(서 있는 자세가 기준이다), 없으면 그 방향 모든 프레임의 평균을 쓴다.
    x는 반드시 정수다 - 반쪽 값은 박스 중심 (W-1)/2와 정확히 같을 때만 안전하고,
    아니면 좌우 반전에서 한 칸 튄다(AnimInstance::GetCurrentPivotCell 참고).

    자동으로 잡힌 값이 마음에 안 들면 --pivot Up=4 로 방향 하나만 덮어쓴다.

--inherit

    원본 시트에 없는 클립을 다른 anim.xml에서 통째로 가져온다. 주석과 Notify가 그대로
    따라온다. Knight의 구르기가 여기 해당한다 - 아직 전용 아트가 없어서 TestActor 것을
    빌려 쓰는데, 그 클립의 RollEnd 노티파이가 빠지면 구르기에서 못 빠져나온다.
"""

import argparse
import io
import os
import re

# "-- frame 1/2  (sheet id #00)  8x8px"
FRAME_HEADER_RE = re.compile(r'^--\s*frame\b.*\(sheet id #([0-9A-Za-z]+)\)')

# "# CLIP 1 / 8 - ROW 1 · A - 앞모습 대기"
CLIP_HEADER_RE = re.compile(r'^#\s*CLIP\s+\d+\s*/\s*\d+\s*(?:—|-)\s*(.+)$')

# '     "LLLLL.L.",'  (지금 시트)
FRAME_QUOTED_ROW_RE = re.compile(r'^\s*"([^"]*)"\s*,?\s*$')

# "    0 | LLLLL.L."  (예전 시트)
FRAME_BARRED_ROW_RE = re.compile(r'^\s*\d+\s*\|\s*(\S+)\s*$')

# 애니메이션 XML에서 클립 하나를 통째로 떼어낸다.
CLIP_BLOCK_RE = r'(?ms)^([ \t]*)<Clip\s+name="%s".*?</Clip>[ \t]*$'

# SymbolPalette(Math/SymbolPalette.h)의 16개 대문자 + 투명.
VALID_SYMBOLS = set('KDLBFGNWTCROYVUP.')

FACING_WORDS = {
    '앞모습': 'Down',
    '정면': 'Down',
    '뒷모습': 'Up',
    '후면': 'Up',
    '측면': 'Side',
}

ACTION_WORDS = {
    '대기': 'Idle',
    '이동': 'Walk',
    '공격': 'Attack',
    '구르기': 'Rolling',
}

# 동작 -> (fps, loop).
ACTION_TIMING = {
    'Idle': (3, True),
    'Walk': (8, True),
    'Attack': (12, False),
    'Rolling': (20, False),
}

# 출력 순서. 사람이 읽을 때 같은 동작끼리 붙어 있는 편이 낫다.
ACTION_ORDER = ['Idle', 'Walk', 'Attack', 'Rolling']
FACING_ORDER = ['Down', 'Side', 'Up', 'Right', 'Left']


class Clip(object):
    def __init__(self, label):
        self.label = label
        self.name = None
        self.facing = None

        # [sheet_id, 배열표기 행들, 막대표기 행들]
        self.frames = []

    def rows_list(self):
        # 배열 표기가 있으면 그쪽이 진실의 원천이다.
        return [quoted if quoted else barred for _, quoted, barred in self.frames]


def parse_label(label):
    """'앞모습 대기' -> ('Idle', 'Down'). 낱말 순서는 상관없다."""
    facing = None
    action = None

    for word in label.replace('·', ' ').split():
        if word in FACING_WORDS:
            facing = FACING_WORDS[word]
        elif word in ACTION_WORDS:
            action = ACTION_WORDS[word]

    if action is None:
        raise ValueError('클립 이름 "%s"에서 동작을 못 읽었다. 쓸 수 있는 낱말: %s'
                         % (label, ' '.join(sorted(ACTION_WORDS))))

    if facing is None:
        raise ValueError('클립 이름 "%s"에서 방향을 못 읽었다. 쓸 수 있는 낱말: %s'
                         % (label, ' '.join(sorted(FACING_WORDS))))

    return action, facing


def read_clips(source_path):
    """원본을 클립 목록으로. 본문의 '# CLIP' 헤더가 경계다."""
    clips = []
    current_clip = None
    current_frame = None

    for line in io.open(source_path, encoding='utf-8'):
        line = line.rstrip('\n')
        stripped = line.strip()

        clip_header = CLIP_HEADER_RE.match(stripped)

        if clip_header is not None:
            # "ROW 1 · A - 앞모습 대기" 처럼 칸이 더 있으면 마지막 칸이 이름이다.
            label = re.split(r'—|-', clip_header.group(1))[-1].strip()

            current_clip = Clip(label)
            current_frame = None
            clips.append(current_clip)

            continue

        frame_header = FRAME_HEADER_RE.match(stripped)

        if frame_header is not None:
            if current_clip is None:
                raise ValueError('클립 헤더보다 프레임이 먼저 나왔다 (#%s)' % frame_header.group(1))

            current_frame = [frame_header.group(1), [], []]
            current_clip.frames.append(current_frame)

            continue

        if current_frame is None:
            continue

        row = FRAME_QUOTED_ROW_RE.match(line)
        bucket = 1

        if row is None:
            row = FRAME_BARRED_ROW_RE.match(line)
            bucket = 2

        if row is None:
            continue

        pixels = row.group(1)

        # 팔레트 밖 기호는 엉뚱한 색으로 나오거나 조용히 투명이 된다.
        # 원본을 다시 뽑을 때마다 눈으로 확인할 수는 없으니 여기서 잡는다.
        unknown = set(pixels) - VALID_SYMBOLS

        if unknown:
            raise ValueError('sheet id #%s에 팔레트에 없는 기호 %s'
                             % (current_frame[0], sorted(unknown)))

        current_frame[bucket].append(pixels)

    return clips


def validate_frames(clip):
    """프레임 크기가 전부 같은지 본다. 엔진도 같은 불변식을 강제한다."""
    rows_list = clip.rows_list()

    if not rows_list:
        raise ValueError('%s: 프레임이 하나도 없다' % clip.label)

    width = len(rows_list[0][0])
    height = len(rows_list[0])

    for index, rows in enumerate(rows_list):
        if not rows:
            raise ValueError('%s: sheet id #%s가 비어 있다' % (clip.label, clip.frames[index][0]))

        for row in rows:
            if len(row) != width:
                raise ValueError('%s: sheet id #%s의 줄 길이가 어긋난다'
                                 % (clip.label, clip.frames[index][0]))

        if len(rows) != height:
            raise ValueError('%s: 프레임마다 크기가 다르다 (#%s)'
                             % (clip.label, clip.frames[index][0]))

    return rows_list, width, height


def feet_center(rows):
    """맨 아래 그림이 있는 행에서 발의 가운데 열."""
    for row in reversed(rows):
        columns = [index for index, symbol in enumerate(row) if symbol != '.']

        if columns:
            return (columns[0] + columns[-1]) * 0.5

    raise ValueError('빈 프레임에서는 발 위치를 잡을 수 없다')


def resolve_pivots(clips, overrides):
    """방향 -> pivot 문자열. 그 방향의 모든 클립이 같은 값을 쓴다."""
    by_facing = {}

    for clip in clips:
        by_facing.setdefault(clip.facing, []).append(clip)

    pivots = {}

    for facing, group in by_facing.items():
        height = len(group[0].rows_list()[0])

        if facing in overrides:
            pivots[facing] = '%d,%d' % (overrides[facing], height - 1)

            continue

        # 서 있는 자세가 기준이다. 대기 클립이 있으면 그 첫 프레임을 쓴다.
        rest = next((clip for clip in group if clip.name == 'Idle'), None)

        if rest is not None:
            center = feet_center(rest.rows_list()[0])
        else:
            centers = [feet_center(rows) for clip in group for rows in clip.rows_list()]
            center = sum(centers) / len(centers)

        # x는 정수여야 한다. 반올림은 한 번만.
        pivots[facing] = '%d,%d' % (int(center + 0.5), height - 1)

    return pivots


def build_clip_block(clip, pivot):
    rows_list, width, height = validate_frames(clip)
    fps, loop = ACTION_TIMING[clip.name]

    lines = [
        '\t<!-- %s -->' % clip.label,
        '\t<Clip name="%s" facing="%s" fps="%d" loop="%s" width="%d" height="%d" pivot="%s">'
        % (clip.name, clip.facing, fps, 'true' if loop else 'false', width, height, pivot),
    ]

    for rows in rows_list:
        lines.append('\t\t<Frame>')

        for row in rows:
            lines.append('\t\t\t' + row)

        lines.append('\t\t</Frame>')

    lines.append('\t</Clip>')

    return '\n'.join(lines)


def inherit_clip_block(spec):
    """다른 anim.xml에서 클립 블록을 통째로 가져온다. 주석과 Notify가 그대로 따라온다."""
    reference_path, _, remainder = spec.partition('#')
    clip_name, _, pivot = remainder.partition('=')

    if not reference_path or not clip_name:
        raise ValueError('--inherit 형식은 FILE#CLIP[=x,y] 이다 (받은 값 "%s")' % spec)

    text = io.open(reference_path, encoding='utf-8').read()
    match = re.search(CLIP_BLOCK_RE % re.escape(clip_name), text)

    if match is None:
        raise ValueError('%s에서 클립 "%s"를 찾지 못했다' % (reference_path, clip_name))

    block = match.group(0)
    indent = match.group(1)

    # 들여쓰기를 한 단계로 맞춘다.
    if indent:
        block = '\n'.join(
            line[len(indent):] if line.startswith(indent) else line
            for line in block.split('\n'))

    block = '\n'.join(('\t' + line) if line.strip() else line for line in block.split('\n'))

    # pivot만 덮어쓴다. 물려받은 아트에 정수 피벗 규칙을 적용하기 위한 자리다.
    if pivot:
        block = re.sub(r'pivot="[^"]*"', 'pivot="%s"' % pivot, block, count=1)

    return clip_name, block


def build_document(blocks, source_path, character_name):
    header = '''<?xml version="1.0" encoding="utf-8"?>
<!--
\t%s 스프라이트 애니메이션.

\t★ 이 파일은 생성물이다 ★
\t손으로 고치지 말고 원본을 고친 뒤 다시 뽑을 것.
\t    원본   : %s
\t    변환기 : Tools/convert_anim.py

\tClip name   : 상태 머신의 State가 지목하는 "논리" 이름.
\tClip facing : 이 그림이 그려진 시점. Up / Right / Down / Left / Side.
\t              같은 name을 facing만 다르게 여러 번 쓰면 하나의 방향 세트가 된다.
\t              빠진 방향은 엔진이 로드 시점에 폴백으로 채운다
\t              (규칙은 Asset/SpriteAnimationLoader.h 주석 참고).
\tpivot       : 액터 위치에 놓일 스프라이트 안의 점. 방향마다 하나이고 x는 정수다 -
\t              클립마다 다르면 상태가 바뀔 때 캐릭터가 옆으로 튀고,
\t              반쪽 값은 좌우 반전에서 한 칸 튄다.

\t기호는 SymbolPalette(Math/SymbolPalette.h)의 16개 대문자 + 투명('.')만 쓸 수 있다.

\t이 파일은 UTF-8(BOM 없이)로 저장할 것.
-->
<SpriteAnimation>
''' % (character_name, source_path.replace('\\', '/'))

    document = header + '\n' + '\n\n'.join(blocks) + '\n\n</SpriteAnimation>\n'

    # 물려받은 블록에 원본의 줄 끝 공백이 섞여 들어온다. 생성물에는 남기지 않는다.
    return '\n'.join(line.rstrip() for line in document.split('\n'))


def parse_pivot_override(text):
    facing, _, value = text.partition('=')

    if not facing or not value:
        raise ValueError('--pivot 형식은 FACING=x 이다 (받은 값 "%s")' % text)

    return facing, int(value)


def main():
    parser = argparse.ArgumentParser(description='스프라이트 원본 -> 애니메이션 XML')
    parser.add_argument('source', help='아트 원본 txt')
    parser.add_argument('output', help='만들 anim.xml')
    parser.add_argument('--inherit', action='append', default=[], metavar='FILE#CLIP[=x,y]',
                        help='원본에 없는 클립을 다른 anim.xml에서 가져온다')
    parser.add_argument('--pivot', action='append', default=[], metavar='FACING=x',
                        help='자동으로 잡힌 방향별 pivot x를 덮어쓴다')

    args = parser.parse_args()

    clips = read_clips(args.source)

    if not clips:
        raise ValueError('%s에서 클립을 하나도 못 읽었다' % args.source)

    for clip in clips:
        clip.name, clip.facing = parse_label(clip.label)

    # 같은 (이름, 방향)이 두 번이면 엔진이 로드할 때 크래시한다. 여기서 먼저 잡는다.
    seen = {}

    for clip in clips:
        key = (clip.name, clip.facing)

        if key in seen:
            raise ValueError('%s / %s 가 "%s"와 "%s" 두 곳에 있다'
                             % (clip.name, clip.facing, seen[key], clip.label))

        seen[key] = clip.label

    overrides = dict(parse_pivot_override(text) for text in args.pivot)
    pivots = resolve_pivots(clips, overrides)

    def sort_key(clip):
        return (ACTION_ORDER.index(clip.name), FACING_ORDER.index(clip.facing))

    ordered = sorted(clips, key=sort_key)
    blocks = [build_clip_block(clip, pivots[clip.facing]) for clip in ordered]

    inherited = []

    for spec in args.inherit:
        clip_name, block = inherit_clip_block(spec)
        inherited.append(clip_name)
        blocks.append(block)

    character_name = os.path.basename(args.output).split('.')[0]

    io.open(args.output, 'w', encoding='utf-8', newline='\n').write(
        build_document(blocks, args.source, character_name))

    print('%s : 클립 %d개(물려받음 %d개) / 프레임 %d개'
          % (os.path.basename(args.output), len(blocks), len(inherited),
             sum(len(clip.frames) for clip in clips)))
    print('  pivot : ' + ', '.join('%s=%s' % (facing, pivots[facing]) for facing in sorted(pivots)))

    for clip in ordered:
        print('  %-8s %-5s %d프레임  (%s)' % (clip.name, clip.facing, len(clip.frames), clip.label))

    for clip_name in inherited:
        print('  %-8s %-5s 물려받음' % (clip_name, '-'))


if __name__ == '__main__':
    main()
