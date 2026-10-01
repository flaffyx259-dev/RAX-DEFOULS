#pragma once

class Movement {
public:
	bool   m_stopping;
	bool   m_invert;
	bool   m_fakewalk;
	bool   m_auto_peek;
	bool   m_done_retreating;
	vec3_t m_retreat_origin;
public:
	float GetMaxSpeed( );

	void JumpRelated( );
	void Strafe( );
	void FixMove( CUserCmd* cmd, const ang_t& old_angles );
	void QuickStop( );
	void AutoStop( );
	void FakeWalk( );
	void AutoPeek( );
	void HandleWalk( );
};

extern Movement g_movement;