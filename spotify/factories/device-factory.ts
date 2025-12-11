import { BluetoothSpeakerAdapter } from "../device-adapter/bluetooth-speaker-adapter";
import { HeadphoneAdapter } from "../device-adapter/headphone-adapter";
import { IAudioOutputDevice } from "../device-adapter/i-audio-output-device";
import { WiredSpeakerAdapter } from "../device-adapter/wired-speaker-adapter";
import { DeviceType } from "../enums/device-type.enum";
import { BluetoothSpeakerAPI } from "../external-apis/bluetooth-speaker-api";
import { HeadphoneAPI } from "../external-apis/headphone-api";
import { WiredSpeakerAPI } from "../external-apis/wired-speaker-api";

export class DeviceFactory {
    static createDevice(type: DeviceType): IAudioOutputDevice {
        switch (type) {
            case DeviceType.BLUETOOTH: return new BluetoothSpeakerAdapter(new BluetoothSpeakerAPI());
            case DeviceType.WIRED_SPEAKER: return new WiredSpeakerAdapter(new WiredSpeakerAPI());
            case DeviceType.HEADPHONES: return new HeadphoneAdapter(new HeadphoneAPI());
            default: throw new Error("Unknown device type");
        }
    }
}