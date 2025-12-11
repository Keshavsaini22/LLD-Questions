import { WiredSpeakerAPI } from "../external-apis/wired-speaker-api";
import { Song } from "../models/song";
import { IAudioOutputDevice } from "./i-audio-output-device";

export class WiredSpeakerAdapter implements IAudioOutputDevice {

    private wiredSpeakerAPI: WiredSpeakerAPI;

    constructor(wiredSpeakerAPI: WiredSpeakerAPI) {
        this.wiredSpeakerAPI = wiredSpeakerAPI;
    }

    playAudio(song: Song): void {
        this.wiredSpeakerAPI.playSongViaWiredSpeaker(song.getFilePath());
    }

}