import { PlaylistStrategyType } from "../enums/playlist-strategy-type.enum";
import { CustomQueueStrategy } from "../strategies/custom-queue-strategy";
import { PlayStrategy } from "../strategies/play-strategy";
import { RandomPlayStrategy } from "../strategies/random-play-strategy";
import { SequentialPlayStrategy } from "../strategies/sequential-play-strategy";


export class StrategyManager {
    private static instance: StrategyManager | null = null;

    private sequentialStrategy: SequentialPlayStrategy;
    private randomStrategy: RandomPlayStrategy;
    private customQueueStrategy: CustomQueueStrategy;

    private constructor() {
        this.sequentialStrategy = new SequentialPlayStrategy();
        this.randomStrategy = new RandomPlayStrategy();
        this.customQueueStrategy = new CustomQueueStrategy();
    }

    public static getInstance(): StrategyManager {
        if (!StrategyManager.instance) {
            StrategyManager.instance = new StrategyManager();
        }
        return StrategyManager.instance;
    }

    public getStrategy(type: PlaylistStrategyType): PlayStrategy {
        switch (type) {
            case PlaylistStrategyType.SEQUENTIAL:
                return this.sequentialStrategy;

            case PlaylistStrategyType.RANDOM:
                return this.randomStrategy;

            case PlaylistStrategyType.CUSTOM_QUEUE:
            default:
                return this.customQueueStrategy;
        }
    }
}
