import { Playlist } from "../models/playlist";
import { Song } from "../models/song";

export class PlaylistManager {
    private static instance: PlaylistManager | null = null;

    private playlists: Map<string, Playlist>;

    private constructor() {
        this.playlists = new Map();
    }

    public static getInstance(): PlaylistManager {
        if (!PlaylistManager.instance) {
            PlaylistManager.instance = new PlaylistManager();
        }
        return PlaylistManager.instance;
    }

    public createPlaylist(name: string): void {
        if (this.playlists.has(name)) {
            throw new Error(`Playlist "${name}" already exists.`);
        }

        this.playlists.set(name, new Playlist(name));
    }

    public addSongToPlaylist(playlistName: string, song: Song): void {
        if (!this.playlists.has(playlistName)) {
            throw new Error(`Playlist "${playlistName}" not found.`);
        }

        const playlist = this.playlists.get(playlistName)!;
        playlist.addSongToPlaylist(song);
    }

    public getPlaylist(name: string): Playlist {
        if (!this.playlists.has(name)) {
            throw new Error(`Playlist "${name}" not found.`);
        }

        return this.playlists.get(name)!;
    }
}
