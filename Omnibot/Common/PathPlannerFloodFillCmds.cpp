////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PathPlannerFloodFill.h"
#include "ScriptManager.h"
#include "IGameManager.h"
#include "Waypoint.h"
#include "IGame.h"
#include "Client.h"
#include "Timer.h"

#include "RenderBuffer.h"

using namespace std;

extern float g_fBottomWaypointOffset;

extern float g_CharacterHeight;;
extern float g_CharacterCrouchHeight;;
extern float g_CharacterStepHeight;
extern float g_CharacterJumpHeight;
extern float g_GridRadius;

//////////////////////////////////////////////////////////////////////////

void PathPlannerFloodFill::InitCommands()
{
	PathPlannerBase::InitCommands();

	SetEx("nav_save", "Save current navigation to disk",
		this, &PathPlannerFloodFill::cmdNavSave);
	SetEx("nav_load", "Load last saved navigation from disk",
		this, &PathPlannerFloodFill::cmdNavLoad);
	SetEx("nav_view", "Turn on/off navmesh visibility.",
		this, &PathPlannerFloodFill::cmdNavView);
	SetEx("nav_viewconnections", "Turn on/off navmesh connection visibility.",
		this, &PathPlannerFloodFill::cmdNavViewConnections);

	//////////////////////////////////////////////////////////////////////////
	SetEx("nav_enablestep", "Enable step by step generation process.",
		this, &PathPlannerFloodFill::cmdNavEnableStep);
	SetEx("nav_step", "Step to the next nav process.",
		this, &PathPlannerFloodFill::cmdNavStep);
	SetEx("nav_addfloodstart", "Adds a starting node for the flood fill.",
		this, &PathPlannerFloodFill::cmdAddFloodStart);
	SetEx("nav_clearfloodstart", "Clear all flood fill starts.",
		this, &PathPlannerFloodFill::cmdClearFloodStarts);
	SetEx("nav_savefloodstart", "Save all flood fill starts to <mapname>.navstarts.",
		this, &PathPlannerFloodFill::cmdSaveFloodStarts);
	SetEx("nav_loadfloodstart", "Load all flood fill starts from <mapname>.navstarts.",
		this, &PathPlannerFloodFill::cmdLoadFloodStarts);
	SetEx("nav_floodfill", "Start the flood fill process.",
		this, &PathPlannerFloodFill::cmdNavMeshFloodFill);
	//////////////////////////////////////////////////////////////////////////
	SetEx("nav_autofeature", "Automatically waypoints jump pads, teleporters, player spawns.",
		this, &PathPlannerFloodFill::cmdAutoBuildFeatures);

	//////////////////////////////////////////////////////////////////////////

	/*SetEx("nav_loadobj", "Loads navmesh from obj file.",
	this, &PathPlannerFloodFill::cmdLoadObj);
	SetEx("nav_loadmap", "Loads navmesh from map file.",
	this, &PathPlannerFloodFill::cmdLoadMap);*/

	SetEx("nav_next", "Steps the current tool to the next operation.",
		this, &PathPlannerFloodFill::cmdNext);

	// INFLUENCE MAP EXPERIMENTATION
	SetEx("nav_mapcreate", "Creates an influence map.",
		this, &PathPlannerFloodFill::cmdInfluenceMapCreate);
	SetEx("nav_mapseed", "Adds a seed point to the map for exploration.",
		this, &PathPlannerFloodFill::cmdInfluenceMapSeed);
	SetEx("nav_mapmem", "Shows the memory usage of the map.",
		this, &PathPlannerFloodFill::cmdInfluenceMapMem);
	SetEx("nav_mapsave", "Saves the influence map.",
		this, &PathPlannerFloodFill::cmdInfluenceMapSave);
	SetEx("nav_mapload", "Load the influence map.",
		this, &PathPlannerFloodFill::cmdInfluenceMapLoad);

	SetEx("nav_mapflood", "Load the influence map.",
		this, &PathPlannerFloodFill::cmdInfluenceMapFlood);

	/*REGISTER_STATE(PathPlannerFloodFill,NoOp);

	SetNextState(NoOp);*/
}

void PathPlannerFloodFill::cmdNavSave(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(Save(g_EngineFuncs->GetMapName()))
	{
		EngineFuncs::ConsoleMessage("Saved Nav.");
	}
	else
		EngineFuncs::ConsoleError("ERROR Saving Nav.");
}

void PathPlannerFloodFill::cmdNavLoad(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	if(Load(g_EngineFuncs->GetMapName()))
	{
		EngineFuncs::ConsoleMessage("Loaded Nav.");
	}
	else
		EngineFuncs::ConsoleError("ERROR Loading Nav.");
}

void PathPlannerFloodFill::cmdNavView(const StringVector &_args)
{
	const char *strUsage[] =
	{
		"nav_view enable[bool]",
		"> enable: Enable nav rendering. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	ScriptManager::GetInstance()->ExecuteStringLogged(
		(std::string)va("Nav.EnableView( %s );",
		bEnable ? "true" : "false"));
}

void PathPlannerFloodFill::cmdNavViewConnections(const StringVector &_args)
{
	const char *strUsage[] =
	{
		"nav_viewconnections enable[bool]",
		"> enable: Enable nav connection rendering. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	ScriptManager::GetInstance()->ExecuteStringLogged(
		(std::string)va("Nav.EnableViewConnection( %s );",
		bEnable ? "true" : "false"));
}

void PathPlannerFloodFill::cmdNavEnableStep(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const char *strUsage[] =
	{
		"nav_enablestep enable[bool]",
		"> enable: Enable step by step nav generation. true/false/on/off/1/0",
	};

	CHECK_NUM_PARAMS(_args, 2, strUsage);
	CHECK_BOOL_PARAM(bEnable, 1, strUsage);
	ScriptManager::GetInstance()->ExecuteStringLogged(
		(std::string)va("Nav.EnableStep( %s );",
		bEnable ? "true" : "false"));
}

void PathPlannerFloodFill::cmdNavStep(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;
	ScriptManager::GetInstance()->ExecuteStringLogged("Nav.Step();");
}

void PathPlannerFloodFill::cmdAddFloodStart(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	Vector3f vPosition;
	if(SUCCESS(g_EngineFuncs->GetEntityPosition(Utils::GetLocalEntity(), vPosition)))
	{
		ScriptManager::GetInstance()->ExecuteStringLogged(
			(std::string)va("Nav.AddFloodStart( Vector3(%f, %f, %f) );",
			vPosition.X(), vPosition.Y(), vPosition.Z()));
	}
}

void PathPlannerFloodFill::cmdClearFloodStarts(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;
	ScriptManager::GetInstance()->ExecuteStringLogged("Nav.ClearFloodStarts();");
}

void PathPlannerFloodFill::cmdSaveFloodStarts(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;
	ScriptManager::GetInstance()->ExecuteStringLogged("Nav.SaveFloodStarts();");
}

void PathPlannerFloodFill::cmdLoadFloodStarts(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;
	ScriptManager::GetInstance()->ExecuteStringLogged("Nav.LoadFloodStarts();");
}

void PathPlannerFloodFill::cmdNavMeshFloodFill(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const char *strUsage[] =
	{
		"nav_floodfill gridradius[#] stepheight[#] jumpheight[#] characterheight[#] charactercrouchheight[#]",
		"> gridradius: radius of the grid cell to test with",
		"> stepheight: the height an entity can step before being blocked by an edge",
		"> jumpheight: the height an entity can jump before being blocked by an edge",
		"> charactercrouchheight: the height the character",
		"> characterheight: the height the character",
	};

	PRINT_USAGE(strUsage);

	OPTIONAL_FLOAT_PARAM(fGridRadius, 1, g_GridRadius);
	OPTIONAL_FLOAT_PARAM(fStepHeight, 2, g_CharacterStepHeight);
	OPTIONAL_FLOAT_PARAM(fJumpHeight, 3, g_CharacterJumpHeight);
	OPTIONAL_FLOAT_PARAM(fCrouchHeight, 4, g_CharacterCrouchHeight);
	OPTIONAL_FLOAT_PARAM(fCharacterHeight, 5, g_CharacterHeight);

	ScriptManager::GetInstance()->ExecuteStringLogged(
		(std::string)va("Nav.FloodFill( %f, %f, %f, %f, %f );",
		fGridRadius,
		fStepHeight,
		fJumpHeight,
		fCrouchHeight,
		fCharacterHeight));
}

//////////////////////////////////////////////////////////////////////////

void PathPlannerFloodFill::cmdAutoBuildFeatures(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;

	const int iMaxFeatures = 128;
	AutoNavFeature features[iMaxFeatures];
	int iNumFeatures = g_EngineFuncs->GetAutoNavFeatures(features, iMaxFeatures);
	for(int i = 0; i < iNumFeatures; ++i)
	{
		Vector3f vPos(features[i].m_Position);
		Vector3f vFace(features[i].m_Facing);
		Vector3f vTarget(features[i].m_TargetPosition);

		// Adjust for waypoint offsets
		if(!features[i].m_Bounds.IsZero())
		{
			features[i].m_Bounds.CenterBottom(vPos);
			vPos.Z() -= g_fBottomWaypointOffset;
		}
		if(!features[i].m_TargetBounds.IsZero())
		{
			features[i].m_TargetBounds.CenterBottom(vTarget);
			vTarget.Z() -= g_fBottomWaypointOffset;
		}

		AddFloodStart(vPos);
		if(vPos != vTarget)
		{
			AddFloodStart(vTarget);
			//pFeature->ConnectTo(pTarget);
		}

		//////////////////////////////////////////////////////////////////////////
		RenderBuffer::AddLine(vPos, vPos+Vector3f::UNIT_Z * 32.f, COLOR::GREEN);
		if(vPos != vTarget)
		{
			RenderBuffer::AddLine(vPos, vTarget, COLOR::MAGENTA);
			RenderBuffer::AddLine(vTarget, vTarget+Vector3f::UNIT_Z * 32.f, COLOR::RED);
		}
		RenderBuffer::AddAABB(features[i].m_Bounds, COLOR::GREEN);
		//////////////////////////////////////////////////////////////////////////
	}
	EngineFuncs::ConsoleMessage(va("Found %d nav features.", iNumFeatures));
}

//void PathPlannerFloodFill::cmdLoadObj(const StringVector &_args)
//{
//	std::string s = "J:/CVS/decompilers/" + std::string(g_EngineFuncs->GetMapName()) + ".obj";
//	std::string line;
//	char trash;
//
//	m_PolyIndexList.clear();
//
//	std::fstream f;
//	f.open(s.c_str(), std::ios_base::in);
//	if(f.is_open())
//	{
//		while(!f.eof())
//		{
//			std::getline(f, line);
//			if(line[0] == '#')
//				continue;
//			if(line.empty())
//				continue;
//
//			if(line.compare(0,1,"v",0,1) == 0)
//			{
//				Vector3f v;
//
//				std::stringstream st;
//				st << line;
//				st >> trash >> v;
//
//				m_Vertices.push_back(v);
//				continue;
//			}
//
//			if(line.compare(0,2,"vt",0,2) == 0)
//			{
//				// textures
//				continue;
//			}
//
//			if(line.compare(0,2,"vn",0,2) == 0)
//			{
//				// normals
//				continue;
//			}
//
//			if(line.compare(0,1,"f",0,1) == 0)
//			{
//				StringList tokens;
//				std::string buffer;
//				std::stringstream st;
//				st << line;
//				while(st >> buffer)
//					tokens.push_back(buffer);
//
//				int vindex, tindex, nindex;
//				IndexList il;
//
//				StringList::iterator it = tokens.begin(), itEnd = tokens.end();
//				for(; it != itEnd; ++it)
//				{
//					const std::string &token = (*it);
//					if(token == "f")
//						continue;
//
//					std::stringstream st;
//					st << token;
//
//					obuint32 c = std::count(token.begin(), token.end(), '/');
//					switch(c)
//					{
//					case 0:
//						st >> vindex;
//						il.push_back(vindex-1);
//						break;
//					case 1:
//						st >> vindex >> trash >> tindex;
//						il.push_back(vindex-1);
//						break;
//					case 3:
//						st >> vindex >> trash >> tindex >> trash >> nindex;
//						il.push_back(vindex-1);
//						break;
//					}
//				}
//
//				// Skip all with a bad normal.
//				obuint32 c = 0;
//				Vector3f vNormal = Vector3f::ZERO;
//				while(vNormal == Vector3f::ZERO)
//				{
//					Vector3f v1 = m_Vertices[il[c+1]] - m_Vertices[il[c]];
//					Vector3f v2 = m_Vertices[il[c+2]] - m_Vertices[il[c]];
//					vNormal = v1.UnitCross(v2);
//					++c;
//				}
//
//				if(Utils::AngleBetween(vNormal, Vector3f::UNIT_Z) > Mathf::DegToRad(45.f))
//					continue;
//
//				m_PolyIndexList.push_back(il);
//				continue;
//			}
//		}
//		f.close();
//	}
//}

//void PathPlannerFloodFill::cmdLoadMap(const StringVector &_args)
//{
//	//std::string s = "J:/CVS/decompilers/" + std::string(g_EngineFuncs->GetMapName()) + ".map";
//	std::string s = "J:/CVS/decompilers/q4ctf1.map";
//
//	std::string line, tmp;
//	char trash;
//	int versionnum = 3;
//	//int bracket = 0;
//
//	m_NavSectors.clear();
//	ReleaseCollision();
//
//	enum MapMode
//	{
//		MAP_UNKNOWN,
//		MAP_PRIMITIVE,
//	};
//
//	MapMode m = MAP_UNKNOWN;
//
//	obuint32 iNumBrushes = 0;
//
//	std::fstream f;
//	f.open(s.c_str(), std::ios_base::in);
//	if(f.is_open())
//	{
//		while(!f.eof())
//		{
//			std::getline(f, line);
//			if(line[0] == '/' && line[1] == '/')
//				continue;
//			if(line.empty())
//				continue;
//
//			if(line.compare(0,7,"Version",0,7) == 0)
//			{
//				std::stringstream st;
//				st << line;
//				st >> tmp >> versionnum;
//				continue;
//			}
//
//			if(line[0]=='{' || line[0]=='}')
//				continue;
//
//			int c = 0;
//			while(line[c] == ' ')
//				++c;
//
//			if(line.compare(0,11,"\"classname\"",0,11) == 0)
//			{
//				continue;
//			}
//
//			/*if(line[c]=='{')
//				bracket++;
//			if(line[c]=='}')
//				bracket--;*/
//
//			if(c != 0)
//				line.erase(0,c);
//
//			if(line.compare(0,8,"brushDef",0,8) == 0)
//			{
//				m = MAP_PRIMITIVE;
//				continue;
//			}
//
//			if(line[0]=='{')
//			{
//				switch(m)
//				{
//				case MAP_PRIMITIVE:
//					{
//						++iNumBrushes;
//
//						PlaneList planes;
//
//						while(line[0]!='}')
//						{
//							std::getline(f, line);
//
//							Plane3f pl;
//
//							std::stringstream st;
//							st << line;
//							st >> trash >> pl >> trash;
//							if(st.good())
//								planes.push_back(pl);
//						}
//
//						// contruct polys from the list
//						obuint32 wp = 0;
//						bool bDidSomething = true;
//						while(bDidSomething)
//						{
//							m_CurrentSector.clear();
//							bDidSomething = false;
//
//							for(; wp < planes.size(); ++wp)
//							{
//								if(Utils::AngleBetween(planes[wp].Normal, Vector3f::UNIT_Z) > Mathf::DegToRad(45.f))
//									continue;
//
//								// build a poly from this walkface
//								bDidSomething = true;
//								m_CurrentSector = Utils::CreatePolygon(planes[wp].Normal*planes[wp].Constant, planes[wp].Normal);
//								break;
//							}
//
//							// Clip the polygon
//							if(!m_CurrentSector.empty())
//							{
//								for(obuint32 p = 0; p < planes.size(); ++p)
//								{
//									if(wp==p)
//										continue;
//
//									if(planes[p].Normal == planes[wp].Normal ||
//										planes[p].Normal == -planes[wp].Normal)
//										continue;
//
//									m_CurrentSector = Utils::ClipPolygonToPlanes(m_CurrentSector, planes[p], true);
//
//									if(m_CurrentSector.empty())
//										break;
//								}
//							}
//
//							// Commit the polygon
//							if(m_CurrentSector.size() > 2)
//							{
//								NavSector ns;
//								ns.m_Boundary = m_CurrentSector;
//								m_NavSectors.push_back(ns);
//
//								m_CurrentSector.clear();
//							}
//
//							++wp;
//						}
//						break;
//					}
//				case MAP_UNKNOWN:
//					continue;
//				}
//			}
//		}
//		f.close();
//	}
//
//	InitCollision();
//}

void PathPlannerFloodFill::_BenchmarkPathFinder(const StringVector &_args)
{
	EngineFuncs::ConsoleMessage("-= FloodFill PathFind Benchmark =-");

	double dTimeTaken = 0.0f;
	obint32 iNumSectors = 0;//(obint32)mRuntimeSectors.size();
	obint32 iNumPaths = iNumSectors * iNumSectors;

	Timer tme;
	tme.Reset();
	for(obint32 w1 = 0; w1 < iNumSectors; ++w1)
	{
		for(obint32 w2 = 0; w2 < iNumSectors; ++w2)
		{
			/*const NavSector &pS1 = mRuntimeSectors[w1];
			const NavSector &pS2 = mRuntimeSectors[w2];

			PlanPathToGoal(NULL,
			pS1.m_Middle+Vector3f(0,0,NavigationMeshOptions::CharacterHeight),
			pS2.m_Middle+Vector3f(0,0,NavigationMeshOptions::CharacterHeight),
			0);*/
		}
	}
	dTimeTaken = tme.GetElapsedSeconds();

	EngineFuncs::ConsoleMessage(va("generated %d paths in %f seconds: %f paths/sec",
		iNumPaths, dTimeTaken, dTimeTaken != 0.0f ? (float)iNumPaths / dTimeTaken : 0.0f));
}

void PathPlannerFloodFill::_BenchmarkGetNavPoint(const StringVector &_args)
{
	obuint32 iNumIterations = 1;
	if(_args.size() > 1)
	{
		iNumIterations = atoi(_args[1].c_str());
		if(iNumIterations <= 0)
			iNumIterations = 1;
	}

	EngineFuncs::ConsoleMessage("-= FloodFill GetNavPoint Benchmark  =-");

	/*double dTimeTaken = 0.0f;
	obuint32 iNumWaypoints = mRuntimeSectors.size();
	Timer tme;

	obuint32 iHits = 0, iMisses = 0;
	tme.Reset();
	for(obuint32 i = 0; i < iNumIterations; ++i)
	{
	for(obuint32 w1 = 0; w1 < iNumWaypoints; ++w1)
	{
	NavSector *pSector = mRuntimeSectors[w1];

	Waypoint *pClosest = _GetClosestWaypoint(pWaypoint->GetPosition(), (NavFlags)0, true);
	if(pClosest)
	++iHits;
	else
	++iMisses;
	}
	}

	dTimeTaken = tme.GetElapsedSeconds();

	EngineFuncs::ConsoleMessage("_GetClosest() %d calls, %d hits, %d misses : avg %f per second",
	iNumWaypoints * iNumIterations,
	iHits,
	iMisses,
	dTimeTaken != 0.0f ? ((float)(iNumWaypoints * iNumIterations) / dTimeTaken) : 0.0f);	*/
}

void PathPlannerFloodFill::cmdNext(const StringVector &_args)
{
	if(!m_PlannerFlags.CheckFlag(NAV_VIEW))
		return;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void PathPlannerFloodFill::cmdInfluenceMapCreate(const StringVector &_args)
{
	const float fmin = std::numeric_limits<float>::lowest();
	const float fmax = std::numeric_limits<float>::max();

	AABB mapbounds;
	mapbounds.Set( Vector3f( -4400,-2000,-800 ), Vector3f( 4400, 2000, 230 ) );
	
	VectorQueue empty;
	mSpanFrontier.swap( empty );

	RenderBuffer::StaticBufferDelete( mInfluenceBufferId );
	mInfluenceBufferId = 0;

	OB_DELETE( mInfluence );

	mSpanMap.Clear();
	mSpanMap.Init( Vector3f(mapbounds.m_Mins), Vector3f(mapbounds.m_Maxs), 16.0f );

	EngineFuncs::ConsoleMessage(va("Created %d x %d span map",
		mSpanMap.GetNumCellsX(), mSpanMap.GetNumCellsY() ) );
}

void PathPlannerFloodFill::cmdInfluenceMapSeed(const StringVector &_args)
{
	/*Vector3f eyePos;
	if ( Utils::GetLocalEyePosition(eyePos) )
	mSpanFrontier.push( eyePos );*/

	enum { MaxFeatures = 64 };
	AutoNavFeature features[ MaxFeatures ];
	const int numFeatures = g_EngineFuncs->GetAutoNavFeatures( features, MaxFeatures );
	for ( int i = 0; i < numFeatures; ++i )
	{
		mSpanFrontier.push( features[ i ].m_Position );
		mSpanFrontier.push( features[ i ].m_TargetPosition );
	}
	
	/*if ( mSpanMap == NULL )
	{
		EngineFuncs::ConsoleMessage( "No Influence Map Created, use nav_mapcreate" );
		return;
	}
	
	while( !mSpanFrontier.empty() )
	{
		Vector3f search = mSpanFrontier.front();
		mSpanFrontier.pop();

		Vector3f floor, ceiling;

		obTraceResult tr;
		EngineFuncs::TraceLine(tr,
			search + Vector3f( 0.f, 0.f, 1.f ),
			search - Vector3f( 0.f, 0.f, 1024.f ),
			NULL,TR_MASK_FLOODFILL,-1,False);
		
		floor = Vector3f( tr.m_Endpos );

		EngineFuncs::TraceLine(tr,
			floor + Vector3f( 0.f, 0.f, 1.f ),
			floor + Vector3f( 0.f, 0.f, 1024.f ),
			NULL,TR_MASK_FLOODFILL,-1,False);

		if ( mSpanMap->AddOpenSpan( floor, tr.m_Endpos[ 2 ] - floor.Z() ) )
		{
			const Vector3f directions[4] =
			{
				Vector3f( -mSpanMap->GetCellSize(), 0.0f ),
				Vector3f(  mSpanMap->GetCellSize(), 0.0f ),
				Vector3f( 0.0f, -mSpanMap->GetCellSize() ),
				Vector3f( 0.0f, mSpanMap->GetCellSize() )
			};

			for ( int i = 0; i < 4; ++i )
			{
			}
		}
	}*/
}

void PathPlannerFloodFill::cmdInfluenceMapMem(const StringVector &_args)
{
	EngineFuncs::ConsoleMessage(va("Influence Map %d x %d ( %s )",
		mSpanMap.GetNumCellsX(),
		mSpanMap.GetNumCellsY(),
		Utils::FormatByteString( mSpanMap.CalculateMemUsage() ).c_str() ) );
}

void PathPlannerFloodFill::cmdInfluenceMapSave(const StringVector &_args)
{
	const std::string filePath	= std::string("nav/") + std::string(g_EngineFuncs->GetMapName()) + ".influence";

	std::string data;
	if ( mSpanMap.Serialize( data ) )
	{
		File f;
		if(f.OpenForWrite( filePath.c_str() ,File::Binary ) )
		{
			f.Write( data.c_str(), data.length() );
		}
	}
}

void PathPlannerFloodFill::cmdInfluenceMapLoad(const StringVector &_args)
{
	const std::string filePath	= std::string("nav/") + std::string(g_EngineFuncs->GetMapName()) + ".influence";

	mSpanMap.Clear();

	File f;
	if(!f.OpenForRead( filePath.c_str() ,File::Binary ) )
	{
		EngineFuncs::ConsoleError( va( "Influence Map %s not found", filePath.c_str() ) );
		return;
	}

	std::string data;
	if (!f.ReadWholeFile( data ) )
	{
		EngineFuncs::ConsoleError( va( "Influence Map Read Error %s", filePath.c_str() ) );
		return;
	}

	if ( !mSpanMap.DeSerialize( data ) )
	{
		EngineFuncs::ConsoleError( va( "Influence Map Parse Error %s", filePath.c_str() ) );
		return;
	}
}

void PathPlannerFloodFill::cmdInfluenceMapFlood(const StringVector &_args)
{
	Vector3f vAimPt;
	if ( !Utils::GetLocalAimPoint( vAimPt ))
		return;

	mInfluence = mSpanMap.CreateInfluenceLayer();
	mInfluence->Reset();
	mInfluence->AddSeed( vAimPt, 0.0f );

	Timer t;
	while( !mInfluence->UpdateInfluences( std::numeric_limits<int>::max() ) )
	{
	}

	EngineFuncs::ConsoleError( va( "Influence Flooded in %.3f sec ( %s )",
		t.GetElapsedSeconds(),
		Utils::FormatByteString( mInfluence->CalculateMemUsage() ).c_str() ) );
}