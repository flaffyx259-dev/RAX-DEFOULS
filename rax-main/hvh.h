#pragma once

class AdaptiveAngle {
public:
	float m_yaw;
	float m_dist;

public:
	// ctor.
	__forceinline AdaptiveAngle( float yaw, float penalty = 0.f ) {
		// set yaw.
		m_yaw = math::NormalizedAngle( yaw );

		// init distance.
		m_dist = 0.f;

		// remove penalty.
		m_dist -= penalty;
	}
};

enum AntiAimMode : size_t {
	STAND = 0,
	WALK,
	AIR,
};

class HVH {
public:
	bool    m_invert_desync;

	float   m_last_moving_yaw;
	float   m_last_standing_yaw;
	float   m_flick_yaw;
	float   m_direction;
	float   m_view;

	bool    m_maintain_desync;
	bool    m_on_ground;
	bool    m_will_land;
	bool    m_can_break;
	bool    m_hide_yaw;
	int     m_mode;

	int     m_front;
	int     m_back;
	int     m_right;
	int     m_left;

	Player* m_target;

	bool    m_updated;
	int     m_updates;

	bool    m_desync;
	bool    m_desync_flip;
public:
	bool    m_old_peeking;
	bool    m_peeking;
	float   m_last_move_time;
public:
	bool IsPeeking( );

	void AutoDirection( );
	bool HandleLBY( );
	void HandleDirection( );
	void HandleReal( );
	void AntiAim( );
	void SendPacket( );
	void HandleShotChoke( );
	void MicroMovement( );
};

extern HVH g_hvh;