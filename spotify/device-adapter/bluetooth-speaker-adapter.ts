import { BluetoothSpeakerAPI } from "../external-apis/bluetooth-speaker-api";
import { Song } from "../models/song";
import { IAudioOutputDevice } from "./i-audio-output-device";

export class BluetoothSpeakerAdapter implements IAudioOutputDevice {
    private bluetoothSpeakerAPI: BluetoothSpeakerAPI;

    constructor(bluetoothSpeakerAPI: BluetoothSpeakerAPI) {
        this.bluetoothSpeakerAPI = bluetoothSpeakerAPI;
    }
    playAudio(song: Song): void {
        this.bluetoothSpeakerAPI.playSongViaBluetooth(song.getFilePath());
    }
}