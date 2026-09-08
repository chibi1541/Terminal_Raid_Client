#include "pch.h"
#include "UI/PixelText.h"

#include <array>
#include <cctype>
#include <unordered_map>

namespace
{
	// 큰 폰트 5x7. 메인 메뉴에 나오는 글자만:
	//   TERMINAL RAID / GAME START / EXIT GAME -> A D E G I L M N R S T X + 공백.
	using BigGlyph = std::array<const char*, PixelText::kBigH>;

	const std::unordered_map<char, BigGlyph>& BigFont()
	{
		static const std::unordered_map<char, BigGlyph> table = {
			{ ' ', { ".....", ".....", ".....", ".....", ".....", ".....", "....." } },
			{ 'A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
			{ 'D', { "####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####." } },
			{ 'E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
			{ 'G', { ".####", "#....", "#....", "#.###", "#...#", "#...#", ".####" } },
			{ 'I', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "#####" } },
			{ 'L', { "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
			{ 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" } },
			{ 'N', { "#...#", "##..#", "#.#.#", "#.#.#", "#..##", "#...#", "#...#" } },
			{ 'R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
			{ 'S', { ".####", "#....", "#....", ".###.", "....#", "....#", "####." } },
			{ 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
			{ 'X', { "#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#" } },
		};
		return table;
	}

	// 작은 폰트 4x6. 일시정지 메뉴에 나오는 글자만:
	//   CONTINUE GAME / EXIT GAME -> A C E G I M N O T U X + 공백.
	using SmallGlyph = std::array<const char*, PixelText::kSmallH>;

	const std::unordered_map<char, SmallGlyph>& SmallFont()
	{
		static const std::unordered_map<char, SmallGlyph> table = {
			{ ' ', { "....", "....", "....", "....", "....", "...." } },
			{ 'A', { ".##.", "#..#", "#..#", "####", "#..#", "#..#" } },
			{ 'C', { ".###", "#...", "#...", "#...", "#...", ".###" } },
			{ 'E', { "####", "#...", "###.", "#...", "#...", "####" } },
			{ 'G', { ".###", "#...", "#.##", "#..#", "#..#", ".###" } },
			{ 'I', { "####", ".##.", ".##.", ".##.", ".##.", "####" } },
			{ 'M', { "#..#", "####", "####", "#..#", "#..#", "#..#" } },
			{ 'N', { "#..#", "##.#", "##.#", "#.##", "#.##", "#..#" } },
			{ 'O', { ".##.", "#..#", "#..#", "#..#", "#..#", ".##." } },
			{ 'T', { "####", ".##.", ".##.", ".##.", ".##.", ".##." } },
			{ 'U', { "#..#", "#..#", "#..#", "#..#", "#..#", ".##." } },
			{ 'X', { "#..#", "#..#", ".##.", ".##.", "#..#", "#..#" } },
		};
		return table;
	}
}

namespace PixelText
{
	int Height(bool small)
	{
		return small ? kSmallH : kBigH;
	}

	int Width(size_t charCount, bool small)
	{
		if (charCount == 0)
		{
			return 0;
		}
		const int gw = small ? kSmallW : kBigW;
		return static_cast<int>(charCount) * gw + (static_cast<int>(charCount) - 1) * kGap;
	}

	std::string Make(const std::string& text, char ink, bool small)
	{
		const int h = Height(small);
		const int w = small ? kSmallW : kBigW;

		std::array<std::string, kBigH> rows;	// kBigH >= kSmallH - 앞 h 개만 쓴다.

		for (size_t i = 0; i < text.size(); ++i)
		{
			const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));

			for (int r = 0; r < h; ++r)
			{
				const char* line = nullptr;
				if (small)
				{
					auto it = SmallFont().find(upper);
					line = ((it != SmallFont().end()) ? it->second : SmallFont().at(' '))[r];
				}
				else
				{
					auto it = BigFont().find(upper);
					line = ((it != BigFont().end()) ? it->second : BigFont().at(' '))[r];
				}

				for (int c = 0; c < w; ++c)
				{
					rows[r] += (line[c] == '#') ? ink : '.';
				}
				if (i + 1 < text.size())
				{
					rows[r].append(kGap, '.');
				}
			}
		}

		std::string out;
		for (int r = 0; r < h; ++r)
		{
			out += rows[r];
			if (r + 1 < h)
			{
				out += '\n';
			}
		}
		return out;
	}

	std::string SolidBlock(int w, int h, char ink)
	{
		std::string row(static_cast<size_t>(w > 0 ? w : 0), ink);
		std::string out;
		for (int r = 0; r < h; ++r)
		{
			out += row;
			if (r + 1 < h)
			{
				out += '\n';
			}
		}
		return out;
	}
}
