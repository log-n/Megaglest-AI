#include "Learning_AI.h"
#include "ai_interface.h"
#include "ai.h"
#include "unit_type.h"
#include "unit.h"
#include "leak_dumper.h"

namespace Glest { namespace Game {

Snapshot::Snapshot(AiInterface * aiInterface , FILE * logs)
{
	this->logs = logs;
    this->aiInterface = aiInterface;
    beingAttacked = 0 ;
    readyForAttack = 0;
    damagedUnitCount = 0;
    noOfWorkers = 0;
    noOfWarriors = 0;
    noOfBuildings = 0;
    for( int i = 0; i < NUM_OF_RESOURCES; i++ ) {
	resourcesAmount[ i ] = 0;
    }
    /*
    resourcesType[ 0 ] = "gold";
    resourcesType[ 1 ] = "wood";
    resourcesType[ 2 ] = "stone";
    resourcesType[ 3 ] = "food";
    resourcesType[ 4 ] = "energy";
    resourcesType[ 5 ] = "housing";*/
    upgradeCount = 0;
    noOfKills = 0;

	Field field ;
	Vec2i  pos;
	getUpgradeCount();
	CheckIfBeingAttacked(pos, field);
	CheckIfStableBase();
	countDamagedUnits( );
	getKills();
	setNumOfDeaths();
	noOfWorkers = getCountOfClass(ucWorker);
	noOfWarriors = getCountOfClass(ucWarrior);
	noOfBuildings = getCountOfClass(ucBuilding);
	getResourceStatus();

	fprintf(logs, "out of Snapshot constructor\n");
	fflush(logs);
}

Snapshot &Snapshot::operator =( const Snapshot &s ) {
    aiInterface = s.aiInterface ;
    beingAttacked = s.beingAttacked ;
    readyForAttack = s.readyForAttack;
    damagedUnitCount = s.damagedUnitCount;
    noOfWorkers = s.noOfWorkers;
    noOfWarriors = s.noOfWarriors;
    noOfBuildings = s.noOfBuildings;
    for( int i = 0; i < NUM_OF_RESOURCES; i++ ) {
	resourcesAmount[ i ] = s.resourcesAmount[ i ] ;
    }
    upgradeCount = s.upgradeCount;
    noOfKills = s.noOfKills;
    noOfDeaths = s.noOfDeaths;

    return *this;
}

void Snapshot::getKills()
{
   /* int count = 0 ;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i)
	{
		const Unit *u= aiInterface->getMyUnit(i);
		count += u->getKills();
	}
	noOfKills  = count;
	fprintf(logs," No of kills : %d \n", noOfKills);
	fflush(logs);
	return count ;*/
	noOfKills = aiInterface->getKills();
}

void Snapshot :: setNumOfDeaths()
{
	noOfDeaths = aiInterface->getDeaths();
}

bool Snapshot::CheckIfBeingAttacked(Vec2i &pos, Field &field)
{
	fprintf(logs, "In check if being attacked....\n");
	if (aiInterface == NULL)
			fprintf(logs, " aiInterface is NULL\n");
	fflush(logs);
    int count= aiInterface->onSightUnitCount();
    const Unit *unit;
	int radius = baseRadius;
    for(int i=0; i<count; ++i){
        unit= aiInterface->getOnSightUnit(i);
        if(!aiInterface->isAlly(unit) && unit->isAlive()){
            pos= unit->getPos();
			field= unit->getCurrField();
            if(pos.dist(aiInterface->getHomeLocation())<radius)
	    {
                aiInterface->printLog(4, "Being attacked at pos "+intToStr(pos.x)+","+intToStr(pos.y)+"\n");
				fprintf(logs," Being attacked at pos  %d   %d \n"  , pos.x ,pos.y);
				fflush(logs);
				beingAttacked = 1;
                return true;
            }
        }
    }
	aiInterface->printLog(4, "Not  Being attacked  \n");
	fprintf(logs," Not  Being attacked  \n");
	fflush(logs);
    beingAttacked = 0;
    return false;

}


bool Snapshot::CheckIfStableBase(){

    if(getCountOfClass(ucWarrior)>minWarriors)
    {
        aiInterface->printLog(4, "Base is stable\n");
		fprintf(logs,"Base is stable\n");
		fflush(logs);
	readyForAttack =  1;
        return true;
    }
    else{
        aiInterface->printLog(4, "Base is not stable\n");
		fprintf(logs,"Base is not stable\n");
		fflush(logs);

	readyForAttack = 0;
        return false;
    }
}

int Snapshot::countDamagedUnits( )
{
    int count = 0 ;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		const Unit *u= aiInterface->getMyUnit(i);
		if(u->getHpRatio()<1.f)
		{
		    count++;
		}
	}
	damagedUnitCount  = count;
	aiInterface->printLog(4, "Damaged Unit count :  " + intToStr(damagedUnitCount)+"\n");
	fprintf(logs,"Damaged Unit count :  %d \n" , damagedUnitCount);
	fflush(logs);
	return count ;
}

int Snapshot::getCountOfType(const UnitType *ut){
    int count= 0;
	for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(ut == aiInterface->getMyUnit(i)->getType()){
			count++;
		}
	}

    return count;
}

int Snapshot::getCountOfClass(UnitClass uc){
    int count= 0;
    for(int i=0; i<aiInterface->getMyUnitCount(); ++i){
		if(aiInterface->getMyUnit(i)->getType()->isOfClass(uc)){
            ++count;
		}
    }
	aiInterface->printLog(4, "Unit count  for unit " +  intToStr(uc ) + "  :  " + intToStr(count)+"\n");
	fprintf(logs, "Unit count  for unit %d is %d \n", uc , count);
	fflush(logs);
    return count;
}

float Snapshot::getRatioOfClass(UnitClass uc){
	if(aiInterface->getMyUnitCount()==0){
		return 0;
	}
	else{
		return static_cast<float>(getCountOfClass(uc))/aiInterface->getMyUnitCount();
	}
}
int Snapshot:: getUpgradeCount()
{
    upgradeCount = aiInterface->getMyUpgradeCount();
	aiInterface->printLog(4, " Upgrade count    " +  intToStr(upgradeCount  ) +"\n");
	fprintf(logs, "Upgrade count  : %d \n",upgradeCount);
	fflush(logs);
    return upgradeCount;
}

void Snapshot:: getResourceStatus()
{
	for(int i = 0 ; i < NUM_OF_RESOURCES; i++)
	{
		resourcesAmount[i] = 0;
	}
    const TechTree *tt= aiInterface->getTechTree();
	for(int unitIndex = 0 ; unitIndex < aiInterface->getMyUnitCount() ; unitIndex++ )
	{
		const Unit *unit = aiInterface->getMyUnit(unitIndex);

		for(int i = 0; i < tt->getResourceTypeCount(); ++i)
		{
			const ResourceType *rt= tt->getResourceType(i);
			const Resource *r= aiInterface->getResource(rt);
			int amt = r->getAmount();
			string resource_name = rt->getName ();
			if(resource_name == "gold" )
			{
					resourcesAmount[gold] += amt ;
			}
			else if(resource_name == "wood"  )
			{
					resourcesAmount[wood] += amt ;
			}
			else if(resource_name == "stone"  )
			{
					resourcesAmount[stone] += amt ;
			}
			else if(resource_name == "food"  )
			{
					resourcesAmount[food] += amt ;
			}
			else if(resource_name == "energy"  )
			{
					resourcesAmount[energy] += amt ;
			}
			else if(resource_name == "housing"  )
			{
					resourcesAmount[housing] += amt ;
			}
		}
	}
}

}}
