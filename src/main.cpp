#include "particle.h"

#include <iostream>

int main(int argc, char* argv[])
{
	Particle p;
	p.position = { 1, 2, 3 };

	std::cout << p.position.x << "," << p.position.y << "," << p.position.z << std::endl;
	return 0;
}