#include <bits/stdc++.h>
using namespace std;

/* ================= SONG ================= */

class Song
{
    string title;
    string artist;
    string filePath;

public:
    Song(string title, string artist, string filePath)
        : title(title), artist(artist), filePath(filePath) {}

    string getTitle() { return title; }
    string getArtist() { return artist; }
    string getFilePath() { return filePath; }
};

/* ================= PLAYLIST ================= */

class Playlist
{
    string name;
    vector<Song *> songs;

public:
    Playlist(string name) : name(name) {}

    void addSong(Song *song)
    {
        songs.push_back(song);
    }

    vector<Song *> &getSongs()
    {
        return songs;
    }

    int size() { return songs.size(); }

    string getName() { return name; }
};

/* ================= DEVICE APIs ================= */

class BluetoothAPI
{
public:
    void play(string path)
    {
        cout << "Playing " << path << " via Bluetooth\n";
    }
};

class HeadphoneAPI
{
public:
    void play(string path)
    {
        cout << "Playing " << path << " via Headphones\n";
    }
};

/* ================= ADAPTER ================= */

class AudioDevice
{
public:
    virtual void playSong(Song *song) = 0;
    virtual ~AudioDevice() = default;
};

class BluetoothDevice : public AudioDevice
{
    BluetoothAPI api;

public:
    void playSong(Song *song) override
    {
        api.play(song->getFilePath());
    }
};

class HeadphoneDevice : public AudioDevice
{
    HeadphoneAPI api;

public:
    void playSong(Song *song) override
    {
        api.play(song->getFilePath());
    }
};

enum class DeviceType
{
    BLUETOOTH,
    HEADPHONE
};

/* ================= FACTORY ================= */

class DeviceFactory
{
public:
    static AudioDevice *createDevice(DeviceType type)
    {
        switch (type)
        {
        case DeviceType::BLUETOOTH:
            return new BluetoothDevice();
        case DeviceType::HEADPHONE:
            return new HeadphoneDevice();
        default:
            throw runtime_error("Invalid device");
        }
    }
};

/* ================= DEVICE MANAGER ================= */

class DeviceManager
{
    AudioDevice *currentDevice = nullptr;

    DeviceManager() {}

public:
    static DeviceManager &getInstance()
    {
        static DeviceManager instance;
        return instance;
    }

    void connect(DeviceType type)
    {
        currentDevice = DeviceFactory::createDevice(type);
        cout << "Device connected\n";
    }

    AudioDevice *getDevice()
    {
        if (!currentDevice)
            throw runtime_error("No device connected");
        return currentDevice;
    }
};

// class DeviceManager {
// private:
//     static DeviceManager* instance;

//     DeviceManager() {}

// public:
//     static DeviceManager* getInstance() {
//         if (instance == nullptr) {
//             instance = new DeviceManager();
//         }
//         return instance;
//     }
// };

/* ================= AUDIO ENGINE ================= */

class AudioEngine
{
    Song *currentSong = nullptr;
    bool paused = false;

public:
    void play(AudioDevice *device, Song *song)
    {
        if (!song)
            throw runtime_error("Song null");

        currentSong = song;
        paused = false;

        cout << "Now playing: " << song->getTitle() << "\n";
        device->playSong(song);
    }

    void pause()
    {
        if (!currentSong)
            throw runtime_error("No song playing");

        paused = true;
        cout << "Paused: " << currentSong->getTitle() << "\n";
    }
};

/* ================= STRATEGY ================= */

class PlayStrategy
{
public:
    virtual void setPlaylist(Playlist *playlist) = 0;
    virtual bool hasNext() = 0;
    virtual Song *next() = 0;
    virtual ~PlayStrategy() = default;
};

class SequentialStrategy : public PlayStrategy
{
    Playlist *playlist = nullptr;
    int index = 0;

public:
    void setPlaylist(Playlist *p) override
    {
        playlist = p;
        index = 0;
    }

    bool hasNext() override
    {
        return playlist && index < playlist->size();
    }

    Song *next() override
    {
        return playlist->getSongs()[index++];
    }
};

class RandomStrategy : public PlayStrategy
{
    vector<Song *> songs;

public:
    void setPlaylist(Playlist *p) override
    {
        songs = p->getSongs();
        shuffle(
            songs.begin(),
            songs.end(),
            mt19937(random_device()()));
    }

    bool hasNext() override
    {
        return !songs.empty();
    }

    Song *next() override
    {
        Song *s = songs.back();
        songs.pop_back();
        return s;
    }
};

/* ================= PLAYLIST MANAGER ================= */

class PlaylistManager
{
    unordered_map<string, Playlist *> playlists;

    PlaylistManager() {}

public:
    static PlaylistManager &getInstance()
    {
        static PlaylistManager instance;
        return instance;
    }

    void createPlaylist(string name)
    {
        playlists[name] = new Playlist(name);
    }

    void addSong(string playlistName, Song *song)
    {
        playlists[playlistName]->addSong(song);
    }

    Playlist *getPlaylist(string name)
    {
        return playlists[name];
    }
};

/* ================= SPOTIFY FACADE ================= */

class SpotifyFacade
{
    AudioEngine engine;
    PlayStrategy *strategy = nullptr;

    SpotifyFacade() {}

public:
    static SpotifyFacade &getInstance()
    {
        static SpotifyFacade instance;
        return instance;
    }

    void connectDevice(DeviceType type)
    {
        DeviceManager::getInstance().connect(type);
    }

    void setStrategy(PlayStrategy *s)
    {
        strategy = s;
    }

    void playSong(Song *song)
    {
        engine.play(DeviceManager::getInstance().getDevice(), song);
    }

    void playPlaylist(Playlist *playlist)
    {
        strategy->setPlaylist(playlist);

        while (strategy->hasNext())
        {
            Song *song = strategy->next();
            playSong(song);
        }
    }

    void pause()
    {
        engine.pause();
    }
};

/* ================= MAIN ================= */

int main()
{
    Song *s1 = new Song("Kesariya", "Arijit", "/music/kesariya.mp3");
    Song *s2 = new Song("Tum Hi Ho", "Arijit", "/music/tumh.mp3");
    Song *s3 = new Song("Zinda", "Siddharth", "/music/zinda.mp3");

    PlaylistManager &pm = PlaylistManager::getInstance();
    pm.createPlaylist("Bollywood");
    pm.addSong("Bollywood", s1);
    pm.addSong("Bollywood", s2);
    pm.addSong("Bollywood", s3);

    SpotifyFacade &spotify = SpotifyFacade::getInstance();

    spotify.connectDevice(DeviceType::BLUETOOTH);

    cout << "\nSequential:\n";
    spotify.setStrategy(new SequentialStrategy());
    spotify.playPlaylist(pm.getPlaylist("Bollywood"));

    cout << "\nRandom:\n";
    spotify.setStrategy(new RandomStrategy());
    spotify.playPlaylist(pm.getPlaylist("Bollywood"));

    spotify.pause();

    return 0;
}