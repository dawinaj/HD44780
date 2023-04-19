#pragma once

struct lcd_symbol_t
{
	uint8_t dots[10];

	uint8_t &operator[](size_t pos)
	{
		return dots[pos];
	}
	const uint8_t &operator[](size_t pos) const
	{
		return dots[pos];
	}
	void set(uint8_t y, uint8_t x)
	{
		dots[y] |= (1 << (5 - x));
	}
	void unset(uint8_t y, uint8_t x)
	{
		dots[y] &= ~(1 << (5 - x));
	}
};
