#include <GestualListener.h>

GestualListener::GestualListener() {
	tiago = new Tiago();
}

GestualListener::~GestualListener() {
	if (tiago)
		delete tiago;
}

void GestualListener::onEvent(SkeletonPoints * sp, int afa) {
	printf("Recebi o esqueleto\n");
	
	//tiago->detectTiagoCommands(sp, afa*subSample, depth, closest);
	tiago->detectTiagoCommands(sp, afa);
}
