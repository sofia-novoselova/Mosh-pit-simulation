#include "vector.hpp"
#include <cinttypes>
#include <cmath>

Vector::Vector() : x(0), y(0) {
}

Vector::Vector(float x_i, float y_i) : x(x_i), y(y_i) {
}

float Vector::get_magnitude() const {
  return std::sqrt(x * x + y * y);
}

float Vector::get_squared_magnitude() const {
  return x * x + y * y;
}

Vector Vector::operator*(float scal) const {
  return {x * scal, y * scal};
}

Vector Vector::operator/(float scal) const {
  return *this * (1 / scal);
}

Vector Vector::operator+(const Vector& other) const {
  return {x + other.x, y + other.y};
}

Vector Vector::operator-(const Vector& other) const {
  return {x - other.x, y - other.y};
}

Vector Vector::operator-() const {
  return {-x, -y};
}

Vector& Vector::operator*=(float scal) {
  x *= scal;
  y *= scal;
  return *this;
}

Vector& Vector::operator+=(const Vector& other) {
  x += other.x;
  y += other.y;
  return *this;
}

Vector operator*(float val, const Vector& other) {
  return {other.x * val, other.y * val};
}