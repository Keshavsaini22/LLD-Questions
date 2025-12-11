import { AudioEngine } from "./core-or-engine/audio-engine";
import { IAudioOutputDevice } from "./device-adapter/i-audio-output-device";
import { DeviceType } from "./enums/device-type.enum";
import { PlaylistStrategyType } from "./enums/playlist-strategy-type.enum";
import { DeviceManager } from "./managers/device-manager";
import { PlaylistManager } from "./managers/playlist-manager";
import { StrategyManager } from "./managers/strategy-manger";
import { Playlist } from "./models/playlist";
import { Song } from "./models/song";
import { PlayStrategy } from "./strategies/play-strategy";

export class MusicPlayerFacade {
    private static instance: MusicPlayerFacade | null = null;

    private audioEngine: AudioEngine;
    private loadedPlaylist: Playlist | null;
    private playStrategy: PlayStrategy | null;

    private constructor() {
        this.audioEngine = new AudioEngine();
        this.loadedPlaylist = null;
        this.playStrategy = null;
    }

    public static getInstance(): MusicPlayerFacade {
        if (!MusicPlayerFacade.instance) {
            MusicPlayerFacade.instance = new MusicPlayerFacade();
        }
        return MusicPlayerFacade.instance;
    }

    //DEVICE MANAGEMENT
    public connectDevice(deviceType: DeviceType): void {
        DeviceManager.getInstance().connect(deviceType);
    }

    //STRATEGY MANAGEMENT
    public setPlayStrategy(strategyType: PlaylistStrategyType): void {
        this.playStrategy = StrategyManager.getInstance().getStrategy(strategyType);
    }

    //PLAYLIST MANAGEMENT
    public loadPlaylist(name: string): void {
        this.loadedPlaylist = PlaylistManager.getInstance().getPlaylist(name);

        if (!this.playStrategy) {
            throw new Error("Play strategy not set before loading.");
        }

        this.playStrategy.setPlaylist(this.loadedPlaylist);
    }

    //songs control

    public playSong(song: Song): void {
        const deviceManager = DeviceManager.getInstance();

        if (!deviceManager.hasOutputDevice()) {
            throw new Error("No audio device connected.");
        }

        const device: IAudioOutputDevice = deviceManager.getOutputDevice();
        this.audioEngine.play(device, song);
    }

    public pauseSong(song: Song): void {
        if (this.audioEngine.getCurrentSongTitle() !== song.getTitle()) {
            throw new Error(
                `Cannot pause "${song.getTitle()}"; not currently playing.`
            );
        }
        this.audioEngine.pause();
    }


    //playback control

    public playAllTracks(): void {
        if (!this.loadedPlaylist) {
            throw new Error("No playlist loaded.");
        }

        if (!this.playStrategy) {
            throw new Error("Play strategy is not set.");
        }

        const device = DeviceManager.getInstance().getOutputDevice();

        while (this.playStrategy.hasNext()) {
            const nextSong = this.playStrategy.next();
            this.audioEngine.play(device, nextSong);
        }

        console.log(`Completed playlist: ${this.loadedPlaylist.getName()}`);
    }
    public playNextTrack(): void {
        if (!this.loadedPlaylist) {
            throw new Error("No playlist loaded.");
        }

        if (!this.playStrategy) {
            throw new Error("Play strategy is not set.");
        }

        const device = DeviceManager.getInstance().getOutputDevice();

        if (this.playStrategy.hasNext()) {
            const nextSong = this.playStrategy.next();
            this.audioEngine.play(device, nextSong);
        } else {
            console.log(`Completed playlist: ${this.loadedPlaylist.getName()}`);
        }
    }

    public playPreviousTrack(): void {
        if (!this.loadedPlaylist) {
            throw new Error("No playlist loaded.");
        }

        if (!this.playStrategy) {
            throw new Error("Play strategy is not set.");
        }

        const device = DeviceManager.getInstance().getOutputDevice();

        if (this.playStrategy.hasPrevious()) {
            const prevSong = this.playStrategy.previous();
            this.audioEngine.play(device, prevSong);
        } else {
            console.log(`Completed playlist: ${this.loadedPlaylist.getName()}`);
        }
    }

    //custom queue 
    public enqueueNext(song: Song): void {
        if (!this.playStrategy) {
            throw new Error("Play strategy not set.");
        }
        this.playStrategy.addToNext(song);
    }

}