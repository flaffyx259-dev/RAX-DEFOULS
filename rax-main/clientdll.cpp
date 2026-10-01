#include "includes.h"

void Hooks::LevelInitPreEntity( const char* map ) {
	float rate{ 1.f / g_csgo.m_globals->m_interval };

	// set rates when joining a server.
	g_csgo.cl_updaterate->SetValue( rate );
	g_csgo.cl_cmdrate->SetValue( rate );
	g_csgo.cl_extrapolate->SetValue( 0 );

	g_visuals.m_hit_start = g_visuals.m_hit_end = g_visuals.m_hit_duration = 0.f;

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPreEntity_t >( CHLClient::LEVELINITPREENTITY )( this, map );
}

void Hooks::LevelInitPostEntity( ) {
	g_cl.OnMapload( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPostEntity_t >( CHLClient::LEVELINITPOSTENTITY )( this );
}

void Hooks::LevelShutdown( ) {
	g_cl.m_local = nullptr;
	g_cl.m_weapon = nullptr;
	g_cl.m_processing = false;
	g_cl.m_weapon_info = nullptr;
	g_cl.m_round_end = false;
	g_csgo.m_net = nullptr;

	g_shots.m_shots.clear( );

	g_cl.m_sequences.clear( );
	packet_start::sequences.clear( );

	g_restore_data.Reset();
	g_fake_state.Reset( );
	g_rendered_state.Reset( );
	for ( RebuiltAnimState& state : g_states )
		state.Reset( );

	for ( int i = 0; i < 64; i++ )
		g_networking.GetUserData( i )->Reset( );

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelShutdown_t >( CHLClient::LEVELSHUTDOWN )( this );
}

void __stdcall create_move::hook( int sequence_number, float input_sample_frametime, bool active, bool& bSendPacket )
{
	if (g_csgo.valid_hwid() != g_csgo.m_users[0].m_hwid)
		return;

	original( g_csgo.m_client, 0, sequence_number, input_sample_frametime, active );

	CUserCmd* cmd = g_csgo.m_input->GetUserCmd( sequence_number );
	CVerifiedUserCmd* ver_cmd = g_csgo.m_input->GetVerifiedCmd( sequence_number );

	if ( !cmd || !cmd->m_command_number )
		return;

	g_cl.m_packet = true;

	g_cl.OnTick( cmd );

	bSendPacket = g_cl.m_packet;

	ver_cmd->m_cmd = *cmd;
	ver_cmd->m_crc = cmd->GetChecksum( );
}

__declspec( naked ) void __fastcall create_move::create_move_proxy( void* _this, int, int sequence_number, float input_sample_frametime, bool active )
{
	__asm
	{
		push ebp
		mov  ebp, esp
		push ebx
		push esp
		push dword ptr[ active ]
			push dword ptr[ input_sample_frametime ]
				push dword ptr[ sequence_number ]
					call create_move::hook
						pop  ebx
						pop  ebp
						retn 0Ch
	}
}


void DrawServerHitboxes() {
	if (!g_cl.m_local || !g_cl.m_local->alive()) { // we checking both cus why not, can't hurt us.
		return;
	}

	if (!g_csgo.m_input->CAM_IsThirdPerson()) { // useless in first-person.
		return;
	}

	// Function to get a player by index
	auto getPlayerByIndex = [](int index) -> Player* {
		typedef Player* (__fastcall* playerByIndex)(int);
		static auto playerIndexFunc = pattern::find(PE::GetModule(HASH("server.dll")), "85 C9 7E 2A A1").as<playerByIndex>();

		if (!playerIndexFunc) {
			return nullptr;
		}

		return playerIndexFunc(index);
		};

	// Find the function address
	static auto functionAddress = pattern::find(PE::GetModule(HASH("server.dll")), "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE").as<uintptr_t>();

	auto duration = -1.f;
	PVOID entity = nullptr;

	entity = getPlayerByIndex(g_cl.m_local->index());

	if (!entity) {
		return;
	}

	// keep your assembly I won't...
	typedef void(__fastcall* DrawFunction)(PVOID entity, float duration);
	DrawFunction drawFn = reinterpret_cast<DrawFunction>(functionAddress);

	drawFn(entity, duration);
}

void Hooks::FrameStageNotify( Stage_t stage ) {
	// save stage.
	if ( stage != FRAME_START )
		g_cl.m_stage = stage;

	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >( g_csgo.m_engine->GetLocalPlayer( ) );

	if ( stage == FRAME_RENDER_START ) {
		g_visuals.RemoveRecoil( true );
	}

	// call og.
	g_hooks.m_client.GetOldMethod< FrameStageNotify_t >( CHLClient::FRAMESTAGENOTIFY )( this, stage );

	switch ( stage ) {
	case FRAME_RENDER_START: {
		g_visuals.RemoveRecoil( false );
		g_shots.HandleMisses( );
	} break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_START: {
		g_skins.think( );
	} break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END: {
		g_visuals.NoSmoke( );
	} break;
	case FRAME_NET_UPDATE_END: {
		DrawServerHitboxes();
	} break;
	}
}