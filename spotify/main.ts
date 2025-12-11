import { DeviceType } from "./enums/device-type.enum";
import { PlaylistStrategyType } from "./enums/playlist-strategy-type.enum";
import { MusicPlayerApplication } from "./music-player-application";


function main() {
    try {
        const application = MusicPlayerApplication.getInstance();

        // Populate library
        application.createSongInLibrary("Kesariya", "Arijit Singh", "/music/kesariya.mp3");
        application.createSongInLibrary("Chaiyya Chaiyya", "Sukhwinder Singh", "/music/chaiyya_chaiyya.mp3");
        application.createSongInLibrary("Tum Hi Ho", "Arijit Singh", "/music/tum_hi_ho.mp3");
        application.createSongInLibrary("Jai Ho", "A. R. Rahman", "/music/jai_ho.mp3");
        application.createSongInLibrary("Zinda", "Siddharth Mahadevan", "/music/zinda.mp3");

        // Create playlist and add songs
        application.createPlaylist("Bollywood Vibes");
        application.addSongToPlaylist("Bollywood Vibes", "Kesariya");
        application.addSongToPlaylist("Bollywood Vibes", "Chaiyya Chaiyya");
        application.addSongToPlaylist("Bollywood Vibes", "Tum Hi Ho");
        application.addSongToPlaylist("Bollywood Vibes", "Jai Ho");

        // Connect device
        application.connectAudioDevice(DeviceType.BLUETOOTH);

        // Play/pause a single song
        application.playSingleSong("Zinda");
        application.pauseCurrentSong("Zinda");
        application.playSingleSong("Zinda"); // resume

        console.log("\n-- Sequential Playback --");
        application.selectPlayStrategy(PlaylistStrategyType.SEQUENTIAL);
        application.loadPlaylist("Bollywood Vibes");
        application.playAllTracksInPlaylist();

        console.log("\n-- Random Playback --");
        application.selectPlayStrategy(PlaylistStrategyType.RANDOM);
        application.loadPlaylist("Bollywood Vibes");
        application.playAllTracksInPlaylist();

        console.log("\n-- Custom Queue Playback --");
        application.selectPlayStrategy(PlaylistStrategyType.CUSTOM_QUEUE);
        application.loadPlaylist("Bollywood Vibes");
        application.queueSongNext("Kesariya");
        application.queueSongNext("Tum Hi Ho");
        application.playAllTracksInPlaylist();

        console.log("\n-- Play Previous in Sequential --");
        application.selectPlayStrategy(PlaylistStrategyType.SEQUENTIAL);
        application.loadPlaylist("Bollywood Vibes");
        application.playAllTracksInPlaylist();

        application.playPreviousTrackInPlaylist();
        application.playPreviousTrackInPlaylist();

    } catch (error: any) {
        console.error("Error:", error.message);
    }
}

main();
