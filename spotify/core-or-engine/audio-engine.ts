import { IAudioOutputDevice } from "../device-adapter/i-audio-output-device";
import { Song } from "../models/song";

export class AudioEngine {

    private currentSong: Song | null = null;
    private songIsPaused: boolean = false;

    getCurrentSongTitle(): string {
        if (this.currentSong) {
            return this.currentSong.getTitle();
        }

        return "No song is currently playing";
    }

    isPaused(): boolean {
        return this.songIsPaused;
    }

    play(aod: IAudioOutputDevice, song: Song): void {
        if (song == null) {
            throw new Error("Cannot play a null song");
        }

        if (this.songIsPaused && this.currentSong === song) {
            this.songIsPaused = false;
            aod.playAudio(song);
            return;
        }

        this.currentSong = song;
        this.songIsPaused = false;
        console.log(`Playing song: ${song.getTitle()}`);
        aod.playAudio(song);
    }

    pause(){
        if (this.currentSong == null) {
            throw new Error("No song is currently playing to pause");
        }

        if (this.songIsPaused) {
            throw new Error("Song is already paused");
        }

        this.songIsPaused = true;
        console.log(`Pausing song: ${this.currentSong.getTitle()}`);
    }
}