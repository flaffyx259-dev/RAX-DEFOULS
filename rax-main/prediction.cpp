#include "includes.h"

void __fastcall run_command::hook(void* ecx, void* edx, Player* player, CUserCmd* ucmd, void* moveHelper) {
	// how the hell did this even happen.
	if (player != g_cl.m_local)
		return;

	// this is a fake command.
	if ((ucmd->m_tick - g_csgo.m_cl->m_server_tick + game::TICKS_TO_TIME(g_cl.m_latency[INetChannel::FLOW_OUTGOING])) > g_csgo.sv_max_usercmd_future_ticks->GetInt())
		return;

	original(ecx, edx, player, ucmd, moveHelper);

	player->m_vphysicsCollisionState() = 0;

	// store data for later use.
	g_restore_data.Setup(player, ucmd->m_command_number);

	static auto last_count = 0;
	auto& client_impact_list = *(CUtlVector< ClientHitVerify_t >*)((uintptr_t)g_cl.m_local + 0xBA84);

	if (g_menu.main.visuals.bullet_impacts.get(0))
	{
		for (auto i = client_impact_list.Count(); i > last_count; i--)
		{
			g_csgo.m_debug_overlay->AddBoxOverlay(client_impact_list[i - 1].m_pos,
				{ -2, -2, -2 },
				{ 2, 2, 2 },
				{ 0, 0, 0 },
				255, 0, 0, 127,
				4.f);
		}
	}

	if (client_impact_list.Count() != last_count)
		last_count = client_impact_list.Count();
}

bool __fastcall in_prediction::hook(void* ecx, void* edx) {
	if (setup_bones::allow)
		return false;

	if (update_clientside_animation::allow)
		return false;

	return original(ecx, edx);
}