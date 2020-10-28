﻿#pragma once
#include <float3.hpp>

namespace guarneri {
	struct sphere {
	public:
		sphere() {
			center = guarneri::float3();
			radius = 0;
		}

		sphere(const guarneri::float3& center, const float& radius) {
			this->center = center;
			this->radius = radius;
		}

		sphere(const sphere& other) {
			this->center = other.center;
			this->radius = other.radius;
		}

	public:
		guarneri::float3 center;
		float radius;	
	
	public:
			std::string str() const {
				std::stringstream ss;
				ss << "sphere: [origin: " << this->center << ", radius: " << std::setprecision(FLOAT_LOG_PRECISION) << this->radius << "]";
				return ss.str();
			}
	};

	static std::ostream& operator << (std::ostream& stream, const sphere& sphere) {
		stream << sphere.str();
		return stream;
	}

	static std::stringstream& operator << (std::stringstream& stream, const sphere& sphere) {
		stream << sphere.str();
		return stream;
	}
}