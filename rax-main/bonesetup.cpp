#include "includes.h"

Bones g_bones{};;

bool Bones::Setup( Player* target, const int& mask, const float& curtime, BoneArray* out ) {
	alignas( 16 ) vec3_t		  pos[ 128 ];
	alignas( 16 ) quaternion_t    q[ 128 ];

	CStudioHdr* hdr = target->GetModelPtr( );
	if ( !hdr )
		return false;

	setup_bones::allow = true;

	const float backup_curtime = g_csgo.m_globals->m_curtime;
	const float backup_frametime = g_csgo.m_globals->m_frametime;
	const float backup_weight = target->m_AnimOverlay( )[ 12 ].m_weight;
	const int backup_eflags = target->m_iEFlags( );

	g_csgo.m_globals->m_curtime = curtime;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	target->m_AnimOverlay( )[ 12 ].m_weight = 0.f;

	target->InvalidateBoneCache( );

	target->m_iEFlags( ) |= EFL_SETTING_UP_BONES;

	// first we setup needed shit for bones
	target->StandardBlendingRules( hdr, pos, q, curtime, mask );

	// build chain.
	static int32_t chain[ 128 ] = {};
	const auto chain_length = hdr->m_studio_hdr->m_num_bones;
	for ( auto i = 0; i < chain_length; i++ )
		chain[ chain_length - i - 1 ] = i;

	// build transformations.
	// (this actually set the bones up)
	static matrix3x4_t rotation;
	math::AngleMatrix( target->m_angAbsRotation( ), target->m_vecAbsOrigin( ), rotation );
	for ( auto j = chain_length - 1; j >= 0; j-- )
	{
		const auto i = chain[ j ];
		const auto parent = hdr->m_bone_parent.Count( ) > i ? &hdr->m_bone_parent[ i ] : nullptr;

		if ( !parent )
			continue;

		static matrix3x4_t qua;
		qua = math::QuaternionMatrix( q[ i ], pos[ i ] );

		if ( *parent == -1 )
			math::ConcatTransforms( rotation, qua, out[ i ] );
		else
			math::ConcatTransforms( out[ *parent ], qua, out[ i ] );
	}

	// start interpolation again.
	g_csgo.m_globals->m_curtime = backup_curtime;
	g_csgo.m_globals->m_frametime = backup_frametime;
	target->m_AnimOverlay( )[ 12 ].m_weight = backup_weight;
	target->m_iEFlags( ) = backup_eflags;

	setup_bones::allow = false;

	return true;
}