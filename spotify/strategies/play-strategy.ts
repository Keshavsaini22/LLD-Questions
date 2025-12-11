import { Playlist } from "../models/playlist";
import { Song } from "../models/song";

export abstract class PlayStrategy {
    abstract setPlaylist(pl: Playlist): void;
    abstract hasNext(): boolean;
    abstract next(): Song;
    abstract hasPrevious(): boolean;
    abstract previous(): Song;
    abstract addToNext(song: Song): void;
    //This method is specifically for CUSTOM_QUEUE strategy. It violates ISP but is added here for simplicity. So this is optional for other strategies. Wehave have taken the trade-off here.
}