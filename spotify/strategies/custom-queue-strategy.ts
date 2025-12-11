import { Playlist } from "../models/playlist";
import { Song } from "../models/song";
import { PlayStrategy } from "./play-strategy";

export class CustomQueueStrategy extends PlayStrategy {
    private currentPlaylist: Playlist | null = null;
    private currentIndex: number = -1;
    private nextQueue: Song[] = [];   // queue → push(), shift()
    private prevStack: Song[] = [];   // stack → push(), pop()

    constructor() {
        super();
    }

    setPlaylist(playlist: Playlist): void {
        this.currentPlaylist = playlist;
        this.currentIndex = -1;
        this.nextQueue = [];
        this.prevStack = [];
    }

    private nextSequential(): Song {
        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            throw new Error("Playlist is empty.");
        }

        this.currentIndex += 1;
        return this.currentPlaylist.getSongs()[this.currentIndex];
    }

    private previousSequential(): Song {
        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            throw new Error("Playlist is empty.");
        }

        this.currentIndex -= 1;
        return this.currentPlaylist.getSongs()[this.currentIndex];
    }

    hasNext(): boolean {
        if (!this.currentPlaylist) return false;
        return this.currentIndex + 1 < this.currentPlaylist.getSize();
    }

    next(): Song {
        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            throw new Error("No playlist loaded or playlist is empty.");
        }

        // 1️⃣ If queued songs exist → they take priority
        if (this.nextQueue.length > 0) {
            const nextSong = this.nextQueue.shift()!;
            this.prevStack.push(nextSong);

            // update index to match the queued song
            const list = this.currentPlaylist.getSongs();
            this.currentIndex = list.indexOf(nextSong);

            return nextSong;
        }

        // 2️⃣ Otherwise sequential
        return this.nextSequential();
    }

    hasPrevious(): boolean {
        return this.currentIndex - 1 >= 0;
    }

    previous(): Song {
        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            throw new Error("No playlist loaded or playlist is empty.");
        }

        // 1️⃣ If stack has history → use it
        if (this.prevStack.length > 0) {
            const prevSong = this.prevStack.pop()!;

            const list = this.currentPlaylist.getSongs();
            this.currentIndex = list.indexOf(prevSong);

            return prevSong;
        }

        // 2️⃣ Otherwise sequential
        return this.previousSequential();
    }

    addToNext(song: Song): void {
        if (!song) {
            throw new Error("Cannot enqueue null song.");
        }
        this.nextQueue.push(song);
    }
}