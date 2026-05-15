// Online C++ compiler to run C++ program online
#include <bits/stdc++.h>
#include <iostream>
using namespace std;

// songs
// playlist
// adapter pattern-> devices
// factory for making devices
// strategy pattern for different kind of playlist play
// audio manager
// Spotify facade payttern
class Song
{
public:
    int id;
    string name;
    string url;
    string artist;

    Song(int id, string name, string url, string artist)
    {
        this->id = id;
        this->name = name;
        this->url = url;
        this->artist = artist;
    }
};

class Playlist
{
public:
    int id;
    string name;
    vector<Song *> songs;

    Playlist(int id, string name, vector<Song *> songs)
    {
        this->id;
        this->name = name;
        this->songs = songs;
    }

    void addSong(Song *song)
    {
        songs.push_back(song);
    }
};

class PlaylistManager
{
    unordered_map<int, Playlist *> playlists;
    static int index;
    static PlaylistManager *instance;

    PlaylistManager()
    {
    }

public:
    static PlaylistManager *getInstance()
    {
        if (!instance)
        {
            instance = new PlaylistManager();
        }

        return instance;
    }

    void addPlaylist(string name, vector<Song *> songs)
    {
        Playlist *playlist = new Playlist(index, name, songs);

        playlists[index] = playlist;

        index++;
    }

    void song(int id, Song *song)
    {
        Playlist *playlist = playlists[id];
        playlist->addSong(song);
    }
};
int PlaylistManager::index = 0;
PlaylistManager *PlaylistManager::instance = nullptr;

class BlueToothAPI
{
public:
    void playSongViaBluetooth(Song *song)
    {
        cout << "PLAYING SONG IN BLUETOOTH" << " " << song->name << "..........";
    }
};

class EarPhoneAPI
{
public:
    void playSongViaEarPhone(Song *song)
    {
        cout << "PLAYING SONG IN EARPHONE" << " " << song->name << "..........";
    }
};

class PhoneAPI
{
public:
    void playSongViaPhone(Song *song)
    {
        cout << "PLAYING SONG IN PHONE" << " " << song->name << "..........";
    }
};

// ADAPTER

class AudioDevice
{
public:
    virtual void playSong(Song *song);
};

class BlueToothDevice : public AudioDevice
{
    BlueToothAPI *api;

public:
    BlueToothDevice()
    {
        api = new BlueToothAPI();
    }

    void playSong(Song *song)
    {
        api->playSongViaBluetooth(song);
    }
};

class EarPhoneDevice : public AudioDevice
{
    EarPhoneAPI *api;

public:
    EarPhoneDevice()
    {
        api = new EarPhoneAPI();
    }

    void playSong(Song *song)
    {
        api->playSongViaEarPhone(song);
    }
};

class PhoneDevice : public AudioDevice
{
    PhoneAPI *api;

public:
    PhoneDevice()
    {
        api = new PhoneAPI();
    }

    void playSong(Song *song)
    {
        api->playSongViaPhone(song);
    }
};

enum class DeviceType
{
    Phone,
    BLueTooth,
    EarPhone
};

class DeviceFactory
{
    static DeviceFactory *instance;
    DeviceFactory()
    {
    }
    static DeviceFactory *getInstance()
    {
        if (!instance)
        {
            instance = new DeviceFactory();
        }
        return instance;
    }

public:
    AudioDevice *createDevice(DeviceType type)
    {
        switch (type)
        {
        case DeviceType::Phone:
            return new PhoneDevice();

        case DeviceType::EarPhone:
            return new EarPhoneDevice();

        case DeviceType::BLueTooth:
            return new BlueToothDevice();

        default:
            throw runtime_error("Type not defined");
        }
    }
};

class PlayListStrategy
{
};

class RandomPlaylistStrategy : public PlayListStrategy
{
};

class SequentialStrategy : public PlayListStrategy
{
};

class CustomPlaylistStrategy : public PlayListStrategy
{
};

enum class AudioStatus
{
    PLAYING,
    PAUSE
};

class AudioEngine
{
    Song *currentSong = nullptr;
    AudioStatus status = nullptr;

public:
    play(Song *song)
    {
    }

    pause()
    {
    }
};

class Spotify
{
};

int main()
{
    cout << "Your code starts..........";
}
