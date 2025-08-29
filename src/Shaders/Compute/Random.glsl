#ifndef Random
#define Random

float RandomSeed(inout uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return float(seed % 10000) / 10000.0;
}

float LCG(inout uint state)
{
    state = 1664525u * state + 1013904223u;
    return float(state & 0x00FFFFFFu) / float(0x01000000u);
}

// Generates a random integer in [min, max]
int RandomIntInRange(int min, int max, inout uint seed)
{
    seed += 1u;
    return min + int(RandomSeed(seed) * float(max - min + 1));
}

// Generates a random float in [min, max]
float RandomFloatInRange(float min, float max, inout uint seed)
{
    seed += 1u;
    return mix(min, max, RandomSeed(seed));
}

uint SeedFromCoords(uint x, uint y, uint frame, uint i)
{
    uint seed = x * 374761393u + y * 668265263u + frame * 1442695040u + i * 88963407u;
    seed = (seed ^ (seed >> 16)) * 0x85ebca6b;
    seed = (seed ^ (seed >> 13)) * 0xc2b2ae35;
    seed = seed ^ (seed >> 16);
    return seed;
}

vec3 SampleSquare(inout uint seed) 
{
    seed += 1u;
    float x = RandomFloatInRange(0.0, 1.0, seed);
    float y = RandomFloatInRange(0.0, 1.0, seed);
    return vec3(x - 0.5, y - 0.5, 0.0);
}

vec3 RandomInUnitDisk(inout uint seed)
{
    seed += 1u;
	while (true)
	{
		vec3 p = vec3(RandomFloatInRange(-1.0, 1.0, seed), RandomFloatInRange(-1.0, 1.0, seed), 0.0);
		if (sqrt(length(p)) < 1)
			return p;
	}
}

vec3 RandomVector(inout uint seed)
{
    seed += 1u;
    return vec3(RandomFloatInRange(-1.0, 1.0, seed), RandomFloatInRange(-1.0, 1.0, seed), RandomFloatInRange(-1.0, 1.0, seed));
}

vec3 RandomUnitVector(inout uint seed)
{
    seed += 1u;
	while (true)
	{
		vec3 p = RandomVector(seed);
		float lensq = length(p) * length(p);
		if (1e-160 < lensq && lensq <= 1.0)
			return p / sqrt(lensq);
	}
}

vec3 RandomOnHemisphere(vec3 normal, inout uint seed)
{
    seed += 1u;
	vec3 onUnitSphere = RandomUnitVector(seed);
	if (dot(onUnitSphere, normal) > 0.0)
		return onUnitSphere;
	else
		return -onUnitSphere;
}

#endif