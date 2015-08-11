/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/



#include "AssetCache/AssetCacheServer.h"
#include "AssetCache/AssetCacheConstants.h"
#include "AssetCache/CachedItemValue.h"
#include "AssetCache/CacheItemKey.h"
#include "AssetCache/TCPConnection/TCPConnection.h"
#include "Debug/DVAssert.h"
#include "FileSystem/KeyedArchive.h"


namespace DAVA
{
    
namespace AssetCache
{
    
Server::~Server()
{
}

bool Server::Listen(uint16 port)
{
    listenPort = port;
    DVASSERT(!netServer);
    
    netServer.reset(TCPConnection::CreateServer(NET_SERVICE_ID, Net::Endpoint(listenPort)));
    netServer->SetListener(this);

    return netServer->IsConnected();
}
    
bool Server::IsConnected() const
{
    if(netServer)
    {
        return netServer->IsConnected();
    }
    
    return false;
}

void Server::Disconnect()
{
    if(netServer)
    {
        netServer->Disconnect();
        netServer->SetListener(nullptr);
        netServer.reset();
    }
}
    
void Server::PacketReceived(DAVA::TCPChannel *tcpChannel, const uint8* packet, size_t length)
{
    if(length)
    {
        ScopedPtr<KeyedArchive> archive(new KeyedArchive());
        archive->Deserialize(packet, length);
        
        const auto packetID = archive->GetUInt32("PacketID", PACKET_UNKNOWN);
        switch (packetID)
        {
            case PACKET_ADD_REQUEST:
                OnAddToCache(tcpChannel, archive);
                break;
                
            case PACKET_GET_REQUEST:
                OnGetFromCache(tcpChannel, archive);
                break;
                
            case PACKET_WARMING_UP_REQUEST:
                OnWarmingUp(tcpChannel, archive);
                break;
                
            default:
                Logger::Error("[AssetCache::Server::%s] Invalid packet id: (%d). Closing channel", __FUNCTION__, packetID);
                netServer->DestroyChannel(tcpChannel);
                break;
        }
    }
}
    
void Server::ChannelClosed(TCPChannel *tcpChannel, const char8* message)
{
    if(delegate)
    {
        delegate->OnChannelClosed(tcpChannel, message);
    }
}

    
bool Server::AddedToCache(DAVA::TCPChannel *tcpChannel, const CacheItemKey &key, bool added)
{
    if(tcpChannel)
    {
        ScopedPtr<KeyedArchive> archieve(new KeyedArchive());
        archieve->SetUInt32("PacketID", PACKET_ADD_RESPONSE);

        ScopedPtr<KeyedArchive> keyArchieve(new KeyedArchive());
        SerializeKey(key, keyArchieve);
        archieve->SetArchive("key", keyArchieve);
       
        archieve->SetBool("added", added);
        
        return tcpChannel->SendArchieve(archieve);
    }
    
    return false;
}
    
    
bool Server::Send(DAVA::TCPChannel *tcpChannel, const CacheItemKey &key, const CachedItemValue &value)
{
    if(tcpChannel)
    {
        ScopedPtr<KeyedArchive> archieve(new KeyedArchive());
        archieve->SetUInt32("PacketID", PACKET_GET_RESPONSE);

        ScopedPtr<KeyedArchive> keyArchieve(new KeyedArchive());
        SerializeKey(key, keyArchieve);
        archieve->SetArchive("key", keyArchieve);

		ScopedPtr<KeyedArchive> valueArchieve(new KeyedArchive());
		value.Serialize(valueArchieve, true);
		archieve->SetArchive("value", valueArchieve);

        return tcpChannel->SendArchieve(archieve);
    }
    
    return false;
}


void Server::OnAddToCache(DAVA::TCPChannel *tcpChannel, KeyedArchive * archieve)
{
    if(delegate)
    {
        KeyedArchive *keyArchieve = archieve->GetArchive("key");
        DVASSERT(keyArchieve);
        
        CacheItemKey key;
        DeserializeKey(key, keyArchieve);
        
		KeyedArchive *valueArchieve = archieve->GetArchive("value");
		DVASSERT(valueArchieve);
        
		CachedItemValue value;
		value.Deserialize(valueArchieve);
        
		delegate->OnAddToCache(tcpChannel, key, std::forward<CachedItemValue>(value));
    }
    else
    {
        Logger::Error("[Server::%s] delegate not installed", __FUNCTION__);
    }
}
    
    
void Server::OnGetFromCache(DAVA::TCPChannel *tcpChannel, KeyedArchive * archieve)
{
    if(delegate)
    {
        KeyedArchive *keyArchieve = archieve->GetArchive("key");
        DVASSERT(keyArchieve);
        
        CacheItemKey key;
        DeserializeKey(key, keyArchieve);
        
        delegate->OnRequestedFromCache(tcpChannel, key);
    }
    else
    {
        Logger::Error("[Server::%s] delegate not installed", __FUNCTION__);
    }
}
    
void Server::OnWarmingUp(DAVA::TCPChannel *tcpChannel, KeyedArchive * archieve)
{
    if(delegate)
    {
        KeyedArchive *keyArchieve = archieve->GetArchive("key");
        DVASSERT(keyArchieve);
        
        CacheItemKey key;
        DeserializeKey(key, keyArchieve);
        
        delegate->OnWarmingUp(tcpChannel, key);
    }
    else
    {
        Logger::Error("[Server::%s] delegate not installed", __FUNCTION__);
    }
}

    

}; // end of namespace AssetCache
}; // end of namespace DAVA

