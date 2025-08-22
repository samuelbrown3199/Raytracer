#ifndef AABB_H
#define AABB_H

class AABB
{
public:
	Interval x, y, z;

	AABB() {}

	AABB(const Interval& x, const Interval& y, const Interval& z)
		: x(x), y(y), z(z) {
	}

	AABB(const Vector3& a, const Vector3& b)
	{
		x = (a[0] <= b[0]) ? Interval(a[0], b[0]) : Interval(b[0], a[0]);
		y = (a[1] <= b[1]) ? Interval(a[1], b[1]) : Interval(b[1], a[1]);
		z = (a[2] <= b[2]) ? Interval(a[2], b[2]) : Interval(b[2], a[2]);
	}

	AABB(const AABB& box0, const AABB& box1) 
	{
		x = Interval(box0.x, box1.x);
		y = Interval(box0.y, box1.y);
		z = Interval(box0.z, box1.z);
	}

	const Interval& AxisInterval(int n) const
	{
		if (n == 1) return y;
		if (n == 2) return z;
		return x;
	}

	int LongestAxis() const 
	{
		if (x.Size() > y.Size())
			return x.Size() > z.Size() ? 0 : 2;
		else
			return y.Size() > z.Size() ? 1 : 2;
	}

	bool Hit(const Ray& r, Interval rayT) const
	{
		const Vector3& rayOrigin = r.Origin();
		const Vector3& rayDirection = r.Direction();

		for (int axis = 0; axis < 3; axis++)
		{
			const Interval& ax = AxisInterval(axis);
			const double adInv = 1.0 / rayDirection[axis];

			double t0 = (ax.min - rayOrigin[axis]) * adInv;
			double t1 = (ax.max - rayOrigin[axis]) * adInv;

			if(t0 < t1)
			{
				if (t0 > rayT.min) rayT.min = t0;
				if (t1 < rayT.max) rayT.max = t1;
			}
			else
			{
				if (t1 > rayT.min) rayT.min = t1;
				if (t0 < rayT.max) rayT.max = t0;
			}

			if (rayT.max <= rayT.min) return false;
		}

		return true;
	}

	static const AABB Empty, Universe;
};

#endif