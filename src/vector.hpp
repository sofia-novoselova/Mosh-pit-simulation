#pragma once
#include <cstdint>
#include <iostream>
struct Vector {
  float x;
  float y;
  Vector();
  Vector(float, float);

  float get_magnitude() const;
  float get_squared_magnitude() const;

  Vector operator*(float) const;
  Vector operator/(float) const;
  Vector operator+(const Vector&) const;
  Vector operator-(const Vector&) const;
  Vector operator-() const;

  Vector& operator*=(float);
  Vector& operator+=(const Vector&);

  friend Vector operator*(float val, const Vector&);
};