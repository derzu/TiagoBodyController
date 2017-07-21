#include <Tiago.h>
#include <stdio.h>
#include <stdlib.h>

#include <Skeleton.h>

#define ROSRUN 1

Tiago::Tiago() {
	moving = false;
	//t = NULL;
	
	bzero(walkAngleQ, sizeof(int)*QUEUE_SIZE);
	walkAngleH = 0;
	
	walkDirection = NONE;
	walkAngle     = NONE;
	lastWalkDirection = -1;
	lastWalkAngle = -1;
	
	tronco=NONE;
	lastTronco=-1;
	
        if (ROSRUN) {
		// Init the joint controller, false means that it has a hand and not a gripper.
		jointController = new TiagoJointController(false);
		baseController  = new TiagoBaseController();
		
		initialPosition();
        }
        else {
	        jointController = NULL;
	        baseController = NULL;
        }
}


Tiago::~Tiago() {
	if (jointController)
		delete jointController;
	if (baseController)
		delete baseController;
}

void Tiago::initialPosition() {
	int r;

	// Send all joints controllers to their intial zero position.
	jointController->execute(true);
}

float Tiago::getAngElbow() {
	return angElbow;
}

float Tiago::getAngShoulder() {
	return angShoulder;
}

void Tiago::setMoving(bool m) {
	moving = m;
}

bool Tiago::isMoving() {
	return moving;
}

void Tiago::setAngElbow(float ang) {
	angShoulder = ang;
}

void Tiago::setAngShoulder(float ang) {
	angElbow = ang;
}


void Tiago::detectTiagoCommands(SkeletonPoints* sp, int afa, Point3D * closest) {
	static int c=0;
	c++;
	//printf("sp->head.z=%6d  left/rightHand=%6d::%6d\n", sp->head.z, sp->leftHand.z, sp->rightHand.z);

	// mao esquerda esticada e afastada do corpo, comandos ativados.
	if (sp->leftHand.x!=0 && sp->leftHand.x < sp->center.x - afa*2 && sp->leftHand.y > sp->center.y + afa)
	{
		// TORSO
		// media dos dois ombros atual
		int y1 = (sp->rightShoulder.y + sp->leftShoulder.y)/2; 
		// ultima media dos dois ombros armazenada
		int y2 = (sp->pointsV[SkeletonPoints::RIGHT_SHOULDER][sp->vHead[SkeletonPoints::RIGHT_SHOULDER] % BUF_SIZE].y + 
			  sp->pointsV[SkeletonPoints::LEFT_SHOULDER][sp->vHead[SkeletonPoints::LEFT_SHOULDER] % BUF_SIZE].y)/2;
		//printf("%d::Recebendo comandos (%d - %d)=%d\n", c++, y1, y2, y1 - y2);
		tronco = NONE;
		if (y1 - y2 >= 18) {
			tronco = DOWN;
		}
		if (y1 - y2 <= -18) {
			tronco = UP;
		}
		
		if (tronco!=lastTronco && tronco!=NONE) {
			printf("%d::TRONCO::%s\n", c, tronco==UP? "UP" : "DOWN");
			if (ROSRUN) {
				if (tronco==UP)
					jointController->setGoal("torso_lift_joint", 1); //systemThread("rosrun play_motion move_joint torso_lift_joint  1 0.2");
				else
					jointController->setGoal("torso_lift_joint", -1); //systemThread("rosrun play_motion move_joint torso_lift_joint -1 0.2");
				jointController->execute();
			}
			
		}
		lastTronco = tronco;



		// BRACO / ARM
		// so entra a cada 10c para nao poluir muito o terminal	
		//if (c%10==0)
		{
			float angShoulder, angElbow;
			// Angulo entre ombro e cotovelo
			if (sp->rightHand.x!=0 && sp->rightElbow.x!=0) {
				angShoulder = -atan2f(sp->rightElbow.y-sp->rightShoulder.y, sp->rightElbow.x-sp->rightShoulder.x)*180./CV_PI;
				angShoulder = (((int)angShoulder)/5)*5;
				setAngShoulder(angShoulder);
				//printf("ANG::COTOVELO::OMBRO::%.1f\n", angShoulder);
			}

			// Angulo entre antebraco e cotovelo
			if (sp->rightHand.x!=0 && sp->rightElbow.x!=0) {
				angElbow = -atan2f(sp->rightHand.y-sp->rightElbow.y, sp->rightHand.x-sp->rightElbow.x)*180./CV_PI;
				angElbow = (((int)angElbow)/5)*5;
				setAngElbow(angElbow);
				//printf("ANG::COTOVELO:: MAO ::%.1f\n\n", angElbow);
			}

			if (ROSRUN)
				moveArm(this);
		}

	}

	// mao esquerda afastada do corpo, e da linha centra para baixo.
	if (sp->leftHand.x!=0 && sp->leftHand.x < (sp->center.x - afa*1.5) && sp->leftHand.y > sp->center.y - afa/2)
	// se a mao esquerda estiver mais a esquerda do que o ombro, e ambos estiverem acima da linha da cintura
	//if (sp->leftHand.x!=0 && sp->leftElbow.x!=0 && sp->leftHand.y < sp->center.y-afa && sp->leftElbow.y < sp->center.y )
	{
		int profRight = sp->rightShoulder.z;
		int profLeft  = sp->leftShoulder.z;
		int diff =  profRight - profLeft;
		walkAngle = -1;
		if (profRight>0 && profLeft>0) {
			if (diff > 135)
				walkAngle = RIGHT;
			else if (diff < -135)
				walkAngle = LEFT;
			else
				walkAngle = NONE;
			
			if (walkAngle != -1)
				walkAngleQ[walkAngleH++ % QUEUE_SIZE] = walkAngle;
			walkAngle = getModeVector(walkAngleQ);
		}
		if (walkAngle != -1) {
			//printf("profundidade: %4d %4d %5d %8s %8s\n", profRight, profLeft, diff, diff > 150 ? "DIREITA" : diff<-150 ? "ESQUERDA" : "NONE",  walkAngle==RIGHT ? "DIREITA" : walkAngle==LEFT ? "ESQUERDA" : "NONE");
		}
		
			
		printf("diff1=%5d diff2=%5d\n", abs(sp->leftHand.y - sp->leftElbow.y), abs(sp->leftHand.x - sp->leftElbow.x));
		walkDirection = NONE;
		// se a mao e o ombro estiverem quase na mesma linha, e os x distantes.
		if (abs(sp->leftHand.y - sp->leftElbow.y)<30  &&  abs(sp->leftHand.x - sp->leftElbow.x)>50) {
			walkDirection = BACKWARD;
		}
		// se a mao estiver afastada no y, mas nao muito AND mao proximo da linha da cintura.
		else if (sp->leftHand.x > (sp->center.x - afa*4) && (sp->leftHand.y < (sp->center.y + afa*1.5)) ) {
			printf("diff3=%5d\n", (int)Skeleton::euclideanDist(*closest, sp->leftHand));
			// se a mao tiver perto do ponto mais proximo
			if (Skeleton::euclideanDist(*closest, sp->leftHand) < 40)
				walkDirection = FORWARD;
		}
		printf("walkDirection=%2d, walkAngle=%2d\n", walkDirection, walkAngle);

		
		if(ROSRUN)
			moveBase(walkDirection, walkAngle);
	}
}



void * Tiago::moveArm(void * t) {
	Tiago * tiago = (Tiago*)t;

	if (tiago->isMoving()) return NULL;

	tiago->setMoving(true);

	char comando[100];
	char jointStr[20];
	int joint;
	float ang;
	int r;

	/*if (aShoulder>0) {
		if (aElbow>0)
			ang = aElbow-aShoulder; // subtrai o shoulder do ombro
		else
			ang = aElbow-aShoulder; // soma os dois angulos
	}
	else {
		if (aElbow>0)
			ang = aElbow-aShoulder; // soma os dois angulos
		else
			ang = aElbow-aShoulder; // subtrai o shoulder do ombro
	}*/

	joint = 4;		
	ang = tiago->getAngElbow() - tiago->getAngShoulder();
	ang = ang*ELBOW_90/90.; // conversao do angulo para os valores compativeis com o Tiago.
	sprintf(jointStr, "arm_%d_joint", joint);
	tiago->jointController->setGoal(jointStr, ang);

	joint = 2;
	ang = tiago->getAngShoulder();
	ang = ang*SHOULDER_45/45.; // conversao do angulo para os valores compativeis com o Tiago.
	sprintf(jointStr, "arm_%d_joint", joint);
	tiago->jointController->setGoal(jointStr, ang);
	
	// The 2 goals will be executed simultaneously
	tiago->jointController->execute();

	tiago->setMoving(false);
	//tiago->mutexUnlock();
}



void Tiago::moveBase(int walkDirection, int walkAngle) {
	int r;
	float ang = 0;
	float dir = 0;

	if (walkDirection==FORWARD)
		dir = TiagoBaseController::FORWARD;
	if (walkDirection==BACKWARD)
		dir = TiagoBaseController::BACKWARD;
	if (walkAngle==RIGHT)
		ang = TiagoBaseController::RIGHT;
	if (walkAngle==LEFT)
		ang = TiagoBaseController::LEFT;
	
	if (dir!=0 || ang!=0) {
		baseController->executeGoal(dir, ang);
	}
	
	lastWalkDirection = walkDirection;
	lastWalkAngle = walkAngle;
}


/**
 * Mode algorithm just for a 3 valued vector.
 **/
int Tiago::getModeVector(int vector[]) {
	int histo[3]={0,0,0}; // NONE, RIGHT, LEFT

	for (int i=0 ; i<QUEUE_SIZE ; i++)
		histo[vector[i]]++;
		
	if (histo[0]>histo[1] && histo[0]>histo[2])
		return 0;
	else if (histo[1]>histo[0] && histo[1]>histo[2])
		return 1;
	else
		return 2;
}

/**
 * Mediana
 **/
int Tiago::getMedianaVector(int vector[]) {
	int m = 1;
	int q=0;

	SkeletonPoints::quick_sort(vector, 0, QUEUE_SIZE);

	for (int i=0 ; i<QUEUE_SIZE ; i++) {
		if (vector[i]!=0) {
			q++;
		}
	}
	if (q>0)
		return vector[q/2+QUEUE_SIZE-q];

	return m;
}



