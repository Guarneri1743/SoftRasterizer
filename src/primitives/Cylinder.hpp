﻿#pragma once
#include <Guarneri.hpp>

namespace Guarneri {
	struct Cylinder {
	public:
		Cylinder() {
			axis = Line();
			radius = 0;
			height = 0;
		}

		Cylinder(const Line& axis_t, const float& _radius, const float& _height) {
			this->axis = axis_t;
			this->radius = _radius;
			this->height = _height;
		}

		Cylinder(const Cylinder& other) {
			this->axis = other.axis;
			this->radius = other.radius;
			this->height = other.height;
		}

	public:
		Line axis;
		float radius;
		float height;

	public:
		std::string str() const {
			std::stringstream ss;
			ss << "Cylinder: [axis: " << this->axis << ", radius: " << std::setprecision(FLOAT_LOG_PRECISION) << this->radius << ", height" << std::setprecision(FLOAT_LOG_PRECISION) << this->height << "]";
			return ss.str();
		}
	};

	static std::ostream& operator << (std::ostream& stream, const Cylinder& Cylinder) {
		stream << Cylinder.str();
		return stream;
	}

	static std::stringstream& operator << (std::stringstream& stream, const Cylinder& Cylinder) {
		stream << Cylinder.str();
		return stream;
	}
}