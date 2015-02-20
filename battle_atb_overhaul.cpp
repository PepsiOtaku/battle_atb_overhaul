// ATB Overhaul
// by PepsiOtaku
// Version 1.0

#include <DynRPG/DynRPG.h>
#define NOT_MAIN_MODULE
#include <vector>
#include <sstream>

std::map<std::string, std::string> configuration;

int confAddValue;
int confHeroSpeedVarM;
int confBattlerStartVar;
int confMonsterSpeedVarM;
int confDefaultHeroSpeed;
int confDefaultMonsterSpeed;
int confAtbModeSwitch;
int confActiveSpeed;
int confWaitSpeed;
bool atbWait;
bool monsterAction = false;
bool confAgilMult;
bool condCheckFail;
int confFreezeSwitch;
std::string condExceptionSet;

std::vector<int> exceptionVect;



unsigned char frameTimer = 0;

bool onStartup (char *pluginName) {
	configuration = RPG::loadConfiguration(pluginName);
	confAtbModeSwitch = atoi(configuration["ATBmodeSwitch"].c_str());
	confAddValue = atoi(configuration["AddValue"].c_str());
	confAgilMult = configuration["MultiplyAgility"] == "true";
    confHeroSpeedVarM = atoi(configuration["HeroSpeedMasterVar"].c_str()); // Convert String to Int
    confMonsterSpeedVarM = atoi(configuration["MonsterSpeedMasterVar"].c_str());
    confBattlerStartVar = atoi(configuration["BattlerStartVar"].c_str());
    confDefaultHeroSpeed = atoi(configuration["DefaultHeroSpeed"].c_str());
    confDefaultMonsterSpeed = atoi(configuration["DefaultMonsterSpeed"].c_str());
    confActiveSpeed = atoi(configuration["ActiveSpeed"].c_str());
    confWaitSpeed = atoi(configuration["WaitSpeed"].c_str());
    confFreezeSwitch = atoi(configuration["FreezeSwitch"].c_str());
    // get the condition exceptions
    condExceptionSet = configuration["ConditionExceptions"];
    std::string delimiter = ",";
    size_t pos = 0;
    int token;
    while ((pos = condExceptionSet.find(delimiter)) != std::string::npos) {
        token = atoi(condExceptionSet.substr(0, pos).c_str());
        exceptionVect.push_back(token);
        condExceptionSet.erase(0, pos + delimiter.length());
    }
    token = atoi(condExceptionSet.c_str());
    exceptionVect.push_back(token);

	return true;
}

void initializeSpeedVars(){
    if (RPG::variables[confHeroSpeedVarM] == 0) RPG::variables[confHeroSpeedVarM] = confDefaultHeroSpeed;
    if (RPG::variables[confMonsterSpeedVarM] == 0) RPG::variables[confMonsterSpeedVarM] = confDefaultMonsterSpeed;
}

void onNewGame(){
    // initialize the master speed variables based on what's in DynRPG.ini
    initializeSpeedVars();
}

void onLoadGame(int id, char *data, int length) {
    // initialize the master speed variables based on what's in DynRPG.ini
    initializeSpeedVars();
    // initialize the Active/Wait system based on what the ATB Mode Switch is in the save file
    if (RPG::switches[confAtbModeSwitch] == true) RPG::system->atbMode = RPG::ATBM_ACTIVE;
    else RPG::system->atbMode = RPG::ATBM_WAIT;
}

bool onSetSwitch(int id, bool value) {
    // if ATBmodeSwitch is on, set it to active, otherwise if off, set to wait
    if (id == confAtbModeSwitch){ // this was moved from ragnadyn since it was more appropriate here
        if (value == true) RPG::system->atbMode = RPG::ATBM_ACTIVE;
        else RPG::system->atbMode = RPG::ATBM_WAIT;
    }
    return true;
}

// this will reset RPG::BattleSpeed. NOTE: RPG::battleSpeed is only set to 100 (default), 0 (for Wait Mode), or 75 (for Active Mode)
// onFrame only makes alterations to individual battler's ATB bars
void resumeAtb(){
    if (atbWait == true) {
        RPG::battleSpeed = 100;
        atbWait = false;
    }
}

void onFrame (RPG::Scene scene){
    if (scene == RPG::SCENE_BATTLE
        && !atbWait && !monsterAction) // do not do the following if the ATB bar is set to 0 or a monster is performing an action
    {
        frameTimer++;
        if (frameTimer > 10)
        {
            frameTimer = 0;
            for (int i=0; i<8; i++)
            {
                for (unsigned int j=0; j<exceptionVect.size(); j++)
                {
                    if (RPG::Actor::partyMember(i) != NULL
                        && RPG::Actor::partyMember(i)->conditions[exceptionVect[j]] != 0)
                    {
                        RPG::Actor::partyMember(i)->atbValue = 0;
                        condCheckFail = true;
                        break;
                    } else condCheckFail = false;
                }
                // Heroes
                if (RPG::Actor::partyMember(i) != NULL && i<4 && condCheckFail == false
                    // The ATB bar shouldn't move in these situations...
                    && ((RPG::Actor::partyMember(i)->conditions[1] == 0 // ... the hero is not dead
                    && RPG::Actor::partyMember(i)->actionStatus == RPG::AS_IDLE // ... the hero is idle
                    && RPG::Actor::partyMember(i)->animationId != 14) // ... the hero has won the battle
                    // 14 is the animation id for battle won for whatever reason-- should be 11
                    && RPG::switches[confFreezeSwitch] == false // Other plugins need a means of freezing battle
                    ))
                {
                    if (RPG::Actor::partyMember(i)->atbValue < 300000)
                        if (confAgilMult == true)
                            RPG::Actor::partyMember(i)->atbValue = RPG::Actor::partyMember(i)->atbValue + ((confAddValue*RPG::Actor::partyMember(i)->getAgility())*RPG::variables[confBattlerStartVar+i]);
                        else RPG::Actor::partyMember(i)->atbValue = RPG::Actor::partyMember(i)->atbValue + (confAddValue*RPG::variables[confBattlerStartVar+i]);
                    else RPG::Actor::partyMember(i)->atbValue = 300000;
                    if (RPG::Actor::partyMember(i)->atbValue < 0)
                        RPG::Actor::partyMember(i)->atbValue = 0;
                }
                for (unsigned int j=0; j<exceptionVect.size(); j++)
                {
                    if (RPG::monsters[i] != NULL
                        && RPG::monsters[i]->conditions[exceptionVect[j]] != 0)
                    {
                        condCheckFail = true;
                        break;
                    } else condCheckFail = false;
                }
                // Monsters
                if (RPG::monsters[i] != NULL && condCheckFail == false
                    && ((RPG::monsters[i]->conditions[1] == 0 // the monster is dead
                    && RPG::monsters[i]->actionStatus == RPG::AS_IDLE)
                    && RPG::switches[confFreezeSwitch] == false // Other plugins need a means of freezing battle
                    ))
                {
                    if (RPG::monsters[i]->atbValue < 300000)
                        if (confAgilMult == true)
                            RPG::monsters[i]->atbValue = RPG::monsters[i]->atbValue + ((confAddValue* RPG::monsters[i]->getAgility())*RPG::variables[confBattlerStartVar+4+i]);
                        else RPG::monsters[i]->atbValue = RPG::monsters[i]->atbValue + (confAddValue*RPG::variables[confBattlerStartVar+4+i]);
                    else RPG::monsters[i]->atbValue = 300000;
                    if (RPG::monsters[i]->atbValue < 0)
                        RPG::monsters[i]->atbValue = 0;
                }
            }
        }
    } else if (scene != RPG::SCENE_BATTLE) {
        resumeAtb();
    }
}

// anytime the battle status window refreshes
bool onBattleStatusWindowDrawn(int x, int selection, bool selActive, bool isTargetSelection, bool isVisible) {
    if (selActive == true) {
        if (RPG::system->atbMode == RPG::ATBM_WAIT) RPG::battleSpeed = confWaitSpeed; // wait mode should stop the atb bar when heros are selecting actions
        else RPG::battleSpeed = confActiveSpeed; // active mode should still move the atb bar by the % set in confActiveSpeed (100 is the default speed)
        atbWait = true;
    }

    // if the party member dies (or has a condition with a status restriction) while the status window was open
    for (unsigned int j=0; j<exceptionVect.size(); j++)
    {
        if ((RPG::Actor::partyMember(selection)->conditions[1]
            && RPG::Actor::partyMember(selection)->conditions[exceptionVect[j]]) != 0) {
            RPG::Actor::partyMember(selection)->atbValue = 0;
            resumeAtb(); // resume the ATB bar
            //MessageBox(NULL, "what happen", "Debug", MB_ICONINFORMATION);
            break;
        }
    }
    return true;
}
// resume the ATB bar when the action is started
bool onDoBattlerAction(RPG::Battler* battler) {
    // however, make an exception if a monster is starting an action
    if (battler->isMonster() == true) monsterAction = true;
    return true;
}

bool onBattlerActionDone(RPG::Battler* battler, bool success) {
    resumeAtb();
    if (monsterAction == true) monsterAction = false; // the monster has finished its action
    return true;
}

// do not move ATB bar when event commands are running
bool onEventCommand(RPG::EventScriptLine* scriptLine, RPG::EventScriptData* scriptData,
                    int eventId, int pageId, int lineId, int* nextLineId) {
    if (eventId == 0) { // battle events
        if (scriptData->currentLineId == 0) atbWait = true; // when an event page starts
        else if (scriptLine->command == RPG::EVCMD_END_OF_EVENT) // when an event page ends
            resumeAtb();
    }
    return true;
}

bool onComment ( const char* text,
                 const RPG::ParsedCommentData* parsedData,
                 RPG::EventScriptLine* nextScriptLine,
                 RPG::EventScriptData* scriptData,
                 int eventId,
                 int pageId,
                 int lineId,
                 int* nextLineId )
{
    std::string cmd = parsedData->command;
    if(!cmd.compare("init_hero_speed"))
    {
        for (int i=0; i<4; i++){
            RPG::variables[confBattlerStartVar+i] = RPG::variables[confHeroSpeedVarM];
        }
    }
    if(!cmd.compare("init_monster_speed"))
    {
        for (int i=0; i<8; i++){
            RPG::variables[confBattlerStartVar+4+i] = RPG::variables[confMonsterSpeedVarM];
        }
    }
    if(!cmd.compare("condition_speed_check"))
    {
        // parameter 1: condition ID
        if (parsedData->parametersCount >= 3 && parsedData->parameters[0].type == RPG::PARAM_NUMBER
            && parsedData->parameters[1].type == RPG::PARAM_NUMBER
            && parsedData->parameters[2].type == RPG::PARAM_NUMBER) {
            for (int i=0; i<8; i++){
                if (i<4){
                    if (RPG::Actor::partyMember(i) != NULL
                        && RPG::Actor::partyMember(i)->conditions[parsedData->parameters[0].number] != 0)
                        RPG::variables[confBattlerStartVar+i] = parsedData->parameters[1].number;
                }
                if (RPG::monsters[i] != NULL
                    && RPG::monsters[i]->conditions[parsedData->parameters[0].number] != 0)
                    RPG::variables[confBattlerStartVar+4+i] = parsedData->parameters[2].number;
            }
        }
    }
    return true;
}

void onExit() {
    exceptionVect.clear();
}

