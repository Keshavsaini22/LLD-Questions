import { Playlist } from "../models/playlist";
import { Song } from "../models/song";
import { PlayStrategy } from "./play-strategy";


export class RandomPlayStrategy extends PlayStrategy {
    private currentPlaylist: Playlist | null = null;
    private remainingSongs: Song[] = [];
    private history: Song[] = []; // acts as stack

    constructor() {
        super();
    }

    setPlaylist(playlist: Playlist): void {
        this.currentPlaylist = playlist;

        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            this.remainingSongs = [];
            this.history = [];
            return;
        }

        // Make a fresh copy of the playlist songs
        this.remainingSongs = [...this.currentPlaylist.getSongs()];
        this.history = [];
    }

    hasNext(): boolean {
        return this.currentPlaylist !== null && this.remainingSongs.length > 0;
    }

    next(): Song {
        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            throw new Error("No playlist loaded or playlist is empty.");
        }

        if (this.remainingSongs.length === 0) {
            throw new Error("No songs left to play.");
        }

        const idx = Math.floor(Math.random() * this.remainingSongs.length);
        const selectedSong = this.remainingSongs[idx];

        // ❗ Remove in O(1) (swap-and-pop trick)
        const lastIndex = this.remainingSongs.length - 1;
        [this.remainingSongs[idx], this.remainingSongs[lastIndex]] =
            [this.remainingSongs[lastIndex], this.remainingSongs[idx]];
        this.remainingSongs.pop();

        // Push to history stack
        this.history.push(selectedSong);

        return selectedSong;
    }

    hasPrevious(): boolean {
        return this.history.length > 0;
    }

    previous(): Song {
        if (this.history.length === 0) {
            throw new Error("No previous song available.");
        }

        return this.history.pop()!;
    }

    addToNext(song: Song): void {
        throw new Error("RandomPlayStrategy does not support addToNext().");
    }

}
