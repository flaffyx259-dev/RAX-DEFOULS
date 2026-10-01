#pragma once

enum ShotStatus {
	STATUS_INVALID = -1,
	STATUS_MISS_ANIMATION,
	STATUS_MISS_SPREAD,
	STATUS_MISS_DEATH,
	STATUS_HIT
};

class ShotRecord {
public:
	ShotRecord( ) {
		m_record = {};
		m_target_index = -1;
		m_tick = -1;
		m_lat = 0.f;
		m_range = 8912.f;
		m_safety = 0.f;
		m_start = { 0, 0, 0 };
		m_status = ShotStatus::STATUS_INVALID;
		m_logged = false;
	}

public:
	int       m_tick;
	int       m_target_index;
	int       m_target_hitgroup;
	int       m_simulated_hitgroup;
	int       m_hitgroup;
	int       m_status;

	float     m_lat;
	float     m_range;
	float     m_safety;

	LagRecord m_record;
	vec3_t    m_start;

	bool      m_logged;
};

class Shots {
public:
	std::array< std::string, 9 > m_groups = {
	XOR( "body" ),
	XOR( "head" ),
	XOR( "chest" ),
	XOR( "stomach" ),
	XOR( "left arm" ),
	XOR( "right arm" ),
	XOR( "left leg" ),
	XOR( "right leg" ),
	XOR( "neck" )
	};

	void OnShotFire( PointData* data );
	void OnImpact( IGameEvent* evt );
	void OnHurt( IGameEvent* evt );
	void HandleMisses( );

public:
	std::deque< ShotRecord >          m_shots;
};

extern Shots g_shots;