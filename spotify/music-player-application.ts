import { DeviceType } from "./enums/device-type.enum";
import { PlaylistStrategyType } from "./enums/playlist-strategy-type.enum";
import { PlaylistManager } from "./managers/playlist-manager";
import { Song } from "./models/song";
import { MusicPlayerFacade } from "./music-player-facade";


export class MusicPlayerApplication {
    private static instance: MusicPlayerApplication | null = null;
    private songLibrary: Song[] = [];

    private constructor() { }

    public static getInstance(): MusicPlayerApplication {
        if (!this.instance) {
            this.instance = new MusicPlayerApplication();
        }
        return this.instance;
    }

    createSongInLibrary(title: string, artist: string, path: string) {
        const newSong = new Song(title, artist, path);
        this.songLibrary.push(newSong);
    }

    findSongByTitle(title: string): Song | null {
        return this.songLibrary.find(s => s.getTitle() === title) || null;
    }

    createPlaylist(playlistName: string) {
        PlaylistManager.getInstance().createPlaylist(playlistName);
    }

    addSongToPlaylist(playlistName: string, songTitle: string) {
        const song = this.findSongByTitle(songTitle);
        if (!song) {
            throw new Error(`Song "${songTitle}" not found in library.`);
        }
        PlaylistManager.getInstance().addSongToPlaylist(playlistName, song);
    }

    connectAudioDevice(deviceType: DeviceType) {
        MusicPlayerFacade.getInstance().connectDevice(deviceType);
    }

    selectPlayStrategy(strategyType: PlaylistStrategyType) {
        MusicPlayerFacade.getInstance().setPlayStrategy(strategyType);
    }

    loadPlaylist(playlistName: string) {
        MusicPlayerFacade.getInstance().loadPlaylist(playlistName);
    }

    playSingleSong(songTitle: string) {
        const song = this.findSongByTitle(songTitle);
        if (!song) {
            throw new Error(`Song "${songTitle}" not found.`);
        }
        MusicPlayerFacade.getInstance().playSong(song);
    }

    pauseCurrentSong(songTitle: string) {
        const song = this.findSongByTitle(songTitle);
        if (!song) {
            throw new Error(`Song "${songTitle}" not found.`);
        }
        MusicPlayerFacade.getInstance().pauseSong(song);
    }

    playAllTracksInPlaylist() {
        MusicPlayerFacade.getInstance().playAllTracks();
    }

    playPreviousTrackInPlaylist() {
        MusicPlayerFacade.getInstance().playPreviousTrack();
    }

    queueSongNext(songTitle: string) {
        const song = this.findSongByTitle(songTitle);
        if (!song) {
            throw new Error(`Song "${songTitle}" not found.`);
        }
        MusicPlayerFacade.getInstance().enqueueNext(song);
    }
}
