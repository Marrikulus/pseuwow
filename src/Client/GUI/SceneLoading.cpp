#include "common.h"
#include <sstream>
#include <math.h>
#include "common.h"
#include "PseuGUI.h"
#include "PseuWoW.h"
#include "Scene.h"
#include "DrawObject.h"
#include "MapTile.h"
#include "World/MapMgr.h"
#include "World/WorldSession.h"
#include "World/World.h"
#include "MemoryInterface.h"
#include <iostream>



 SceneLoading::SceneLoading(PseuGUI *g) : Scene(g)
 {  
	DEBUG(logdebug("SceneLoading: Loading models..."));
	Ldebugmode = false;
	// store some pointers right now to prevent repeated ptr dereferencing later (speeds up code)
    gui = g;
    Lwsession = gui->GetInstance()->GetWSession();
    Lworld = Lwsession->GetWorld();
    Lmapmgr = Lworld->GetMapMgr();
	Finished = false;
	LoadingTime = false;
	loaded = 0;
	percent = 0;
	
	//open map and loading screen dbc
	SCPDatabase *mapDbc = instance->dbmgr.GetDB("map");
	SCPDatabase *lsDbc = instance->dbmgr.GetDB("loadingscreens");
	uint32 ls_ID = mapDbc->GetUint32(Lworld->GetMapId(),"loadingscreen");      //get loadingscreen id for our mapID
	ls_path = lsDbc->GetString(ls_ID,"loadingscreen");  //get the loadingscreen path string for our loadingscreen ID
	//logdetail("SceneLoading:: mapid= %u lsId= %u lsPath= %s",Lworld->GetMapId(),ls_ID,ls_path.c_str());

	dimension2d<u32> scrn = driver->getScreenSize();
	
	loadingscreen = guienv->addImage(driver->getTexture(io::IrrCreateIReadFileBasic(device,ls_path.c_str())), core::position2d<s32>(5,5),true,rootgui);
	loadingscreen->setRelativePosition(rect<s32>(0,0,scrn.Width,scrn.Height));
	loadingscreen->setScaleImage(true);

	if (AllDoodads.empty() == true) //fill list
	{
		// TODO: at some later point we will need the geometry for correct collision calculation, etc...
		
		for(uint32 tiley = 0; tiley < 3; tiley++)
		{
			for(uint32 tilex = 0; tilex < 3; tilex++)
			{
				MapTile *maptile = Lworld->GetMapMgr()->GetNearTile(tilex - 1, tiley - 1);
				if(maptile)
				{
					for(uint32 i = 0; i < maptile->GetDoodadCount(); i++)
					{
						Doodad *doo = maptile->GetDoodad(i);
						// add each doodad to a list of models
						AllDoodads.push_back (doo->MPQpath);
					
					}
				}
			}
		}
	}
	// only keep models that are unique
	AllDoodads.sort();
	AllDoodads.unique();
	doodadtotal = AllDoodads.size();

 }

void SceneLoading::OnUpdate(s32 timediff)
{
	if (LoadingTime == true)
	{
		if (AllDoodads.empty() != true)
		{
			// using the "IF" statement here alows us to load one model and one skin per scene cycle
			std::string m2model = AllDoodads.back(); //get last elemment from the list
			AllDoodads.pop_back();                   //remove used element from list
			io::IReadFile* modelfile = io::IrrCreateIReadFileBasic(device, m2model.c_str()); // load with MDH
			logdetail("SceneLoading:: loaded model %s",m2model);
			std::string skinfile = m2model.append(4, 'x');
			NormalizeFilename(skinfile); //is this nessasary?
			skinfile.replace(skinfile.end()-7,skinfile.end(),"00.skin");
			io::IReadFile* modelskin = io::IrrCreateIReadFileBasic(device, skinfile.c_str()); //load skin with MDH
				
		}
	}
	LoadingTime = true;
	loaded = (doodadtotal - AllDoodads.size());

	if (loaded != 0)
	{percent = (loaded*100)/doodadtotal;}

	//calulate loading bar size and position relative to screensize
	dimension2d<u32> scrn = driver->getScreenSize();        
	s32 barmaxlength = (scrn.Width/2.0f);
	float Width = ((barmaxlength/100.0f)*percent);
	s32 leftX = (scrn.Width/4.0f);           //left top corner horizontal position coord
	s32 leftY = ((scrn.Height/10.0f)*9.0f);         //left top corner vertical position coord
	s32 rightX = (scrn.Width/4.0f)+Width;    //bottom right horisontal coord
	s32 rightY = ((scrn.Height/10.0f)*9.0f)+20.0f;  //bottom right vertical coord, diff of y coords gives bar thickness
	
	IGUIImage *loadingbar = guienv->addImage(rect<s32>(leftX,leftY,rightX,rightY), 0, -1, L"");
	loadingbar->setImage(driver->getTexture("../bin/data/misc/bar.png"));
	
	//if we finished loading then its time to switch to the world scene
	if (AllDoodads.empty() == true)
	{
		DEBUG(logdebug("SceneLoading: finished Loading models..."));
		gui->SetSceneState(SCENESTATE_WORLD);

	}

}