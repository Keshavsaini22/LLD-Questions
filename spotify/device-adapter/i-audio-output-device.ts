import { Song } from "../models/song";

export interface IAudioOutputDevice {
    playAudio(song: Song): void;
}