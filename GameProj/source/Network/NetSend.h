#pragma once

#include "Globals.h"
#include "Network/Session.h"
#include "Protocol/ServerPacketHandler.h"

// 서버로 패킷 하나를 보낸다.
//
// 어느 쓰레드에서 불러도 안전하다. 송신 경로가 이미 보호되어 있다.
//  - MakeSendBuffer가 쓰는 버퍼 청크는 thread_local
//  - SendBuffer::PushSendQueue는 락을 잡고 복사
//
// PacketType은 ServerPacketHandler::MakeSendBuffer 오버로드가 있는 C_ 패킷이어야 한다.
// (없는 타입을 넣으면 컴파일 단계에서 걸린다)
template<typename PacketType>
void SendToServer(PacketType& pkt)
{
	if (GService == nullptr)
	{
		return;
	}

	Craft::Session* session = GService->GetSession();

	// 아직 연결 전이거나 이미 끊긴 상태.
	// 조용히 버린다 - 여기서 터뜨리면 종료 중에 날아온 패킷 하나로 게임이 죽는다.
	if (session == nullptr || session->IsConnected() == false)
	{
		return;
	}

	int32 size = 0;
	BYTE* buffer = ServerPacketHandler::MakeSendBuffer(pkt, OUT size);

	session->RegisterSend(buffer, size);
}
