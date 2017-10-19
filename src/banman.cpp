// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "banman.h"

#include "netaddress.h"
#include "utiltime.h"
#include "util.h"
#include "ui_interface.h"


BanMan::BanMan(fs::path ban_file, CClientUIInterface* client_interface, int64_t default_ban_time) : m_client_interface(client_interface), m_ban_db(std::move(ban_file)), m_default_ban_time(default_ban_time)
{
    if (m_client_interface)
        m_client_interface->InitMessage(_("Loading banlist..."));

    int64_t nStart = GetTimeMillis();
    m_is_dirty = false;
    banmap_t banmap;
    if (m_ban_db.Read(banmap)) {
        SetBanned(banmap); // thread save setter
        SetBannedSetDirty(false); // no need to write down, just read data
        SweepBanned(); // sweep out unused entries

        LogPrint(BCLog::NET, "Loaded %d banned node ips/subnets from banlist.dat  %dms\n",
            banmap.size(), GetTimeMillis() - nStart);
    } else {
        LogPrintf("Invalid or missing banlist.dat; recreating\n");
        SetBannedSetDirty(true); // force write
        DumpBanlist();
    }
}

BanMan::~BanMan()
{
    DumpBanlist();
}

void BanMan::DumpBanlist()
{
    SweepBanned(); // clean unused entries (if bantime has expired)

    if (!BannedSetIsDirty())
        return;

    int64_t nStart = GetTimeMillis();

    banmap_t banmap;
    GetBanned(banmap);
    if (m_ban_db.Write(banmap)) {
        SetBannedSetDirty(false);
    }

    LogPrint(BCLog::NET, "Flushed %d banned node ips/subnets to banlist.dat  %dms\n",
        banmap.size(), GetTimeMillis() - nStart);
}

void BanMan::ClearBanned()
{
    {
        LOCK(m_cs_banned);
        m_banned.clear();
        m_is_dirty = true;
    }
    DumpBanlist(); //store banlist to disk
    if(m_client_interface)
        m_client_interface->BannedListChanged();
}

bool BanMan::IsBanned(CNetAddr ip)
{
    LOCK(m_cs_banned);
    for (const auto& it : m_banned) {
        CSubNet subNet = it.first;
        CBanEntry banEntry = it.second;

        if (subNet.Match(ip) && GetTime() < banEntry.nBanUntil) {
            return true;
        }
    }
    return false;
}

bool BanMan::IsBanned(CSubNet subnet)
{
    LOCK(m_cs_banned);
    banmap_t::iterator i = m_banned.find(subnet);
    if (i != m_banned.end())
    {
        CBanEntry banEntry = (*i).second;
        if (GetTime() < banEntry.nBanUntil) {
            return true;
        }
    }
    return false;
}

void BanMan::Ban(const CNetAddr& addr, const BanReason &banReason, int64_t bantimeoffset, bool sinceUnixEpoch) {
    CSubNet subNet(addr);
    Ban(subNet, banReason, bantimeoffset, sinceUnixEpoch);
}

void BanMan::Ban(const CSubNet& subNet, const BanReason &banReason, int64_t bantimeoffset, bool sinceUnixEpoch) {
    CBanEntry banEntry(GetTime());
    banEntry.banReason = banReason;
    if (bantimeoffset <= 0)
    {
        bantimeoffset = m_default_ban_time;
        sinceUnixEpoch = false;
    }
    banEntry.nBanUntil = (sinceUnixEpoch ? 0 : GetTime() )+bantimeoffset;

    {
        LOCK(m_cs_banned);
        if (m_banned[subNet].nBanUntil < banEntry.nBanUntil) {
            m_banned[subNet] = banEntry;
            m_is_dirty = true;
        }
        else
            return;
    }
    if(m_client_interface)
        m_client_interface->BannedListChanged();

    if(banReason == BanReasonManuallyAdded)
        DumpBanlist(); //store banlist to disk immediately if user requested ban
}

bool BanMan::Unban(const CNetAddr &addr) {
    CSubNet subNet(addr);
    return Unban(subNet);
}

bool BanMan::Unban(const CSubNet &subNet) {
    {
        LOCK(m_cs_banned);
        if (!m_banned.erase(subNet))
            return false;
        m_is_dirty = true;
    }
    if(m_client_interface)
        m_client_interface->BannedListChanged();
    DumpBanlist(); //store banlist to disk immediately
    return true;
}

void BanMan::GetBanned(banmap_t &banMap)
{
    LOCK(m_cs_banned);
    // Sweep the banlist so expired bans are not returned
    SweepBanned();
    banMap = m_banned; //create a thread safe copy
}

void BanMan::SetBanned(const banmap_t &banMap)
{
    LOCK(m_cs_banned);
    m_banned = banMap;
    m_is_dirty = true;
}

void BanMan::SweepBanned()
{
    int64_t now = GetTime();

    LOCK(m_cs_banned);
    banmap_t::iterator it = m_banned.begin();
    while(it != m_banned.end())
    {
        CSubNet subNet = (*it).first;
        CBanEntry banEntry = (*it).second;
        if(now > banEntry.nBanUntil)
        {
            m_banned.erase(it++);
            m_is_dirty = true;
            LogPrint(BCLog::NET, "%s: Removed banned node ip/subnet from banlist.dat: %s\n", __func__, subNet.ToString());
        }
        else
            ++it;
    }
}

bool BanMan::BannedSetIsDirty()
{
    LOCK(m_cs_banned);
    return m_is_dirty;
}

void BanMan::SetBannedSetDirty(bool dirty)
{
    LOCK(m_cs_banned); //reuse m_banned lock for the isDirty flag
    m_is_dirty = dirty;
}
