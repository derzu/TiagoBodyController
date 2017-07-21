#include <GestualListener.h>

GestualListener::GestualListener() {
	tiago = new Tiago();
}

GestualListener::~GestualListener() {
	if (tiago)
		delete tiago;
}

void GestualListener::onEvent(SkeletonPoints * sp, int afa, Point3D *closest) {
	//printf("Recebi o esqueleto\n");
	
	tiago->detectTiagoCommands(sp, afa, closest);
}
