import { IAudioOutputDevice } from "../device-adapter/i-audio-output-device";
import { DeviceType } from "../enums/device-type.enum";
import { DeviceFactory } from "../factories/device-factory";

export class DeviceManager {
    private static instance: DeviceManager | null = null;
    private currentOutputDevice: IAudioOutputDevice | null = null;

    private constructor() { }

    static getInstance(): DeviceManager {
        if (!DeviceManager.instance) {
            DeviceManager.instance = new DeviceManager();
        }
        return DeviceManager.instance;
    }

    connect(type: DeviceType) {
        if (this.currentOutputDevice) {
            this.currentOutputDevice = null;
        }
        this.currentOutputDevice = DeviceFactory.createDevice(type);

        switch (type) {
            case DeviceType.BLUETOOTH:
                console.log("Connected to Bluetooth Speaker");
                break;
            case DeviceType.WIRED_SPEAKER:
                console.log("Connected to Wired Speaker");
                break;
            case DeviceType.HEADPHONES:
                console.log("Connected to Headphones");
                break;
            default:
                throw new Error("Unknown device type");
        }
    }

    getOutputDevice(): IAudioOutputDevice {

        if (!this.currentOutputDevice) {
            throw new Error("No device connected");
        }

        return this.currentOutputDevice;
    }

    hasOutputDevice(): boolean {
        return this.currentOutputDevice !== null;
    }
}