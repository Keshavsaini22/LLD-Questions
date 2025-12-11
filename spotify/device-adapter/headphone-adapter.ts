import { HeadphoneAPI } from "../external-apis/headphone-api";
import { Song } from "../models/song";
import { IAudioOutputDevice } from "./i-audio-output-device";

export class HeadphoneAdapter implements IAudioOutputDevice {
    private headphoneAPI: HeadphoneAPI;

    constructor(headphoneAPI: HeadphoneAPI) {
        this.headphoneAPI = headphoneAPI;
    }

    playAudio(song: Song): void {
        this.headphoneAPI.playSongViaHeadphones(song.getFilePath());
    }
}