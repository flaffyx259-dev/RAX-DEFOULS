#include "includes.h"

Networking g_networking;

uint32_t GetHash( )
{
#ifdef _DEBUG
	return HASH_DEBUG;
#else
	return HASH_RELEASE;
#endif
}

void Networking::SendData( ) {
	CCLCMsg_VoiceData msg;
	memset( &msg, 0, sizeof( msg ) );

	g_csgo.ConstructVoiceMsg( ( void* )&msg, nullptr );

	VoiceData data;

	Packet* packet = reinterpret_cast< Packet* >( &data );

	packet->m_hash = GetHash( );

#ifndef _DEBUG
	packet->m_yaw = static_cast< int >( g_cl.m_angle.y );
#endif
	strcpy( packet->m_username, g_csgo.m_users[ 0 ].m_name.substr( 0, 12 ).c_str( ) );

	_String_t unk;
	msg.SetData( &data );
	msg.m_data = &unk;
	msg.m_format = 0; /*VoiceFormat_Steam*/
	msg.m_flags = 63;

	g_csgo.m_net->SendNetMsg( &msg, false, true );
}

void Networking::RecieveData( const CSVCMsg_VoiceData* msg ) {
	VoiceData* data = &msg->GetData( );
	if ( !data )
		return;

	Packet* packet = ( Packet* )data;
	if ( !packet || !packet->IsRax( ) )
		return;

	if ( ( msg->client - 1 ) == g_csgo.m_engine->GetLocalPlayer( ) )
		return;

	m_players[ msg->client ] = *packet;
}

Packet* Networking::GetUserData( int index )
{
	return &m_players[ index - 1 ];
}