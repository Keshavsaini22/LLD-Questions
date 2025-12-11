export class Song {
    private title: string;
    private artist: string;
    private filePath: string;

    constructor(title: string, artist: string, filePath: string) {
        this.title = title;
        this.artist = artist;
        this.filePath = filePath;
    }

    getTitle(): string {
        return this.title;
    }

    getArtist(): string {
        return this.artist;
    }

    getFilePath(): string {
        return this.filePath;
    }
}