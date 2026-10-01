#include "includes.h"

bool Hooks::IsConnected( ) {
	return g_hooks.m_engine.GetOldMethod< IsConnected_t >( IVEngineClient::ISCONNECTED )( this );
}

bool Hooks::IsHLTV( ) {
	if (update_clientside_animation::allow )
		return true;

	if ( setup_bones::allow )
		return true;

	return g_hooks.m_engine.GetOldMethod< IsHLTV_t >( IVEngineClient::ISHLTV )( this );
}