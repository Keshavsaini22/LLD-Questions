import { MenuItem } from "./menu-item";

export class Restaurant {
  private static nextOrderId = 0;

  private id: number;
  private name: string;
  private location: string;
  private menus: MenuItem[];

  constructor(name: string, location: string) {
    this.id = ++Restaurant.nextOrderId;; //auto generated
    this.name = name;
    this.location = location;
  }

  getName(): string {
    return this.name;
  }

  getLocation(): string {
    return this.location;
  }

  getMenus(): MenuItem[] {
    return this.menus;
  }

  setName(name: string): void {
    this.name = name;
  }

  setLocation(location: string): void {
    this.location = location;
  }

  setMenu(menu: MenuItem): void {
    this.menus.push(menu);
  }
}
