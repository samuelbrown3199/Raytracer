#ifndef BVH_H
#define BVH_H

#include <memory>
#include <vector>

#include <algorithm>

#include "AABB.h"
#include "../Hittable.h"

class BVHNode : public Hittable
{
public:

	BVHNode(HittableList list) : BVHNode(list.objects, 0, list.objects.size()) {}
	BVHNode(std::vector<std::shared_ptr<Hittable>>& objects, size_t start, size_t end)
	{
		bbox = AABB::Empty;
		for (size_t objectIndex = start; objectIndex < end; objectIndex++)
			bbox = AABB(bbox, objects[objectIndex]->BoundingBox());

		int axis = bbox.LongestAxis();
		auto comparator = (axis == 0) ? BoxXCompare
			: (axis == 1) ? BoxYCompare
			: BoxZCompare;

		size_t objectSpan = end - start;

		if (objectSpan == 1)
		{
			left = right = objects[start];
		}
		else if (objectSpan == 2)
		{
			left = objects[start];
			right = objects[start + 1];
		}
		else
		{
			std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
			auto mid = start + objectSpan / 2;
			left = std::make_shared<BVHNode>(objects, start, mid);
			right = std::make_shared<BVHNode>(objects, mid, end);
		}
	}

	bool Hit(const Ray& r, Interval t, HitRecord& rec) const override
	{
		if(!bbox.Hit(r, t))
			return false;

		bool hitLeft = left->Hit(r, t, rec);
		bool hitRight = right->Hit(r, Interval(t.min, hitLeft ? rec.t : t.max), rec);

		return hitLeft || hitRight;
	}

	AABB BoundingBox() const override
	{
		return bbox;
	}

private:

	std::shared_ptr<Hittable> left;
	std::shared_ptr<Hittable> right;

	AABB bbox;

	static bool BoxCompare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b, int axis)
	{
		auto aAxisInterval = a->BoundingBox().AxisInterval(axis);
		auto bAxisInterval = b->BoundingBox().AxisInterval(axis);

		return aAxisInterval.min < bAxisInterval.min;
	}

	static bool BoxXCompare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b)
	{
		return BoxCompare(a, b, 0);
	}

	static bool BoxYCompare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b)
	{
		return BoxCompare(a, b, 1);
	}

	static bool BoxZCompare(const std::shared_ptr<Hittable> a, const std::shared_ptr<Hittable> b)
	{
		return BoxCompare(a, b, 2);
	}
};

#endif