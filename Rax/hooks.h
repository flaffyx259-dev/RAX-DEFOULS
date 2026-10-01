#pragma once

struct CSVCMsg_VoiceData;

namespace get_data_map {
	// this shit got inlined apparently
	bool prepare_data_map(datamap2_t* map);

	datamap2_t* __fastcall hook(Player* player, void* edx);
	using fn = datamap2_t * (__thiscall*)(void*);
	inline fn original = nullptr;
}

namespace create_move {
	void __stdcall hook(int sequence_number, float input_sample_frametime, bool active, bool& bSendPacket);
	void __fastcall create_move_proxy(void* _this, int, int sequence_number, float input_sample_frametime, bool active);
	using fn = void* (__fastcall*)(void*, int, int, float, bool);
	inline fn original = nullptr;
}

//namespace update_animstate {
//	inline bool allow = false;
//
//	void __vectorcall hook( CCSGOPlayerAnimState* this_, void* a, float b, float yaw, float pitch, void* unk );
//	typedef void( __vectorcall* fn )( CCSGOPlayerAnimState* this_, void* a, float b, float yaw, float pitch, void* unk );
//	inline fn original;
//}

namespace call_post_data_updates {
	void __fastcall hook(void* ecx, void* edx);
	typedef void(__fastcall* fn)(void* ecx, void* edx);
	inline fn original;
}

namespace get_eye_angles {
	ang_t* __fastcall hook(void* ecx, void* edx);
	using fn = ang_t * (__fastcall*)(void*, void*);
	inline fn original = nullptr;
}

namespace extra_bones_processing {
	void __fastcall hook(void* ecx, void* edx, int a2, int a3, int a4, int a5, int a6, int a7);
	using fn = void(__fastcall*)(void*, void*, int, int, int, int, int, int);
	inline fn original = nullptr;
}

namespace setup_bones {
	inline bool allow = false;

	bool __fastcall hook(void* ecx, void* edx, BoneArray* bone_to_world_out, int max_bones, int bone_mask, float curtime);
	using fn = bool(__fastcall*)(void*, void*, BoneArray*, int, int, float);
	inline fn original = nullptr;
}

namespace interpolate_server_entities {
	void __fastcall hook();
	using fn = void(__fastcall*)();
	inline fn original = nullptr;
}

namespace modify_eye_position {
	void __fastcall hook(CCSGOPlayerAnimState* state, uintptr_t edx, vec3_t& position);
	using fn = void(__fastcall*)(CCSGOPlayerAnimState*, uintptr_t edx, vec3_t&);
	inline fn original = nullptr;
}

namespace send_data_gram {
	int __fastcall hook(void* ecx, void* edx, void* data);
	using fn = int(__fastcall*)(void*, void*, void*);
	inline fn original = nullptr;
}

namespace process_packet {
	void __fastcall hook(void* ecx, int edx, void* packet, bool bHasHeader);
	using fn = void(__fastcall*)(void* ecx, int edx, void* packet, bool bHasHeader);
	inline fn original = nullptr;
}

namespace packet_start {
	inline std::vector<int> sequences;

	void __fastcall hook(void* _this, int edx, int nIncomingSequence, int nOutgoingAcknowledged);
	typedef void(__fastcall* fn)(void* _this, int edx, int nIncomingSequence, int nOutgoingAcknowledged);
	inline fn original = nullptr;
}

namespace maintain_sequence_transitions {
	void __fastcall hook(void* _this, void* boneSetup, float flCycle, void* pos, void* q);
	using fn = void(__fastcall*)(void* _this, void* boneSetup, float flCycle, void* pos, void* q);
	inline fn original;
}

namespace run_command
{
	void __fastcall hook(void* ecx, void* edx, Player* player, CUserCmd* ucmd, void* moveHelper);
	using fn = void(__fastcall*)(void*, void*, Player*, CUserCmd*, void*);
	inline fn original = nullptr;
}

namespace in_prediction
{
	bool __fastcall hook(void* ecx, void* edx);
	using fn = bool(__fastcall*)(void*, void*);
	inline fn original = nullptr;
}

namespace voice_data {
	bool __fastcall hook(void* ecx, void* edx, const CSVCMsg_VoiceData& msg);
	using fn = bool(__fastcall*)(void*, void*, const CSVCMsg_VoiceData&);
	inline fn original = nullptr;
}

namespace update_clientside_animation {
	inline bool allow = false;

	void __fastcall hook(Player* player);
	using fn = void(__fastcall*)(Player*);
	inline fn original = nullptr;
}

namespace on_latch_interpolated_vars {
	void __fastcall hook(Entity* entity, void* edx, int flags);
	using fn = void(__thiscall*)(Entity* entity, int flags);
	inline fn original = nullptr;
}

namespace cmd_execute_command {


	void* __fastcall hook(int target, CCommand* cmd);
	using fn = void*(__fastcall*)(int target, CCommand* cmd);
	inline fn original = nullptr;
}

class Hooks {
public:
	void init();

public:
	// forward declarations
	class IRecipientFilter;

	// prototypes.
	using PaintTraverse_t = void(__thiscall*)(void*, VPANEL, bool, bool);
	using DoPostScreenSpaceEffects_t = bool(__thiscall*)(void*, CViewSetup*);
	using LevelInitPostEntity_t = void(__thiscall*)(void*);
	using LevelShutdown_t = void(__thiscall*)(void*);
	using LevelInitPreEntity_t = void(__thiscall*)(void*, const char*);
	using IN_KeyEvent_t = int(__thiscall*)(void*, int, int, const char*);
	using FrameStageNotify_t = void(__thiscall*)(void*, Stage_t);
	using UpdateClientSideAnimation_t = void(__thiscall*)(void*);
	using GetActiveWeapon_t = Weapon * (__thiscall*)(void*);
	using DoExtraBoneProcessing_t = void(__thiscall*)(void*, int, int, int, int, int, int);
	using BuildTransformations_t = void(__thiscall*)(void*, int, int, int, int, int, int);
	using CalcViewModelView_t = void(__thiscall*)(void*, vec3_t&, ang_t&);
	using OverrideView_t = void(__thiscall*)(void*, CViewSetup*);
	using LockCursor_t = void(__thiscall*)(void*);
	using GetScreenSize_t = void(__thiscall*)(void*, int&, int&);
	using Push3DView_t = void(__thiscall*)(void*, CViewSetup&, int, void*, void*);
	using SceneEnd_t = void(__thiscall*)(void*);
	using DrawModelExecute_t = void(__thiscall*)(void*, uintptr_t, const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
	using ComputeShadowDepthTextures_t = void(__thiscall*)(void*, const CViewSetup&, bool);
	using GetInt_t = int(__thiscall*)(void*);
	using GetBool_t = bool(__thiscall*)(void*);
	using IsConnected_t = bool(__thiscall*)(void*);
	using IsHLTV_t = bool(__thiscall*)(void*);
	using OnEntityCreated_t = void(__thiscall*)(void*, Entity*);
	using OnEntityDeleted_t = void(__thiscall*)(void*, Entity*);
	using RenderSmokeOverlay_t = void(__thiscall*)(void*, bool);
	using ShouldDrawFog_t = bool(__thiscall*)(void*);
	using ShouldDrawParticles_t = bool(__thiscall*)(void*);
	using Render2DEffectsPostHUD_t = void(__thiscall*)(void*, const CViewSetup&);
	using OnRenderStart_t = void(__thiscall*)(void*);
	using RenderView_t = void(__thiscall*)(void*, const CViewSetup&, const CViewSetup&, int, int);
	using GetMatchSession_t = CMatchSessionOnlineHost * (__thiscall*)(void*);
	using OnScreenSizeChanged_t = void(__thiscall*)(void*, int, int);
	using OverrideConfig_t = bool(__thiscall*)(void*, MaterialSystem_Config_t*, bool);
	using PostDataUpdate_t = void(__thiscall*)(void*, DataUpdateType_t);
	// using PreDataUpdate_t            = void( __thiscall* )( void*, DataUpdateType_t );

public:
	void                     PaintTraverse(VPANEL panel, bool repaint, bool force);
	bool                     DoPostScreenSpaceEffects(CViewSetup* setup);
	void                     LevelInitPostEntity();
	void                     LevelShutdown();
	//int                    IN_KeyEvent( int event, int key, const char* bind );
	void                     LevelInitPreEntity(const char* map);
	void                     FrameStageNotify(Stage_t stage);
	bool                     ShouldDrawParticles();
	bool                     ShouldDrawFog();
	void                     OverrideView(CViewSetup* view);
	void                     LockCursor();
	void                     OnScreenSizeChanged(int oldwidth, int oldheight);
	//void                   GetScreenSize( int& w, int& h );
	void                     SceneEnd();
	void                     DrawModelExecute(uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone);
	void                     ComputeShadowDepthTextures(const CViewSetup& view, bool unk);
	bool                     IsConnected();
	bool                     IsHLTV();
	void                     RenderSmokeOverlay(bool unk);
	void                     OnRenderStart();
	void                     RenderView(const CViewSetup& view, const CViewSetup& hud_view, int clear_flags, int what_to_draw);
	void                     Render2DEffectsPostHUD(const CViewSetup& setup);
	CMatchSessionOnlineHost* GetMatchSession();
	bool                     OverrideConfig(MaterialSystem_Config_t* config, bool update);

	static LRESULT WINAPI WndProc(HWND wnd, uint32_t msg, WPARAM wp, LPARAM lp);

public:
	// vmts.
	VMT m_panel;
	VMT m_client_mode;
	VMT m_client;
	VMT m_engine;
	VMT m_surface;
	VMT m_render;
	VMT m_render_view;
	VMT m_model_render;
	VMT m_shadow_mgr;
	VMT m_view_render;
	VMT m_match_framework;
	VMT m_material_system;

	// player shit.
	std::array< VMT, 64 > m_player;

	// wndproc old ptr.
	WNDPROC m_old_wndproc;

	// netvar proxies.
	RecvVarProxy_t m_Force_original;
};

extern Hooks                g_hooks;