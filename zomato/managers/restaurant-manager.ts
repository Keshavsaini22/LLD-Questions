import { Restaurant } from "../models/restaurant";

export class RestaurantManager {
  private restaurants: Restaurant[] = [];
  private static instance: RestaurantManager | null = null;

  private constructor() {}

  public static getInstance(): RestaurantManager {
    if (!RestaurantManager.instance) {
      RestaurantManager.instance = new RestaurantManager();
    }
    return RestaurantManager.instance;
  }

  public addRestaurant(restaurant: Restaurant): void {
    this.restaurants.push(restaurant);
  }

  public searchRestaurantByLocation(location: string): Restaurant[] {
    location = location.toLowerCase();

    return this.restaurants.filter((restaurant) => restaurant.getLocation().toLowerCase() === location);
  }

  public listRestaurants(): Restaurant[] {
    return this.restaurants;
  }
}
