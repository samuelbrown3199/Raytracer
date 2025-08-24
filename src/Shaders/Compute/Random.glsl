#ifndef Random
#define Random

float RandomSeed(out uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return float(seed % 10000) / 10000.0;
}

// Generates a random integer in [min, max]
int RandomIntInRange(int min, int max, out uint seed)
{
    return min + int(RandomSeed(seed) * float(max - min + 1));
}

// Generates a random float in [min, max]
float RandomFloatInRange(float min, float max, out uint seed)
{
    return mix(min, max, RandomSeed(seed));
}

uint SeedFromCoords(uint x, uint y)
{
    // Use a split for the large constant into two 32-bit values
    uint high = 1442695040u;  // Upper part of the number
    uint low = 88963407u;     // Lower part of the number

    uint seed = x * 374761393u + y * 668265263u * high + low;

    seed = (seed ^ (seed >> 16)) * 0x85ebca6b;
    seed = (seed ^ (seed >> 13)) * 0xc2b2ae35;
    seed = seed ^ (seed >> 16);
    return seed;
}

vec3 SampleSquare(out uint seed) 
{
    float x = RandomFloatInRange(0.0, 1.0, seed);
    float y = RandomFloatInRange(0.0, 1.0, seed);
    return vec3(x - 0.5, y - 0.5, 0.0);
}

vec3 RandomInUnitDisk(out uint seed)
{
	while (true)
	{
		vec3 p = vec3(RandomFloatInRange(-1.0, 1.0, seed), RandomFloatInRange(-1.0, 1.0, seed), 0.0);
		if (sqrt(length(p)) < 1)
			return p;
	}
}

#endif