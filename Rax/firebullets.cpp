#include "includes.h"

class C_TEFireBullets {
public:
	PAD( 0xC );
	int		m_index;
	int     m_item_id;
	vec3_t	m_origin;
	ang_t	m_angles;
	int		m_weapon_id;
	int		m_mode;
	int		m_seed;
	float	m_spread;
};