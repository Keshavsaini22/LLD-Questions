import { Playlist } from "../models/playlist";
import { Song } from "../models/song";
import { PlayStrategy } from "./play-strategy";

export class SequentialPlayStrategy extends PlayStrategy {
    private currentPlaylist: Playlist | null = null;
    private currentIndex: number = -1;

    constructor() {
        super();
    }

    setPlaylist(playlist: Playlist): void {
        this.currentPlaylist = playlist;
        this.currentIndex = -1;
    }

    hasNext(): boolean {
        if (!this.currentPlaylist) return false;
        return (this.currentIndex + 1) < this.currentPlaylist.getSize();
    }

    next(): Song {
        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            throw new Error("No playlist loaded or playlist is empty.");
        }

        this.currentIndex++;
        return this.currentPlaylist.getSongs()[this.currentIndex];
    }

    hasPrevious(): boolean {
        return this.currentIndex - 1 >= 0;
    }

    previous(): Song {
        if (!this.currentPlaylist || this.currentPlaylist.getSize() === 0) {
            throw new Error("No playlist loaded or playlist is empty.");
        }

        this.currentIndex--;
        return this.currentPlaylist.getSongs()[this.currentIndex];
    }

    // For sequential strategy, addToNext is not supported
    addToNext(song: Song): void {
        throw new Error("SequentialPlayStrategy does not support addToNext().");
    }
}
