////////////////////////////////////////////////////////////////////////////////
//
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __BOTBASESTATES_H__
#define __BOTBASESTATES_H__

#include "StateMachine.h"
#include "Path.h"
#include "Criteria.h"
#include "MapGoal.h"
#include "FilterSensory.h"

class gmScriptGoal;

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////

	class FollowPathUser
	{
	public:
		friend class FollowPath;

		enum FailType
		{
			None,
			NoPath,
			Blocked,
			Interrupted,
		};

		virtual bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt) { OBASSERT(0, "Not Implemented"); return false; }

		virtual void OnPathSucceeded() { m_PathSuccess = true; };
		virtual void OnPathFailed(FailType _how) { m_PathFailed = _how; };

		bool DidPathSucceed() const { return m_PathSuccess; }
		bool DidPathFail() const { return m_PathFailed!=0; }
		FailType GetFailType() const { return m_PathFailed; }

		int GetDestinationIndex() const { return m_DestinationIndex; }

		void ResetPathUser() { m_PathFailed = None; m_PathSuccess = false; }
		bool InProgress() const { return m_PathFailed==None&&!m_PathSuccess; }

		void SetSourceThread(int _threadId) { m_CallingThread = _threadId; }
		int GetSourceThread() const { return m_CallingThread; }

		obuint32 GetFollowUserName() const { return m_UserName; }

		void SetFollowUserName(obuint32 _name);
		void SetFollowUserName(const std::string &_name);

		FollowPathUser(const std::string &_user);
		FollowPathUser(obuint32 _name);
		virtual ~FollowPathUser() {}
	private:
		obuint32					m_UserName;

		int							m_CallingThread;

		int							m_DestinationIndex;

		FailType					m_PathFailed : 3;
		obuint8						m_PathSuccess : 1;

		FollowPathUser();
	};

	//////////////////////////////////////////////////////////////////////////
	class TrackTargetZone
	{
	public:
		enum { MaxTargetZones = 8 };
		struct TargetZone
		{
			Vector3f	m_Position;
			obint32		m_TargetCount;
			bool		m_InUse : 1;
		};

		void Restart(float _radius);

		void UpdateAimPosition();

		bool HasAim() const { return m_ValidAim; }

		const Vector3f &GetAimPosition() { return m_AimPosition; }

		void RenderDebug();

		void Update(Client *_client);

		TrackTargetZone();
	private:
		float			m_Radius;
		Vector3f		m_AimPosition;
		GameEntity		m_LastTarget;
		TargetZone		m_TargetZones[MaxTargetZones];
		bool			m_ValidAim;
	};
	//////////////////////////////////////////////////////////////////////////
	class AimerUser
	{
	public:
		virtual bool GetAimPosition(Vector3f &_aimpos) = 0;
		virtual void OnTarget() = 0;

		virtual ~AimerUser() {}
	private:
	};

	class Aimer : public StateChild
	{
	public:
		enum AimType
		{
			WorldPosition,
			WorldFacing,
			MoveDirection,
			UserCallback,
		};

		struct AimRequest
		{
			friend class Aimer;

			Priority::ePriority		m_Priority;
			obuint32 				m_Owner;
			AimType					m_AimType;
			Vector3f				m_AimVector;
			AimerUser				*m_AimerUser;

			void Reset();
			AimRequest();
		};

		enum { MaxAimRequests = 8 };

		bool AddAimRequest(Priority::ePriority _prio, AimerUser *_owner, obuint32 _ownername);
		bool AddAimFacingRequest(Priority::ePriority _prio, obuint32 _owner, const Vector3f &_v);
		bool AddAimPositionRequest(Priority::ePriority _prio, obuint32 _owner, const Vector3f &_v);
		bool AddAimMoveDirRequest(Priority::ePriority _prio, obuint32 _owner);

		void ReleaseAimRequest(obuint32 _owner);
		void UpdateAimRequest(obuint32 _owner, const Vector3f &_pos);

		AimRequest *GetHighestAimRequest(bool _clearontarget);

		void OnSpawn();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void GetDebugString(std::stringstream &out);

		int GetAllRequests(AimRequest *_records, int _max);

		void RenderDebug();

		Aimer();
	private:
		AimRequest	m_AimRequests[MaxAimRequests];
		obuint32 	m_BestAimOwner;

		AimRequest *FindAimRequest(obuint32 _owner);
	};
	
	//////////////////////////////////////////////////////////////////////////

	class CaptureTheFlag : public StateChild, public FollowPathUser
	{
	public:
		enum GoalState
		{
			Idle,
			GettingFlag,
			CarryingToCap,
			CarryingToHold,
			HoldingFlag,
		};

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		GoalState GetGoalState() const { return m_GoalState; }

		void GetDebugString(std::stringstream &out);
		MapGoal *GetMapGoalPtr();
		void RenderDebug();

		// FollowPathUser
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		CaptureTheFlag();
	private:
		GoalState					m_GoalState;
		int							m_LastFlagState;
		obint32						m_NextMoveTime;
		MapGoalPtr					m_MapGoalFlag;
		MapGoalPtr					m_MapGoalCap;

		Trackers					Tracker;

		bool LookForCapGoal(MapGoalPtr &ptr, GoalState &st);
	};

	//////////////////////////////////////////////////////////////////////////

	class ReturnTheFlag : public StateChild, public FollowPathUser
	{
	public:

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		//void GetDebugString(std::stringstream &out);
		MapGoal *GetMapGoalPtr();
		void RenderDebug();

		// FollowPathUser

		ReturnTheFlag();
	private:
		Vector3f			m_LastGoalPosition;
		MapGoalPtr			m_MapGoal;

		TrackInUse			m_MapGoalProg;
	};

	//////////////////////////////////////////////////////////////////////////

	//class Defend : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(std::stringstream &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	Defend();
	//private:
	//	Trackers		Tracker;
	//	MapGoalPtr		m_MapGoal;
	//	Vector3f		m_AimPosition;
	//
	//	TrackTargetZone m_TargetZone;

	//	Seconds			m_MinCampTime;
	//	Seconds			m_MaxCampTime;
	//	int				m_EquipWeapon;
	//	int				m_Stance;

	//	int				m_ExpireTime;
	//};

	//////////////////////////////////////////////////////////////////////////

	//class Attack : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(std::stringstream &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	Attack();
	//private:
	//	Trackers		Tracker;
	//	MapGoalPtr		m_MapGoal;
	//	Vector3f		m_AimPosition;

	//	TrackTargetZone m_TargetZone;

	//	Seconds			m_MinCampTime;
	//	Seconds			m_MaxCampTime;
	//	int				m_EquipWeapon;
	//	int				m_Stance;

	//	int				m_ExpireTime;
	//};

	//////////////////////////////////////////////////////////////////////////

	class Roam : public StateChild, public FollowPathUser
	{
	public:

		obReal GetPriority();

		void Exit();
		StateStatus Update(float fDt);

		Roam();
	private:
	};

	//////////////////////////////////////////////////////////////////////////

	class HighLevel : public StatePrioritized
	{
	public:
		//obReal GetPriority();

		HighLevel();
	private:
	};

	//////////////////////////////////////////////////////////////////////////

	class LookAround : public StateChild
	{
	public:

		int GetNextLookTime();

		void OnSpawn();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		LookAround();
	private:
		int		m_NextLookTime;

		float	m_Awareness[8]; // 8 cardinal directions
	};

	//////////////////////////////////////////////////////////////////////////

	class MotorControl : public StateSimultaneous
	{
	public:
		MotorControl();
	private:
	};

	//////////////////////////////////////////////////////////////////////////

	class LowLevel : public StateSimultaneous
	{
	public:
		LowLevel();
	private:
	};

	//////////////////////////////////////////////////////////////////////////

	class Main : public StateSimultaneous
	{
	public:
		obReal GetPriority();
		void Enter();
		Main();
		void OnSpawn();
	private:
		bool	m_OnSpawnCalled;
	};

	//////////////////////////////////////////////////////////////////////////

	class Dead : public StateChild
	{
	public:
		obReal GetPriority();
		StateStatus Update(float fDt);
		Dead();
	private:
		bool bForceActivate;
	};

	//////////////////////////////////////////////////////////////////////////

	class Warmup : public StateChild
	{
	public:
		obReal GetPriority();
		StateStatus Update(float fDt);
		Warmup();
	private:
	};

	//////////////////////////////////////////////////////////////////////////

	class Root : public StateFirstAvailable
	{
	public:
		Root();
	private:
	};

	//////////////////////////////////////////////////////////////////////////

	struct Event_ProximityTrigger
	{
		obuint32	m_OwnerState;
		GameEntity	m_Entity;
		Vector3f	m_Position;
	};

	class ProximityWatcher : public StateChild
	{
	public:

		enum { MaxTriggers = 8 };
		struct Trigger
		{
			obuint32	m_OwnerState;
			FilterPtr	m_SensoryFilter;
			bool		m_DeleteOnFire : 1;
		};

		void AddWatch(obuint32 _owner, FilterPtr _filter, bool _fireonce = true);
		void RemoveWatch(obuint32 _owner);

		void RenderDebug();

		obReal GetPriority();
		StateStatus Update(float fDt);

		ProximityWatcher();
	private:
		Trigger		m_Triggers[MaxTriggers];
	};

	//////////////////////////////////////////////////////////////////////////

	//class Snipe : public StateChild, public FollowPathUser, public AimerUser
	//{
	//public:

	//	void GetDebugString(std::stringstream &out);
	//	void RenderDebug();

	//	obReal GetPriority();
	//	void Enter();
	//	void Exit();
	//	StateStatus Update(float fDt);

	//	// FollowPathUser functions.
	//	bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

	//	// AimerUser functions.
	//	bool GetAimPosition(Vector3f &_aimpos);
	//	void OnTarget();

	//	Snipe();
	//private:
	//	Trackers			Tracker;
	//	MapGoalPtr			m_MapGoal;

	//	Vector3f			m_AimPosition;

	//	GameEntity			m_TargetEntity;
	//	Vector3f			m_LastTargetPos;

	//	int					m_ScopedWeaponId;
	//	int					m_NonScopedWeaponId;

	//	TrackTargetZone		m_TargetZone;

	//	Seconds				m_MinCampTime;
	//	Seconds				m_MaxCampTime;
	//	int					m_Stance;

	//	int					m_ExpireTime;
	//	int					m_NextScanTime;
	//};

	//////////////////////////////////////////////////////////////////////////

	class DeferredCaster : public StateChild
	{
	public:
		struct CastInput
		{
			Vector3f	CastStart;
			Vector3f	CastEnd;
			AABB		Bounds;
			int			Mask;
		};

		struct CastOutput
		{
			obTraceResult	Result;
			bool			Done;

			void Reset() { Done=true; }
		};

		enum Status { Pending = 0, Done };

		int AddDeferredCasts(const CastInput *_CastIn, int _NumCasts, const char *_UserName);
		Status GetDeferredCasts(int GroupId, CastOutput *_CastOut, int _NumCasts);

		void GetDebugString(std::stringstream &out);
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		DeferredCaster();
	private:
		enum { MaxCasts = 64, InvalidGroup = 0 };
		CastInput	CastInputs[MaxCasts];
		CastOutput	CastOutputs[MaxCasts];
		int			GroupId[MaxCasts];
		const char *UserName[MaxCasts];;

		int			CastReadPosition;
		int			CastWritePosition;
		int			GroupNext;
	};

	//////////////////////////////////////////////////////////////////////////

	class FloodFiller : public StateChild
	{
	public:
		enum FillState
		{
			FillIdle,
			FillInit,
			FillSearching,
			FillOpenNess,
			FillSectorize,
			FillDone,
		};

		enum Direction
		{
			DIR_NORTH,
			DIR_EAST,
			DIR_SOUTH,
			DIR_WEST,

			DIR_NUM
		};

		FillState GetFillState() const { return State; }
		void NextFillState();
		void StartFloodFill(const Vector3f &_Start, float _Radius = 16.f);

		void Reset();

		//////////////////////////////////////////////////////////////////////////

		void GetDebugString(std::stringstream &out);
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		FloodFiller();
	private:
		Vector3f	Start;
		float		Radius;

		FillState	State;

		AABB		FloodBlock;

		//////////////////////////////////////////////////////////////////////////
		struct IntOffset2d
		{
			obint16		X;
			obint16		Y;
		};
		//////////////////////////////////////////////////////////////////////////
		struct Node;
		struct Connection
		{
			Node		*Destination;
			obuint8		Cover : 4;
			obuint8		Jump : 1;

			void Reset();
		};
		struct Node
		{
			IntOffset2d		MinOffset;
			IntOffset2d		MaxOffset;
			float			Height;

			Connection		Connections[DIR_NUM];

			obuint16		SectorId;

			obuint8			OpenNess;

			obuint8			Open : 1;
			obuint8			NearObject : 1;
			obuint8			NearEdge : 1;
			obuint8			Sectorized : 1;

			void Init(obint16 _X = 0, obint16 _Y = 0, float _Height = 0.f, bool _Open = false);
		};

		Vector3f				_GetNodePosition(const Node &_Node);
		FloodFiller::Node *		_NextOpenNode();
		Node *					_NodeExists(obint16 _X, obint16 _Y, float _Height);
		bool					_TestNode(const Node *_Node);
		bool					_DropToGround(Node *_Node);
		void					_MakeConnection(Node *_NodeA, Node *_NodeB, Direction _Dir);
		void					_FillOpenNess(bool _ResetAll);
		void					_MergeSectors();
		bool					_CanMergeWith(Node *_Node, Node *_WithNode);

		enum		{ NumSectors = 8192, NumConnections = 16384 };

		Node		Nodes[NumSectors];
		int			FreeNode;
	};

	//////////////////////////////////////////////////////////////////////////
};

#endif
