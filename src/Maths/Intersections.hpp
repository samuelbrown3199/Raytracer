#pragma once

#include "Ray.hpp"

// Returns the nearest intersection point (t value) of a ray with a sphere.
// If there is no intersection, returns -1.0.
//
// Parameters:
// - center: The center position of the sphere.
// - radius: The radius of the sphere.
// - ray:    The ray to test for intersection.
//
// The function solves the quadratic equation for the intersection of a ray and a sphere.
// The discriminant determines if the ray hits the sphere (discriminant >= 0).
// If it does, the function returns the smallest positive t value (nearest intersection).
// Otherwise, it returns -1.0 to indicate no intersection.
double HitSphere(const Vector3& center, const double& radius, const Ray& ray)
{
    // Vector from the ray's origin to the sphere's center
    Vector3 oc = center - ray.Origin();

	auto a = ray.Direction().SqrLength();
	auto h = Dot(ray.Direction(), oc);
	auto c = oc.SqrLength() - radius * radius;
	auto discriminant = h * h - a * c;

    if (discriminant < 0)
    {
		// No intersection: return -1.0
        return -1.0;
    }
    else
    {
        // Ray intersects the sphere: return the nearest intersection point (smallest t)
		return (h - std::sqrt(discriminant)) / a;
    }
}