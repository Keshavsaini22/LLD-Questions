import { Song } from "./song";

export class Playlist{
    private name: string;
    private songs: Song[];

    constructor(name: string){
        this.name = name;
        this.songs = [];
    }

    getName(): string{
        return this.name;
    }

    getSongs(): Song[]{
        return this.songs;
    }

    getSize(): number{
        return this.songs.length;
    }

    addSongToPlaylist(song: Song): void{
        this.songs.push(song);
    }
}