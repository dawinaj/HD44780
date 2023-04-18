#pragma once

struct lcd_dims_t
{
	uint8_t y;
	uint8_t x;

	lcd_dims_t &operator+=(const lcd_dims_t &r)
	{
		y += r.y;
		x += r.x;
		return *this;
	}
	lcd_dims_t &operator-=(const lcd_dims_t &r)
	{
		y -= r.y;
		x -= r.x;
		return *this;
	}
	lcd_dims_t &operator*=(const lcd_dims_t &r)
	{
		y *= r.y;
		x *= r.x;
		return *this;
	}
	lcd_dims_t &operator/=(const lcd_dims_t &r)
	{
		y /= r.y;
		x /= r.x;
		return *this;
	}
	lcd_dims_t &operator%=(const lcd_dims_t &r)
	{
		y %= r.y;
		x %= r.x;
		return *this;
	}
};

lcd_dims_t operator+(lcd_dims_t l, const lcd_dims_t &r)
{
	l.y += r.y;
	l.x += r.x;
	return l;
}
lcd_dims_t operator-(lcd_dims_t l, const lcd_dims_t &r)
{
	l.y -= r.y;
	l.x -= r.x;
	return l;
}
lcd_dims_t operator*(lcd_dims_t l, const lcd_dims_t &r)
{
	l.y *= r.y;
	l.x *= r.x;
	return l;
}
lcd_dims_t operator/(lcd_dims_t l, const lcd_dims_t &r)
{
	l.y /= r.y;
	l.x /= r.x;
	return l;
}
lcd_dims_t operator%(lcd_dims_t l, const lcd_dims_t &r)
{
	l.y %= r.y;
	l.x %= r.x;
	return l;
}
